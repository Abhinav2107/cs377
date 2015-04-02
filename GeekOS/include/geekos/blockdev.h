/*
 * Block devices
 * Copyright (c) 2001,2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * Copyright (c) 2003,2013,2014 Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 *
 * All rights reserved.
 *
 * This code may not be resdistributed without the permission of the copyright holders.
 * Any student solutions using any of this code base constitute derviced work and may
 * not be redistributed in any form.  This includes (but is not limited to) posting on
 * public forums or web sites, providing copies to (past, present, or future) students
 * enrolled in similar operating systems courses the University of Maryland's CMSC412 course.
 *
 * $Revision: 1.17 $
 * 
 */

#ifndef GEEKOS_BLOCKDEV_H
#define GEEKOS_BLOCKDEV_H

#include <geekos/ktypes.h>
#include <geekos/kthread.h>
#include <geekos/list.h>
#include <geekos/fileio.h>
#include <geekos/synch.h>
#ifdef GEEKOS

#define NUM_CACHE_BLOCKS 100

/*
 * Type of block device request.
 */
enum Request_Type {
    BLOCK_READ, BLOCK_WRITE
};

/*
 * State of a block I/O request.
 */
enum Request_State {
    PENDING, COMPLETED, ERROR
};

struct Block_Request;

/*
 * List of block I/O requests.
 */
DEFINE_LIST(Block_Request_List, Block_Request);

/*
 * An I/O request for a block device.
 */
struct Block_Request {
    struct Block_Device *dev;
    enum Request_Type type;
    int blockNum;
    void *buf;
    volatile enum Request_State state;
    volatile int errorCode;
    struct Thread_Queue waitQueue;

     DEFINE_LINK(Block_Request_List, Block_Request);
};

IMPLEMENT_LIST(Block_Request_List, Block_Request);

struct Block_Device;
struct Block_Device_Ops;

/*
 * A block device.
 */
struct Block_Device {
    char name[BLOCKDEV_MAX_NAME_LEN];
    struct Block_Device_Ops *ops;
    int unit;
    bool inUse;
    void *driverData;
    struct Thread_Queue *waitQueue;
    struct Block_Request_List *requestQueue;
    int referenced[NUM_CACHE_BLOCKS];
    int dirty[NUM_CACHE_BLOCKS];
    int replace_hand;
    int clear_hand;
    int cache_block[NUM_CACHE_BLOCKS];
    char cache[NUM_CACHE_BLOCKS][SECTOR_SIZE];
	struct Mutex cache_lock;
    unsigned int reads, writes; /* statistics */
    unsigned int hits, misses;
     DEFINE_LINK(Block_Device_List, Block_Device);
};

/*
 * Operations that may be requested on block devices.
 */
struct Block_Device_Ops {
    int (*Open) (struct Block_Device * dev);
    int (*Close) (struct Block_Device * dev);
    int (*Get_Num_Blocks) (struct Block_Device * dev);
};

/*
 * Low level block device API.
 * Only block device drivers need to use these functions.
 */
int Register_Block_Device(const char *name, struct Block_Device_Ops *ops,
                          int unit, void *driverData,
                          struct Thread_Queue *waitQueue,
                          struct Block_Request_List *requestQueue);
int Open_Block_Device(const char *name, struct Block_Device **pDev);
int Close_Block_Device(struct Block_Device *dev);
struct Block_Request *Create_Request(struct Block_Device *dev,
                                     enum Request_Type type, int blockNum,
                                     void *buf);
void Post_Request_And_Wait(struct Block_Request *request);
struct Block_Request *Dequeue_Request(struct Block_Request_List *requestQueue,
                                      struct Thread_Queue *waitQueue);
void Notify_Request_Completion(struct Block_Request *request,
                               enum Request_State state, int errorCode);

/*
 * High level block device API.
 * For use by filesystem and disk paging code.
 */
int Block_Read(struct Block_Device *dev, int blockNum, void *buf);
int Block_Write(struct Block_Device *dev, int blockNum, void *buf);
int Get_Num_Blocks(struct Block_Device *dev);
void Write_Cache_Back(struct Block_Device *dev);
/*
 * Misc. routines
 */

void Dump_Blockdev_Stats(void);

/*
 * Round offset up to nearest sector.
 */
static __inline__ ulong_t Round_Up_To_Block(ulong_t offset) {
    return (offset % SECTOR_SIZE) == 0
        ? offset : offset + (SECTOR_SIZE - (offset % SECTOR_SIZE));
}

/*
 * Round offset down to nearest sector.
 */
static __inline__ ulong_t Round_Down_To_Block(ulong_t offset) {
    return (offset % SECTOR_SIZE) == 0
        ? offset : offset - (offset % SECTOR_SIZE);
}

#endif /* GEEKOS */

#endif
