#include "fileio.h"
#include "block_dev.h"
#include "vfs.h"

int MAX_FILE_BLOCKS = 5000;		//In #blocks

int SIZE_OF_BLOCK = 1024;		//In #bytes

int NUM_OF_BLOCKS = 5000;

struct superBlock {
    int magic; //Magic Number to identify the filesystem
    int rootAddr;
	int numOfBlocks;
	int DSM[NUM_OF_BLOCKS];
};


struct FCB {
	int fd;
	FCB* nextFCB;
	FCB* prevFCB;
	char fname[200];	//File name
	char ftype[40];		//Type
	int recordSize;
	//int blockSize;
	//name of access method
	//Number of buffers
	//Name of access method
	
	//directory_struct* parentDirectory;
	FMT* fmt;
	char permissions[NUM_OF_USERS][3];	//assuming 10 users, 3 flags for read, write execute
	int fileSize;			//In bytes
	short mod_time;					
	short mod_date;
	int pid;		//Process id
};

struct FMT {
	int blockArray[MAX_FILE_BLOCKS]; // this is statically formed and designed to be the maximum number of blocks 
	//that are there in a file
	/// This needs to be multilevel file table
	int current_size;
};


struct directory_struct {
	FMT* fmt;
	char permissions[NUM_OF_USERS][3];
	int fileSize;
	short create_date;
	short create_time;
	short mod_time;					
	short mod_date;
};

/*
FCB* findFCB(OFT* _oft, int fd){

	int i=0;
	FCB* curFCB = _oft->head;
	while(1){
		if(curFCB == NULL) return NULL;
		if(i==fd) return curFCB;
		i++;
		curFCB = curFCB->next;		
	}
}
*/

void Init_MyFS(void);
