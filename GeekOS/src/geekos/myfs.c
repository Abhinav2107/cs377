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

struct superBlock sb;
unsigned int dsm_array[128];
int Allocate_Block(struct Block_Device * dev) {
	int i, j;
	for(i = 0; i < 128; i++) {
		for(j = 0; j < 32; j++) {
			if(!(dsm_array[i] & (1<<(31-j)))) {
					dsm_array[i] = (dsm_array[i] | (1<<(31-j)));
					Block_Write(dev, sb.dsm, dsm_array);
					return (32 * i) + j;
			}
		}
	}
	return -1;
}

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
    struct myfs_File f;
    struct FCB_Data * fdata = (struct FCB_Data *)file->fsData;
    if(! (file->mode & O_WRITE))
    	return EACCESS;
    Block_Read(file->mountPoint->dev, fdata->blockno, &f);
    int start = file->filePos;
    int end = start + numBytes;
    if(end > 121 * 512)
    	return ENOSPACE;
    int index;
    int offset;
    int pos = start;
    int written = 0;
    while(1) {
    	index = pos / 512;
    	offset = pos % 512;
    	int to_write = 512 - offset;
    	if(to_write > end - pos)
    		to_write = end - pos;
    	if(to_write <= 0)
    		break;
    	int blockno;
    	if((index == 0 && file->endPos == 0) || index > ((int)file->endPos - 1)/ 512) {
    		if((blockno = Allocate_Block(file->mountPoint->dev)) < 0)
    			break;
    		f.fmt[index] = blockno;
    		Block_Write(file->mountPoint->dev, fdata->blockno, &f);
    	}
    	else
    		blockno = f.fmt[index];
    	char temp[512];
    	Block_Read(file->mountPoint->dev, blockno, temp);
    	memcpy(temp+offset, buf+pos-start, to_write);
    	Block_Write(file->mountPoint->dev, blockno, temp);
    	pos += to_write;
    	if(pos > (int)file->endPos) {
    		file->endPos = pos;
    		f.fileSize = file->endPos;
    		Block_Write(file->mountPoint->dev, fdata->blockno, &f);
    	}
    	written += to_write;
    }
    file->filePos = pos;
    return written;
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
	if(!strcmp(path, "/writecacheback")) {
			Write_Cache_Back(mountPoint->dev);
			return -1;
	}
	char buf[16];
	char * name = buf;
	char * last = &buf[15];
	int lastblock = -1;
	memset(name, 0, 16);
	//Print("%s\n", path);
	path++; //Skip first slash
	struct myfs_directoryEntry dir;
	lastblock = sb.root;
	Block_Read(mountPoint->dev, lastblock, (char *)&dir);
	while(*path) {
		if(*path == '/') {
				if(*buf == '\x00')
					continue;
				else {
					int i;
					*name = '\x00';
					for(i = 0; i < 24; i++) {
						if(*dir.files[i] == '\x00')
								continue;
						if(!strcmp(dir.files[i], buf)) {
							lastblock = dir.fileblock[i];
							Block_Read(mountPoint->dev, lastblock, &dir);
							if(dir.type != 0)
									return ENOTDIR;
							name = buf;
							memset(name, 0, 16);
							break;
						}
					}
					if(i == 24)
							return ENOTFOUND;
				}
		}
		else {
			if(name == last)
					return ENAMETOOLONG;
			*name = *path;
			name++;
			path++;
		}
	}
	if(*buf == '\x00')
			return ENOTFOUND;
	int i;
	*name = '\x00';
	struct myfs_File f;
	memset(&f, 0, 512);
	for(i = 0; i < 24; i++) {
			if(*dir.files[i] == '\x00')
					continue;
			if(!strcmp(dir.files[i], buf)) {
					Block_Read(mountPoint->dev, dir.fileblock[i], &f);
					if(f.type != 1)
							return ENOTFOUND;
					break;
			}
	}
	int blockno;
	if(i == 24) {
			if(!(mode & O_CREATE))
				return ENOTFOUND;
			else {
				if(!(dir.perms & 2))
					return EACCESS;
				strcpy(f.fileName, buf);
				f.type = 1;
				f.perms = 7;
				blockno = Allocate_Block(mountPoint->dev);
				if(blockno < 0)
						return ENOSPACE;
				Block_Write(mountPoint->dev, blockno, &f);
				for(i = 0; i < 24; i++) {
					if(*dir.files[i] == '\x00') {
						strcpy(dir.files[i], f.fileName);
						dir.fileblock[i] = blockno;
						Block_Write(mountPoint->dev, lastblock, &dir);
						break;
					}
				}
				if(i == 24)
						return EUNSPECIFIED;
			}
	}
	else {
		if(mode & O_CREATE)
			return EEXIST;
		blockno = dir.fileblock[i];
	}
	if((mode & O_WRITE) && (!(f.perms & 2)))
		return EACCESS;
	if((mode & O_READ) && (!(f.perms & 4)))
		return EACCESS;
	struct FCB_Data * fdata = Malloc(sizeof(struct FCB_Data));
	fdata->blockno = blockno;
	*pFile = Allocate_File(&myfs_File_Ops, 0, f.fileSize, fdata, mode, mountPoint);
	if(!*pFile)
			return EUNSPECIFIED;
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
	memset(&sb, 0, 512);
    sb.magic=69;
    sb.dsm=1;
    sb.root=2;
    sb.n_blocks=2048;
    memset(dsm_array,0,512);
    dsm_array[0]=(1<<31)|(1<<30)|(1<<29);
    struct myfs_directoryEntry root;
    memset(&root,0,512);
    root.type=0;//Directory
    root.perms=7;
    Block_Write(blockDev,0,&sb);
    Block_Write(blockDev,1,dsm_array);
    Block_Write(blockDev,2,&root);
    Write_Cache_Back(blockDev);
    Print("Succesful Format\n");
    return 0;
}

static int myfs_Mount(struct Mount_Point * mountPoint) {

    Block_Read(mountPoint->dev,0,(char*)&sb);
    if(sb.magic!=69)
    {
        Print("Incorrect Magic Number\n");
        return EINVALIDFS;
    }
    Block_Read(mountPoint->dev, sb.dsm,(char*)dsm_array);
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
