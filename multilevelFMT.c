//Assuming that an fmt is stored in a single block

int get_from_fmt(struct Mount_Point *mountPoint,int block,int index){
	int cur_fmt[128];
	if(block == 0)
			return 0;
	Block_Read(mountPoint->dev,block,cur_fmt);
	if(index<127){
		return cur_fmt[index];
	}
	return get_from_fmt(mountPoint,cur_fmt[127],index-127);
}

int get_physical_block(struct Mount_Point *mountPoint,struct myfs_File f,int logical_block){
	if(logical_block < 120){
		return f.fmt[logical_block];
	}
	return get_from_fmt(mountPoint,f.fmt[120],logical_block-120);
}

int put_into_fmt(struct Mount_Point * mountPoint, int block, int index, int physical_block) {
	int cur_fmt[128];
	Block_Read(mountPoint->dev,block,cur_fmt);
	if(index < 127) {
			cur_fmt[index] = physical_block;
			Block_Write(mountPoint->dev, block, cur_fmt);
			return 0;
	}
	if(index == 127) {
		int newblock = Allocate_Block(mountPoint->dev);
		if(newblock < 0)
				return newblock;
		cur_fmt[127] = newblock;
		Block_Write(mountPoint->dev, block, cur_fmt);
		int new_fmt[128];
		memset(new_fmt, 0, 512);
		new_fmt[0] = physical_block;
		Block_Write(mountPoint->dev, newblock, new_fmt);
		return 0;
	}
	return put_into_fmt(mountPoint, cur_fmt[127], index-128, physical_block);
}

int put_physical_block(struct Mount_Point * mountPoint, struct myfs_File * f, int block, int logical_block, int physical_block) {
	if(logical_block < 120) {
		f->fmt[logical_block] = physical_block;
		Block_Write(mountPoint->dev, block, f);
		return 0;
	if(logical_block == 120) {
		int newblock = Allocate_Block(mountPoint->dev);
		if(newblock < 0)
				return newblock;
		f->fmt[120] = newblock;
		Block_Write(mountPoint->dev, block, f);
		int cur_fmt[128];
		memset(temp, 0, 512);
		cur_fmt[0] = physical_block;
		Block_Write(mountPoint->dev, newblock, cur_fmt);
		return 0;
	}
	return put_into_fmt(mountPoint, f.fmt[120], logical_block - 120, physical_block);
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
	char tempbuf[512];
	
	int physicalBlock = get_physical_block(file->mountPoint,f,curBlock);
	Block_Read(file->mountPoint->dev,physicalBlock,tempbuf);

	curBlock++;
	int i;

	for(i=already_read;i<512 && bytesRead < numBytes ; i++){
			((char*)buf)[bytesRead++]=tempbuf[i];
	}
	while(1){
			if((int)numBytes-(int)bytesRead < 512) break;
			physicalBlock = get_physical_block(file->mountPoint,f,curBlock);
			Block_Read(file->mountPoint->dev,physicalBlock,&((char*)buf)[bytesRead]);
			curBlock++;
			bytesRead+=512;
	}
	char tempbuf2[512];
	char myarr[512];
	int h;
	if((int)numBytes-(int)bytesRead > 0){
					physicalBlock = get_physical_block(file->mountPoint,f,curBlock);
					Block_Read(file->mountPoint->dev,physicalBlock,tempbuf2);
					//Print("block read: %d\n",y);
					for(i=0;bytesRead<numBytes;i++){
						((char*)buf)[bytesRead++]=tempbuf2[i];
					}
	}
	else {
		
	}
	file->filePos += bytesRead;
    return bytesRead;
}

static int myfs_Write(struct File * file, void * buf, ulong_t numBytes) {


	if(!(file->mode & O_READ)) return EACCESS;
	//if(file->mode && O_READ) return EACCESS;	
	
	struct myfs_File f;
	Block_Read(file->mountPoint->dev,((struct FCB_Data*)file->fsData)->blockno,&f);
	if((int)file->filePos+(int)numBytes>f.fileSize) {
		numBytes = f.fileSize - (int)file->filePos;
	}

	ulong_t bytesWritten=0;
	int curPos=file->filePos;
	int curBlock = curPos/512;
	int already_written = curPos%512;
	char tempbuf[512];
	int physicalBlock;
	int toWrite = numBytes;
	int minimum = 512 - already_written;
	if (toWrite < minimum)
			minimum = toWrite;
	if(already_writen > 0) {
		physicalBlock = get_physical_block(file->mountPoint,f,curBlock);
		Block_Read(file->mountPoint->dev,physicalBlock,tempbuf);
		memcpy(&tempbuf[already_written], buf, minimum);
		Block_Write(file->mountPoint->dev,physicalBlock, tempbuf);
		bytesWritten += minumum;
		toWrite -= minimum;
		curBlock++;
	}

	int i;

	while(1){
			if(toWrite < 512) break;
			physicalBlock = get_physical_block(file->mountPoint,f,curBlock);
			if(physicalBlock == 0) {
					physicalBlock = Allocate_Block(mountPoint->dev);
					if(physicalBlock < 0)
							return physicalBlock;
					put_physical_block(mountPoint, &f, ((struct FCB_Data *)file->fsData)->blockno, curBlock, physicalBlock);
			}
			Block_Write(file->mountPoint->dev,physicalBlock,&((char*)buf)[bytesWritten]);
			curBlock++;
			bytesWritten+=512;
			toWrite -= 512;
	}
	char tempbuf2[512];
	memset(tempbuf2, 0, 512);
	char myarr[512];
	int h;
	if(toWrite > 0){	
					physicalBlock = get_physical_block(file->mountPoint,f,curBlock);
					if(physicalBlock == 0) {
							physicalBlock = Allocate_Block(mountPoint->dev);
							if(physicalBlock < 0)
									return physicalBlock;
							put_physical_block(mountPoint, &f, ((struct FCB_Data *)file->fsData->blockno, curBlock, physicalBlock));
					}
					else {
							Block_Read(file->mountPoint->dev,physicalBlock,tempbuf2);
					}
					//Print("block read: %d\n",y);
					memcpy(tempbuf2, &buf[bytesWritten], toWrite);
					Block_Write(file->mountPoint->dev,physicalBlock,tempbuf2);
					bytesWritten += toWrite;
					toWrite = 0;
	}
	else {
		
	}
	file->filePos += bytesWritten;
    return bytesWritten;
}

void free_fmt(struct Mount_Point *mountPoint, int block) {
	if(block == 0)
			return;
	int fmt[128];
	int i;
	Block_Read(mountPoint->dev, block, fmt);
	for(i = 0; i < 127; i++) {
		if(fmt[i] != 0)
				Free_Block(mountPoint->dev, fmt[i]);
		else
				break;
	}
	if(i == 127)
			free_fmt(mountPoint, fmt[127]);
	Free_Block(mountPoint->dev, block);
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
				for(j = 0; j < 120; j++) {
					if(buf.fmt[j] != 0)
						Free_Block(mountPoint->dev, buf.fmt[j]);
					else
							break;
				}
				if(j == 120)
						free_fmt(mountPoint, buf.fmt[120]);
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
