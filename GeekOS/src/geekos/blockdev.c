/*
 * Block devices
 * Copyright (c) 2003 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.17 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/errno.h>
#include <geekos/screen.h>
#include <geekos/string.h>
#include <geekos/malloc.h>
#include <geekos/int.h>
#include <geekos/kthread.h>
#include <geekos/synch.h>
#include <geekos/blockdev.h>

/*#define BLOCKDEV_DEBUG */
#ifdef BLOCKDEV_DEBUG
#  define Debug(args...) Print(args)
#else
#  define Debug(args...)
#endif

extern Spin_Lock_t kthreadLock;

/* ----------------------------------------------------------------------
 * Private data and functions
 * ---------------------------------------------------------------------- */

/*
 * Lock protecting access/modification of block device list.
 */
static struct Mutex s_blockdevLock;

/*
 * List datatype for list of block devices.
 */
DEFINE_LIST(Block_Device_List, Block_Device);
IMPLEMENT_LIST(Block_Device_List, Block_Device);

/*
 * The list in which all block devices in the system
 * are registered.
 */
static struct Block_Device_List s_deviceList;


/*
 * Perform a block IO request.
 * Returns 0 if successful, error code on failure.
 */
static int Do_Request(struct Block_Device *dev, enum Request_Type type,
                      int blockNum, void *buf) {
    struct Block_Request *request;
    int rc;

    // Print("about to do_req\n");
    Mutex_Lock(&s_blockdevLock);
    request = Create_Request(dev, type, blockNum, buf);
    // Print("created req\n");
    if (request == 0) {
        // Print("req returned null\n");
        Mutex_Unlock(&s_blockdevLock);
        return ENOMEM;
    }
    // Print("about to post\n");
    Post_Request_And_Wait(request);
    rc = request->errorCode;
    Free(request);
    Mutex_Unlock(&s_blockdevLock);
    return rc;
}

/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */

/*
 * Register a block device.
 * This should be called by device drivers in their Init
 * functions to register all detected devices.
 * Returns 0 if successful, error code otherwise.
 */
int Register_Block_Device(const char *name, struct Block_Device_Ops *ops,
                          int unit, void *driverData,
                          struct Thread_Queue *waitQueue,
                          struct Block_Request_List *requestQueue) {
    struct Block_Device *dev;

    KASSERT(ops != 0);
    KASSERT(waitQueue != 0);
    KASSERT(requestQueue != 0);

    dev = (struct Block_Device *)Malloc(sizeof(*dev));
    if (dev == 0)
        return ENOMEM;

    strcpy(dev->name, name);
    dev->ops = ops;
    dev->unit = unit;
    dev->inUse = false;
    dev->driverData = driverData;
    dev->waitQueue = waitQueue;
    dev->requestQueue = requestQueue;
    int i;
    for(i = 0; i < NUM_CACHE_BLOCKS; i++) {
        dev->referenced[i] = 0;
        dev->cache_block[i] = -1;
        dev->dirty[i] = 0;
    }
    dev->replace_hand = NUM_CACHE_BLOCKS - 1;
    dev->clear_hand = 9;
    dev->reads = dev->writes = 0;
    dev->hits = dev->misses = 0;
    Mutex_Lock(&s_blockdevLock);
    /* FIXME: handle name conflict with existing device */
    Debug("Registering block device %s\n", dev->name);
    Add_To_Back_Of_Block_Device_List(&s_deviceList, dev);
    Mutex_Unlock(&s_blockdevLock);

    return 0;
}

/*
 * Open a named block device.
 * Return 0 if successful, error code on error.
 */
int Open_Block_Device(const char *name, struct Block_Device **pDev) {
    struct Block_Device *dev;
    int rc = 0;

    Mutex_Lock(&s_blockdevLock);

    dev = Get_Front_Of_Block_Device_List(&s_deviceList);
    while (dev != 0) {
        if (strcmp(dev->name, name) == 0)
            break;
        dev = Get_Next_In_Block_Device_List(dev);
    }

    if (dev == 0)
        rc = ENODEV;
    else if (dev->inUse)
        rc = EBUSY;
    else {
        rc = dev->ops->Open(dev);
        if (rc == 0) {
            *pDev = dev;
            dev->inUse = true;
        }
    }

    Mutex_Unlock(&s_blockdevLock);

    return rc;
}

/*
 * Close given block device.
 * Return 0 if successful, error code on error.
 */
int Close_Block_Device(struct Block_Device *dev) {
    int rc;
    Mutex_Lock(&s_blockdevLock);

    KASSERT(dev->inUse);
    rc = dev->ops->Close(dev);
    if (rc == 0)
        dev->inUse = false;

    Mutex_Unlock(&s_blockdevLock);

    return rc;
}

/*
 * Create a block device request to transfer a single block.
 */
struct Block_Request *Create_Request(struct Block_Device *dev,
                                     enum Request_Type type, int blockNum,
                                     void *buf) {
    struct Block_Request *request = Malloc(sizeof(*request));
    if (request != 0) {
        request->dev = dev;
        request->type = type;
        request->blockNum = blockNum;
        request->buf = buf;
        request->state = PENDING;
        Clear_Thread_Queue(&request->waitQueue);
    }
    return request;
}

/*
 * Send a block IO request to a device and wait for it to be handled.
 * Returns when the driver completes the requests or signals
 * an error.
 */
void Post_Request_And_Wait(struct Block_Request *request) {
    struct Block_Device *dev;

    KASSERT(request != 0);

    dev = request->dev;
    KASSERT(dev != 0);

    /* Send request to the driver */
    Debug("Posting block device request [@%x]...\n", request);
    Disable_Interrupts();
    Spin_Lock(&kthreadLock);
    Add_To_Back_Of_Block_Request_List(dev->requestQueue, request);
    Wake_Up_Locked(dev->waitQueue);
    Spin_Unlock(&kthreadLock);
    Enable_Interrupts();

    /* Wait for request to be processed */
    Disable_Interrupts();
    while (request->state == PENDING) {
        Debug("Waiting, state=%d\n", request->state);
        Wait(&request->waitQueue);
    }
    Debug("Wait completed!\n");
    Enable_Interrupts();
}

/*
 * Wait for a block request to arrive.
 */
struct Block_Request *Dequeue_Request(struct Block_Request_List *requestQueue,
                                      struct Thread_Queue *waitQueue) {
    struct Block_Request *request;

    Disable_Interrupts();
    while (Is_Block_Request_List_Empty(requestQueue))
        Wait(waitQueue);
    request = Get_Front_Of_Block_Request_List(requestQueue);
    Remove_From_Front_Of_Block_Request_List(requestQueue);
    Enable_Interrupts();

    return request;
}

/*
 * Signal the completion of a block request.
 */
void Notify_Request_Completion(struct Block_Request *request,
                               enum Request_State state, int errorCode) {
    Disable_Interrupts();
    // Spin_Lock(&kthreadLock);
    request->state = state;
    request->errorCode = errorCode;
    // Print("+++ in Notify_Request_Completion: core %d\n", Get_CPU_ID()); 
    Wake_Up(&request->waitQueue);
    // Print("--- in Notify_Request_Completion: core %d\n", Get_CPU_ID()); 
    // Spin_Unlock(&kthreadLock);
    Enable_Interrupts();
}

int Get_Cache_Index(struct Block_Device *dev, int blockNum) {
    int i;
    for(i = 0; i < NUM_CACHE_BLOCKS; i++) {
        if(blockNum == dev->cache_block[i]) {
            return i;
        }
    }
    return -1;
}

int Allocate_Cache_Index(struct Block_Device *dev) {
    int ret;
    while(1) {
        dev->replace_hand = (dev->replace_hand + 1) % NUM_CACHE_BLOCKS;
        ret = dev->replace_hand;
        if(dev->referenced[ret] == 0) {
            if(dev->dirty[ret]) {
                Do_Request(dev, BLOCK_WRITE, dev->cache_block[ret], dev->cache[ret]);
            }
            dev->dirty[ret] = 0;
            dev->referenced[dev->clear_hand] = 0;
            dev->clear_hand = (dev->clear_hand + 1) % NUM_CACHE_BLOCKS;
            dev->referenced[ret] = 1;
            return ret;
        }
        dev->referenced[dev->clear_hand] = 0;
        dev->clear_hand = (dev->clear_hand + 1) % NUM_CACHE_BLOCKS;
    }
}

/*
 * Read a block from given device.
 * Return 0 if successful, error code on error.
 */
int Block_Read(struct Block_Device *dev, int blockNum, void *buf) {
// 	Dump_Blockdev_Stats();
    KASSERT(dev);
    KASSERT(buf);
    dev->reads++;
    int index;
    int ret;
	Mutex_Lock(&dev->cache_lock);
    index = Get_Cache_Index(dev, blockNum);
    if(index < 0) {
        dev->misses++;
        index = Allocate_Cache_Index(dev);
        dev->cache_block[index] = blockNum;
        ret = Do_Request(dev, BLOCK_READ, blockNum, dev->cache[index]);
    }
    else {
        dev->hits++;
        dev->referenced[index] = 1;
        ret = 0;
    }
    memcpy(buf, dev->cache[index], SECTOR_SIZE);
	Mutex_Unlock(&dev->cache_lock);

	return ret;
}

/*
 * Write a block to given device.
 * Return 0 if successful, error code on error.
 */
int Block_Write(struct Block_Device *dev, int blockNum, void *buf) {
//	Dump_Blockdev_Stats();
    KASSERT(dev);
    KASSERT(buf);
    dev->writes++;
    int index;
	Mutex_Lock(&dev->cache_lock);
    index = Get_Cache_Index(dev, blockNum);
    if(index < 0) {
        dev->misses++;
		Mutex_Unlock(&dev->cache_lock);
        return Do_Request(dev, BLOCK_WRITE, blockNum, buf);
    }
    else {
        dev->hits++;
        memcpy(dev->cache[index], buf, SECTOR_SIZE);
        dev->referenced[index] = 1;
        dev->dirty[index] = 1;
		Mutex_Unlock(&dev->cache_lock);
 	return 0;
    }
}

void Write_Cache_Back(struct Block_Device *dev) {
	int i;
	Mutex_Lock(&dev->cache_lock);
	for(i = 0; i < NUM_CACHE_BLOCKS; i++) {
		if(dev->dirty[i] && dev->cache_block[i] >= 0) {
			Do_Request(dev, BLOCK_WRITE, dev->cache_block[i], dev->cache[i]);
			dev->dirty[i] = 0;
		}
	}
	Mutex_Unlock(&dev->cache_lock);

}

/*
 * Get number of blocks in given device.
 */
int Get_Num_Blocks(struct Block_Device *dev) {
    return dev->ops->Get_Num_Blocks(dev);
}

/* 
 * Dump block device statistics, mostly for performance diagnostics.
 */
void Dump_Blockdev_Stats(void) {
    struct Block_Device *dev;
    int i;
    Print("Block Device Stats:\n");
    Mutex_Lock(&s_blockdevLock);
    for (dev = Get_Front_Of_Block_Device_List(&s_deviceList), i = 5;
         dev != 0 && i > 0;
         dev = Get_Next_In_Block_Device_List(dev), i -= 1) {
        Print(" %s: read %u wrote %u\n", dev->name, dev->reads, dev->writes);
        Print(" %s: hits %u misses %u\n", dev->name, dev->hits, dev->misses);
    }
    Mutex_Unlock(&s_blockdevLock);
}
