#include <geekos/timer.h>
#include <geekos/ktypes.h>
#include <geekos/malloc.h>
#include <string.h>
#include <geekos/kthread.h>
#include <geekos/user.h>
#include <geekos/simd.h>

#define BLOCK_SIZE 512

#define NUM_TRACKS 128
#define BLOCKS_PER_TRACK 256
#define NUM_BLOCKS (1>>15)

#define TICKS_PER_SEEK 2

static int s_queueHead_simd, s_queueTail_simd;

static struct Thread_Queue s_waitQueue_simd;

ulong_t lastpos;
ulong_t finish;

struct event_list_ele {
    int id;
    struct Kernel_Thread * kt;
    int block_no;
    ulong_t user_ptr;
    struct event_list_ele * next;
    struct event_list_ele * prev;
};

struct event_list_ele * head;
struct event_list_ele * tail;

char data[BLOCK_SIZE];

void Init_SimD() {
    head = NULL;
    tail = NULL;
    memset(data, '\0', BLOCK_SIZE);
    finish = 0;
    lastpos = 0;
    s_queueHead_simd = s_queueTail_simd = 0;
}

int Track_From_Block(int block) {
    return block / BLOCKS_PER_TRACK;
}

void IO_Complete_Callback(int id) {
    struct event_list_ele * e = head;
    while(e != NULL) {
        if(e->id == id) {
            break;
        }
        e = e->next;
    }
    if(e == NULL) {
        //Error
    }
    Copy_To_User(e->user_ptr, data, BLOCK_SIZE);
    if(e->prev) {
        e->prev->next = e->next;
    }
    if(e->next) {
        e->next->prev = e->prev;
    }
    if(head == e) {
        head = e->next;
    }
    if(tail == e) {
        tail = e->prev;
    }
    Remove_Thread(&s_waitQueue_simd, e->kt);
    Make_Runnable(e->kt);
    Free(e);
}

int Movement(int block) {
    int track;
    track = Track_From_Block(block);
    int cost = track - lastpos;
    if(cost < 0)
        cost = -cost;
    cost *= TICKS_PER_SEEK;
    return cost;
}

int Start_IO(struct Kernel_Thread * kt, int block_no, ulong_t user_ptr) {
    if(block_no >= NUM_BLOCKS)
        return -1;
    int moveTime;
    moveTime = Movement(block_no);
    int ticks;
    ticks = finish - g_numTicks;
    if(ticks < 0)
        ticks = 0;
    ticks += moveTime;
    int id;
    id = Start_Timer(ticks, &IO_Complete_Callback);
    if(id >= 0) {
        struct event_list_ele * new = Malloc(sizeof(struct event_list_ele));
        new->id = id;
        new->kt = kt;
        new->user_ptr = user_ptr;
        new->block_no = block_no;
        new->next = NULL;
        new->prev = tail;
        if(head == NULL) {
            head = new;
        }
        else {
            tail->next = new;
        }
        tail = new;
        finish += moveTime;
        lastpos = Track_From_Block(block_no);
        Enqueue_Thread(&s_waitQueue_simd, kt);
    }
    return id;
}



