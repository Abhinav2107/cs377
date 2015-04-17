#include "fileio.h"
#include "blockdev.h"
#include "vfs.h"

#define MAX_FILE_BLOCKS 5000		//In #blocks
#define SIZE_OF_BLOCK 1024		//In #bytes
#define NUM_OF_BLOCKS 5000
#define NUM_OF_USERS  1

#define MYFS_BOOT_RECORD_OFFSET 350

#define MYFS_MAGIC 210

#define MAX_NAME_SIZE 16


struct myfs_directoryEntry{
	int type;
    char fileName[MAX_NAME_SIZE];
    int perms; // permission
    /* attribute bits */
    int next; // incase of overflow of directory
    char files[24][MAX_NAME_SIZE];
    int fileblock[24];
    int faltu;
};

struct myfs_File;

struct superBlock {
    int magic;
    int root;
    int dsm;
    int n_blocks;
    char padding[496];
};

struct myfs_File {
	int type;
	char fileName[16];
	int perms;
	int fileSize;
	int fmt[121];
};

struct FCB_Data {
	int blockno;
    int type;
};

void Init_myfs();

