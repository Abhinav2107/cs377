#include <limits.h>
#include <geekos/errno.h>
#include <geekos/screen.h>
#include <geekos/string.h>
#include <geekos/malloc.h>
#include <geekos/ide.h>
#include <geekos/blockdev.h>
#include <geekos/bitset.h>
#include <geekos/vfs.h>
#include <geekos/list.h>
#include <geekos/synch.h>
#include <geekos/myfs.h>
#include <geekos/projects.h>

#define PAGEFILE_FILENAME "/pagefile.bin"

int debugmyfs = 0;
#define Debug(args...) if (debugmyfs) Print("myfs: " args)


static struct myfs_directoryEntry *myfs_Lookup(struct Mount_Point *mountPoint,
                                   struct superBlock *sblock,
                                   const char *path) {
    return 0;
}



static void myfs_Register_Paging_File(struct Mount_Point *mountPoint, struct superBlock *sblock){

}




static int myfs_Read(struct File * file, void * buf, ulong_t numBytes) {
    return 0;
}

static int myfs_Write(struct File * file, void * buf, ulong_t numBytes) {
    return 0;
}

static int myfs_Seek(struct File * file, ulong_t pos) {
    return 0;
}

static int myfs_Close(struct File * file) {
    //Any close operations if required
    return 0;
}

static struct File_Ops myfs_File_Ops = {
    0, //Fstat
    &myfs_Read,
    &myfs_Write,
    &myfs_Seek,
    &myfs_Close,
    0, //Read_Entry
};

static struct myfs_File* Get_myfs_File(struct superBlock * sblock, struct myfs_directoryEntry * entry){

    return 0;
}

static int myfs_Open(struct Mount_Point * mountPoint, const char * path, int mode, struct File ** pFile) {
    return 0;

}

static int myfs_Create_Directory(struct Mount_Point * mountPoint, const char * path) {
    return 0;
}


static int Copy_Stat_For_myfs(struct VFS_File_Stat *stat, struct myfs_directoryEntry * entry){

    return 0;

}

static int myfs_Stat(struct Mount_Point * mountPoint, const char * path, struct VFS_File_Stat * stat) {
    return 0;
}

static int myfs_Delete(struct Mount_Point * mountPoint, const char * path, bool recursive) {
    //Delete the file or directory (if recursive) at the path
    return 0;
}

static int myfs_Rename(struct Mount_Point * mountPoint, const char * oldpath, const char * newpath) {
    return 0;

	


}

struct Mount_Point_Ops myfs_Mount_Point_Ops = {
    &myfs_Open,
    &myfs_Create_Directory,
    0, //Open_Directory
    &myfs_Stat,
    0, //Sync
    &myfs_Delete,
    &myfs_Rename,
    0, //Link
    0, //SymLink
    0, //SetSetUid
    0, //SetAcl
    0, //Disk_Properties
};

static int myfs_Format(struct Block_Device * blockDev) {
    struct superBlock s;
    s.magic=69;
    s.dsm=1;
    s.root=2;
    s.n_blocks=2048;
    memset(dsm_array,0,512);
    dsm_array[0]=(char)224;
    struct myfs_directoryEntry root;
    memset(&root,0,512);
    root.type=0;//
    root.perms=7;
    Block_Write(blockDev,0,(char*)&s);
    Block_Write(blockDev,1,dsm_array);
    Block_Write(blockDev,2,(char*)&root);
    Write_Cache_Back(blockDev);
    Print("Succesful Format\n");
    return 0;
}

static int myfs_Mount(struct Mount_Point * mountPoint) {

    struct superBlock s;
    Block_Read(mountPoint->dev,0,(char*)&s);
    if(s.magic!=69)
    {
        Print("Incorrect Magic Number\n");
        return -1;
    }
    
    mountPoint->ops = &myfs_Mount_Point_Ops;
    return 0;

}

static struct Filesystem_Ops myfs_Filesystem_Ops = {
    &myfs_Format,
    &myfs_Mount,
};

void Init_myfs() {
    Register_Filesystem("myfs", &myfs_Filesystem_Ops);
}
