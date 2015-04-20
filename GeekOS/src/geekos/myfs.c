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

void Free_Block(struct Block_Device * dev, int blockno) {
	dsm_array[blockno/32] = (dsm_array[blockno/32] & (~(1<<(31-(blockno%32)))));
	Block_Write(dev, sb.dsm, dsm_array);
}

static int myfs_Lookup(struct Mount_Point *mountPoint,
                                   const char *path, struct myfs_directoryEntry * direntry, int * index, int * blockno) {
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
	int idx = 1;
	*index = 1;
	while(*path) {
		if(*path == '/') {
				path++;
				idx++;
				*index = idx;
				if(*buf == '\x00') {
					continue;
				}
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
			idx++;
		}
	}

	*blockno = lastblock;
	*direntry = dir;
    return 0;
}



static void myfs_Register_Paging_File(struct Mount_Point *mountPoint, struct superBlock *sblock){

}


static int myfs_Seek(struct File * file, ulong_t pos) {
    if(pos > file->endPos)
        return EINVALID;
	file->filePos=pos;
    return 0;
}

static int myfs_Read(struct File * file, void * buf, ulong_t numBytes) {


	if(!(file->mode & O_READ)) return EACCESS;
	//if(file->mode && O_READ) return EACCESS;	
	
	struct myfs_File f;
	Block_Read(file->mountPoint->dev,((struct FCB_Data*)file->fsData)->blockno,&f);
	if((int)file->filePos+(int)numBytes>f.fileSize) {
		numBytes = f.fileSize - (int)file->filePos;
	}

	ulong_t bytesRead=0;
	int curPos=file->filePos;
	int curBlock = curPos/512;
	int already_read = curPos%512;
	char * tempbuf = Malloc(512);
	//Print("block num: %d\n",f.fmt[curBlock]);
	Block_Read(file->mountPoint->dev,f.fmt[curBlock],tempbuf);
	curBlock++;
	int i;

	for(i=already_read;i<512 && bytesRead < numBytes ; i++){
			((char*)buf)[bytesRead++]=tempbuf[i];
	}
	Free(tempbuf);
	while(1){
			if((int)numBytes-(int)bytesRead < 512) break;
			Block_Read(file->mountPoint->dev,f.fmt[curBlock++],&((char*)buf)[bytesRead]);
			bytesRead+=512;
	}
	char * tempbuf2=Malloc(512);
	char myarr[512];
	int h;
	//for(h=0;h<512;h++) myarr[h]='a'+(h)%25;
	if((int)numBytes-(int)bytesRead > 0){
					
					//int jk=0;	
					Block_Read(file->mountPoint->dev,f.fmt[curBlock++],tempbuf2);
					//Print("block read: %d\n",y);
					for(i=0;bytesRead<numBytes;i++){
						((char*)buf)[bytesRead++]=tempbuf2[i];
						//Print("tempbuf2 char: %c\n",tempbuf2[i]);
						//Print("cur char: %c\n",((char*)buf)[bytesRead-1]);
						//jk++;
					}
					//Print("jk: %d\n",jk);
	}
	else {
		
	}
	Free(tempbuf2);
	file->filePos += numBytes;
    return bytesRead;
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


static int myfs_FStat(struct File * file, struct VFS_File_Stat * stat) {
	stat->size = (int)file->endPos;
	stat->isDirectory = 0;
	stat->isSetuid = 0;
	stat->acls[0].permission = file->mode;
	return 0;
}

static int myfs_Close(struct File * file) {
    //Any close operations if required
	Free(file->fsData);
    return 0;
}

static int myfs_Read_Entry(struct File * dir, struct VFS_Dir_Entry * entry) {
    struct FCB_Data * fdata;
    fdata = (struct FCB_Data *)dir->fsData;
    struct myfs_directoryEntry direntry;
    struct myfs_File f;
    Block_Read(dir->mountPoint->dev, fdata->blockno, &direntry);
    while(dir->filePos < dir->endPos) {
        if(*direntry.files[dir->filePos] == '\x00') {
            dir->filePos++;
            continue;
        }
        Block_Read(dir->mountPoint->dev, direntry.fileblock[dir->filePos], &f);
        strcpy(entry->name, direntry.files[dir->filePos]);
        entry->stats.isSetuid = 0;
        entry->stats.acls[0].uid = 0;
        entry->stats.acls[0].permission = f.perms;
        if(f.type == 0) {
            entry->stats.size = 512;
            entry->stats.isDirectory = 1;
        }
        else {
            entry->stats.size = f.fileSize;
            entry->stats.isDirectory = 0;
        }
        dir->filePos++;
        return 0;
    }
    return 1;
}

static struct File_Ops myfs_File_Ops = {
    &myfs_FStat, //Fstat
    &myfs_Read,
    &myfs_Write,
    &myfs_Seek,
    &myfs_Close,
    &myfs_Read_Entry, //Read_Entry
};

static struct myfs_File* Get_myfs_File(struct superBlock * sblock, struct myfs_directoryEntry * entry){

    return 0;
}

static int myfs_Open(struct Mount_Point * mountPoint, const char * path, int mode, struct File ** pFile) {
	if(!strcmp(path, "/writecacheback")) {
			Write_Cache_Back(mountPoint->dev);
			return -1;
	}
	int lastblock;
    int index;
    struct myfs_directoryEntry dir;
    int ret;
    if((ret = myfs_Lookup(mountPoint, path, &dir, &index, &lastblock)) < 0)
        return ret;
	path = &(path[index]);
	if(*path == '\x00')
        return ENOTFOUND;
	int i;
	struct myfs_File f;
	memset(&f, 0, 512);
	for(i = 0; i < 24; i++) {
			if(*dir.files[i] == '\x00')
					continue;
			if(!strcmp(dir.files[i], path)) {
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
				strcpy(f.fileName, path);
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
						return ENOSPACE;
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
    fdata->type = 1;
	*pFile = Allocate_File(&myfs_File_Ops, 0, f.fileSize, fdata, mode, mountPoint);
	if(!*pFile)
			return EUNSPECIFIED;
	return 0;
}

static int myfs_Create_Directory(struct Mount_Point * mountPoint, const char * path) {
	struct myfs_directoryEntry temp;
	int index;
	int blockno;
	int ret = myfs_Lookup(mountPoint, path, &temp, &index, &blockno);
	if(ret < 0)
		return ret;
	if(!(temp.perms & O_WRITE))
		return EACCESS;
	struct myfs_directoryEntry new;
	memset(&new, 0, 512);
	new.perms = 7;
	new.type = 0;
	strcpy(new.fileName, &(path[index]));
	int block = Allocate_Block(mountPoint->dev);
	if(block < 0)
		return ENOSPACE;
	Block_Write(mountPoint->dev, block, &new);
	
	int i = 0;
	while(( i < 24) && (*(temp.files[i]) != '\x00'))
		i++;
	if(i == 24)
		return ENOSPACE;
	strcpy(temp.files[i], &(path[index]));
	temp.fileblock[i] = block;
	Block_Write(mountPoint->dev, blockno, &temp);

    return 0;
}

static int myfs_Open_Directory(struct Mount_Point * mountPoint, const char * path, struct File ** pdir) {
    int block;
    int index;
    int ret;
    struct myfs_directoryEntry dir;
    if((ret = myfs_Lookup(mountPoint, path, &dir, &index, &block)) < 0)
        return ret;
    path = &(path[index]);
    struct FCB_Data * fdata = (struct FCB_Data *)Malloc(sizeof(struct FCB_Data));
    fdata->type = 0;
    if(*path == '\x00') {
        fdata->blockno = block;
    }
	else {
		int i;
		for(i = 0; i < 24; i++) {
			if(!strcmp(path, dir.files[i])) {
				block = dir.fileblock[i];
				Block_Read(mountPoint->dev, block, &dir);
				if(dir.type != 0) {
					Free(fdata);
					return ENOTDIR;
				}
				fdata->blockno = block;
				break;
			}
		}
		if(i == 24) {
				Free(fdata);
				return ENOTFOUND;
		}
	}
    *pdir = Allocate_File(&myfs_File_Ops, 0, 24, fdata, 0, mountPoint);
    if(!(*pdir)) {
		Free(fdata);
        return ENOMEM;
	}
	return 0;

}

static int Copy_Stat_For_myfs(struct VFS_File_Stat *stat, struct myfs_directoryEntry * entry){
	

    return 0;

}

static int myfs_Stat(struct Mount_Point * mountPoint, const char * path, struct VFS_File_Stat * stat) {
    struct myfs_directoryEntry dirInfo;
    int index,block;
    int search=myfs_Lookup(mountPoint,path,&dirInfo,&index,&block);
    if(search<0) return search;
    int i;
    char fname[16];
    strcpy(fname,&path[index]);
	stat->isSetuid = 0;
	stat->acls[0].uid = 0;
	if(fname[0] == '\x00') {
		stat->isDirectory = 1;
		stat->size = 512;
		stat->acls[0].permission = dirInfo.perms;
		return 0;
	}
    int flag=0;
    int fileBlockNo;
    for(i=0;i<24;i++){
    	if(strcmp(fname,dirInfo.files[i])==0) {
				flag=1;
				fileBlockNo=dirInfo.fileblock[i];
				break;
		}
    }
    if(!flag) return ENOTFOUND;
    struct myfs_File f;
    Block_Read(mountPoint->dev,fileBlockNo,&f);
	//directory
	if(f.type==0){
			stat->isDirectory = 1;
			stat->size = 512;
	}
	//file
	else{
		stat->isDirectory = 0;
		stat->size = f.fileSize;
	}
	stat->acls[0].permission = f.perms;
    return 0;
}

int __myfs_Delete(struct Mount_Point *mountPoint, const char *path, bool recursive, struct myfs_directoryEntry *temp, int blockno) {
	int i;
	for(i = 0; i < 24; i++) {
		if(*temp->files[i] == '\x00')
				continue;
		if(path == NULL || (strcmp(path, temp->files[i]) == 0)) {
			struct myfs_File buf;
			Block_Read(mountPoint->dev, temp->fileblock[i], &buf);
			if(buf.type == 0) {
				if(!recursive)					
					return ENOTFOUND;
				
				int ret = __myfs_Delete(mountPoint, NULL, recursive, (struct myfs_directoryEntry *)(&buf), temp->fileblock[i]);
				if(ret != 0)
						return ret;
			}
			else {
				int j;
				for(j = 0; j < 121; j++) {
					if(buf.fmt[j] != 0)
						Free_Block(mountPoint->dev, buf.fmt[j]);
				}
			}
			Free_Block(mountPoint->dev, temp->fileblock[i]);
			memset(temp->files[i], 0, MAX_NAME_SIZE);
			temp->fileblock[i] = 0;
			Block_Write(mountPoint->dev, blockno, temp);
			if(path != NULL)
					return 0;
			
		}
	}

	if(path == NULL)
		return 0;
	return ENOTFOUND;
}

static int myfs_Delete(struct Mount_Point * mountPoint, const char * path, bool recursive) {
    //Delete the file or directory (if recursive) at the path
	int len = strlen(path);
	char * notconstpath = (char *)Malloc(len+1);
	strcpy(notconstpath, path);
	if(notconstpath[strlen(notconstpath)-1] == '/')
			notconstpath[strlen(notconstpath)-1] = '\x00';
	struct myfs_directoryEntry temp;
	int index;
	int blockno;
	int ret = myfs_Lookup(mountPoint, notconstpath, &temp, &index, &blockno);
	if(ret < 0)
		return ret;
	if(!(temp.perms & O_WRITE))
		return EACCESS;
	ret = __myfs_Delete(mountPoint, &(notconstpath[index]), recursive, &temp, blockno);
	Free(notconstpath);
	return ret;
}

//
static int myfs_Rename(struct Mount_Point * mountPoint, const char * oldpath, const char * newpath) {
	struct myfs_directoryEntry dirInfo;
    int index,block;
    int search=myfs_Lookup(mountPoint,oldpath,&dirInfo,&index,&block);
    if(search<0) return ENOTFOUND;
    int i;
    char fname[16];
    strcpy(fname,&oldpath[index]);
    int flag=0;
    int fileBlockNo;
    int entryNo;
    for(i=0;i<24;i++){
    	if(dirInfo.fileblock[i]!=0){
    		if(strcmp(fname,dirInfo.files[i])==0) {
    			flag=1;fileBlockNo=dirInfo.fileblock[i];entryNo=i;break;
    		}
    	}
    }
    if(!flag) return ENOTFOUND;



    struct myfs_directoryEntry newDirInfo;
    int newIndex,newBlock;
    int newSearch=myfs_Lookup(mountPoint,newpath,&newDirInfo,&newIndex,&newBlock);
    if(newSearch<0) return ENOTFOUND;
    char newFname[16];
    strcpy(newFname,&newpath[newIndex]);
    flag=0;
    int newFileBlockNo;
    int emptyIndex=-1;
    for(i=0;i<24;i++){
    	if(newDirInfo.fileblock[23-i]==0) emptyIndex=i;
    	if(strcmp(newFname,newDirInfo.files[i])==0) {flag=1;break;}
    }
    if(flag) return EEXIST;
    if(emptyIndex==-1) return ENOSPACE;	

    //remove from old path

    strcpy(dirInfo.files[entryNo],"");//Darsh
    /*int ij;
    for(ij=0;ij<MAX_NAME_SIZE;ij++)
    {
    	dirInfo.files[entryNo][ij]='';
    }*/
    dirInfo.fileblock[entryNo]=0;
    //write it back
    Block_Write(mountPoint->dev,block,(void*)&dirInfo);

    //enter into new path
    strcpy(newDirInfo.files[emptyIndex],(fname));//Darsh
    newDirInfo.fileblock[emptyIndex]=fileBlockNo;
    //write it back
    Block_Write(mountPoint->dev,newBlock,(void*)&newDirInfo);





	return 0;
}

struct Mount_Point_Ops myfs_Mount_Point_Ops = {
    &myfs_Open,
    &myfs_Create_Directory,
    &myfs_Open_Directory, //Open_Directory
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
