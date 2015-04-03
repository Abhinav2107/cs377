#include <geekos/myfs.h>

static int myfs_Read(struct File * file, void * buf, ulong_t numBytes) {
    //Read numBytes from file into buf at the position file->filePos
    return 0;
}

static int myfs_Write(struct File * file, void * buf, ulong_t numBytes) {
    //Write numBytes from buf into file at the position file->filePos
    return 0;
}

static int myfs_Seek(struct File * file, ulong_t pos) {
    //Set file->filePos to pos
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

static int myfs_Open(struct Mount_Point * mountPoint, const char * path, int mode, struct File ** pFile) {
    //Open a file, Create an FCB (struct File) using AllocateFile in vfs.c and set *pFile to that pointer
    //When calling AllocateFile use &myfs_File_Ops as the first argument
    return 0;
}

static int myfs_Create_Directory(struct Mount_Point * mountPoint, const char * path) {
    //Create a directory at this path
    return 0;
}

static int myfs_Stat(struct Mount_Point mountPoint, const char * path, struct VFS_File_Stat * stat) {
    //Return details about the file/directory specified by the path in the VFS_File_Stat structure
    return 0;
}

static int myfs_Delete(struct Mount_Point mountPoint, const char * path, bool recursive) {
    //Delete the file or directory (if recursive) at the path
    return 0;
}

static int myfs_Rename(struct Mount_Point mountPoint, const char * oldpath, const char * newpath) {
    return 0;
}

static struct Mount_Point_Ops myfs_Mount_Point_Ops = {
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
    //Format the block device to this filesystem
    return 0;
}

static int myfs_Mount(struct Mount_Point * mountPoint) {
    //Mount the filesystem at this mount point in the VFS
    //Eg:- If the mount point is "d", then the root directory of this filesystem be /d/
    //At some point you have to do mountPoint->ops = &myfs_Mount_Point_Ops;
    return 0;
}

static struct Filesystem_Ops myfs_Filesystem_Ops = {
    &myfs_Format,
    &myfs_Mount,
};

void Init_myfs() {
    Register_Filesystem("myfs", &myfs_Filesystem_Ops);
}
