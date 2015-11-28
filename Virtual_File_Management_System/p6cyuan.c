/*
	=============================================
	Virtual File Management System
	Author:	Chengjun Yuan    <cy3yb@virginia.edu>
	Time:	Nov.23 2015
	=============================================
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "disk.h"

int searchFile(char *fileName); // return -1 if no exist, else the file descriptor.
int searchFreeBlockInFat(void);
void freeDataBlock(int numBlock);
void saveOft(int fildes, int block, int offsetInBlock, int offsetFilePointer);

#define META_SIZE 176
char meta[META_SIZE];

int offsetDirectory = 1 * BLOCK_SIZE;
int offsetFat = 5 * BLOCK_SIZE; // File Allocation Table offset.
int offsetOft = 9 * BLOCK_SIZE; // Open File Table offset.
int unitDirectory = 8;
int unitFat = 2;
int unitOft = 4;

int make_fs(char *disk_name) {
	char buf[BLOCK_SIZE];
	if (make_disk(disk_name) == -1) return -1;	// make disk for file system.
	if (open_disk(disk_name) == -1) return -1;	// open disk.
	strcpy(buf, disk_name);						
	if (block_write(0, buf) == -1) return -1;	// write name of file system into disk.
	close_disk(); 								// close disk.
	return 0;
}

int mount_fs(char *disk_name) {
	char buf[BLOCK_SIZE];
	int i, j;
	if (open_disk(disk_name) == -1) return -1;
	// read metadata from the first 11 blocks of disk.
	for (i = 0; i < 11; i++) {
		if (block_read(i, buf) == -1) return -1;
		for (j = 0; j < 16; j++) {
			meta[i * 16 + j] = buf[j];
		}
	}
	return 0;
}

int dismount_fs(char *disk_name) {
	char buf[BLOCK_SIZE];
	int i, j, oftFlag;
	// close all opened files.
	for (i = 0; i < 8; i++) {
		oftFlag = meta[i * unitOft + offsetOft];
		oftFlag = oftFlag & 1;
		if (oftFlag == 1) {
			fs_close(i);
		}
	}
	// write metadata to the first 11 blocks of disk.
	for (i = 0; i < 11; i++) {
		for (j = 0; j < 16; j++) {
			buf[j] = meta[i * 16 + j];
		}
		if (block_write(i, buf) == -1) return -1;
	}
	// close disk.
	close_disk();
	return 0;
}

int fs_open(char *name) {
	int i, oftFlag, currBlock;
	// search the file descriptor.
	int fildes = searchFile(name);
	if (fildes == -1) {
		printf("*** Error: can not find file %s ***\n", name);
		return -1;
	}
	// check whether it has been opened but not closed before.
	oftFlag = meta[fildes * unitOft + offsetOft];
	oftFlag = oftFlag & 1;
	if (oftFlag == 1) {
		printf("*** Error: file %s has been opened ***\n", name);
		return -1;
	}
	// mark the file is opened.
	currBlock = meta[offsetDirectory + fildes * unitDirectory + 7];
	meta[fildes * unitOft + offsetOft] = 1 | (currBlock << 1);
	return fildes;
}

int fs_close(int fildes) {
	int oftFlag;
	// check whether fildes exist in directory.
	if (meta[offsetDirectory + fildes * 8] == 0) {
		printf("*** Error: fildes %d does not exist ***\n", fildes);
		return -1;
	}
	// check whether fildes is opened.
	oftFlag = meta[fildes * unitOft + offsetOft];
	oftFlag = oftFlag & 1;
	if (oftFlag == 0) {
		printf("*** Error: the fildes %d is not opened ***\n", fildes);
		return -1;
	}
	// clear OFT to mark this file closed.
	meta[fildes * unitOft + offsetOft] = 0;
	meta[fildes * unitOft + 1 + offsetOft] = 0;
	meta[fildes * unitOft + 2 + offsetOft] = 0;
	meta[fildes * unitOft + 3 + offsetOft] = 0;
	return 0;
}

int fs_create(char *name) {
	int i, j, len, firstBlock;
	// check the length of file name.
	len = strlen(name);
	if (strlen(name) > 4) {
		printf("*** Error: the lenght of file name - %s exceed 4 ***\n", name);
		return -1;
	}
	// check whether the file exists.
	if (searchFile(name) > -1) {
		printf("*** Error: file %s has existed in file system ***\n", name);
		return -1;
	}
	// search the available directory entry.
	for (i = 0; i < 8; i++) {
		if (meta[offsetDirectory + i * unitDirectory] == 0) break;
	}
	if (i >= 8) {
		printf("*** Error: the number of files exceeds 8 ***\n");
		return -1;
	}
	// check whether there are available data block in file disk.
	if ((firstBlock = searchFreeBlockInFat()) == -1) {
		return -1;
	}
	meta[offsetDirectory + i * unitDirectory] = 1;
	// save file name in to directory.
	for (j = 0; j < len; j++) {
		meta[offsetDirectory + i * unitDirectory + 3 + j] = name[j];
	}
	// save first block information into directory.
	meta[offsetDirectory + i * unitDirectory + 7] = firstBlock;
	// mark the block is used in FAT.
	meta[offsetFat + (firstBlock - 32) * unitFat] = 1; 
	return 0;
}

int fs_delete(char *name) {
	int fildes, firstBlock, i;
	// check whether the file exists.
	if ((fildes = searchFile(name)) == -1) {
		printf("*** Error: file %s does not existed in file system ***\n", name);
		return -1;
	}
	// check whether the file is opened now.
	if ((meta[offsetOft + fildes * unitOft] & 1) == 1) {
		printf("*** Error: file %s is opened now. ***\n", name);
		return -1;
	}
	firstBlock = meta[offsetDirectory + fildes * unitDirectory + 7];
	// Free the listed data block of the file according to FAT.
	freeDataBlock(firstBlock);
	// free directory entry.
	for (i = 0; i < unitDirectory; i++) {
		meta[offsetDirectory + fildes * unitDirectory + i] = 0;
	}
	printf("*** file %s has been deleted *** \n", name);
	return 0;
}

int fs_read(int fildes, void *buf_, size_t nbyte) {
	int blockNo, nextBlockNo, offsetInBlock, offsetFilePointer, len = 0;
	char blockBuf[BLOCK_SIZE];
	char *buf = (char *) buf_;
	// check the validation of fildes.
	if (fildes < 0 || fildes >= 8) {
		printf("*** Error: invalid fildes %d ***\n", fildes);
		return -1;
	}
	// check whether the file exists or not.
	if (meta[offsetDirectory + fildes * unitDirectory] == 0) {
		printf("*** Error: invalid fildes %d : the file does not exist in file system. ***\n", fildes);
		return -1;
	}
	// check whether the file is opened or not.
	if ((meta[offsetOft + fildes * unitOft] & 1) == 0) {
		printf("*** Error: the file %d is not opened. ***\n", fildes);
		return -1;
	}
	// obtain the current file pointer from OFT
	blockNo = meta[offsetOft + fildes * unitOft] >> 1;
	offsetInBlock = meta[offsetOft + fildes * unitOft + 1];
	offsetFilePointer = meta[offsetOft + fildes * unitOft + 3];
	offsetFilePointer = (offsetFilePointer << 8) + meta[offsetOft + fildes * unitOft + 2];
	// read nbyte data and save them to buf_.
	while (len < nbyte) {
		block_read(blockNo, blockBuf);
		while (offsetInBlock < BLOCK_SIZE && len < nbyte && blockBuf[offsetInBlock] != 0) {
			buf[len] = blockBuf[offsetInBlock];
			len++;
			offsetInBlock++;
			offsetFilePointer++;
		}
		// if reach the end of the file.
		if (offsetInBlock < BLOCK_SIZE && blockBuf[offsetInBlock] == 0) {
			saveOft(fildes, blockNo, offsetInBlock, offsetFilePointer);
			return len;
		} else if (len == nbyte) {
			if (offsetInBlock < BLOCK_SIZE) {
				saveOft(fildes, blockNo, offsetInBlock, offsetFilePointer);
				return len;
			} else {
				nextBlockNo = meta[offsetFat + (blockNo - 32) * unitFat + 1];
				if (nextBlockNo == 0) saveOft(fildes, blockNo, BLOCK_SIZE - 1, offsetFilePointer - 1);
				else saveOft(fildes, nextBlockNo, 0, offsetFilePointer);
				return len;
			}
		} else if (len < nbyte && offsetInBlock == BLOCK_SIZE) {
			nextBlockNo = meta[offsetFat + (blockNo - 32) * unitFat + 1];
			if (nextBlockNo == 0) {
				printf("*** Warning: reach the end of file ***\n");
				saveOft(fildes, blockNo, BLOCK_SIZE - 1, offsetFilePointer - 1);
				return len;
			} else {
				blockNo = nextBlockNo;
				offsetInBlock = 0;
			}
		} else {
			printf("*** Warning: abnormal situation; fs_read ***\n");
			return -1;
		}
	}
	return 0;
}

int fs_write(int fildes, void *buf_, size_t nbyte) {
	int blockNo, nextBlockNo, offsetInBlock, offsetFilePointer, len = 0, fileLength;
	char blockBuf[BLOCK_SIZE];
	char * buf = (char *) buf_;
	// Check the validation of fildes.
	if (fildes < 0 || fildes >= 8) {
		printf("*** Error: invalid fildes %d ***\n", fildes);
		return -1;
	}
	// check whether the file exists or not.
	if (meta[offsetDirectory + fildes * unitDirectory] == 0) {
		printf("*** Error: invalid fildes %d : the file does not exist in file system. ***\n", fildes);
		return -1;
	}
	// check whether the file is opened or not.
	if ((meta[offsetOft + fildes * unitOft] & 1) == 0) {
		printf("*** Error: the file %d is not opened. ***\n", fildes);
		return -1;
	}
	// Obtain the current file pointer from OFT and get file length from directory.
	blockNo = meta[offsetOft + fildes * unitOft] >> 1;
	offsetInBlock = meta[offsetOft + fildes * unitOft + 1];
	offsetFilePointer = meta[offsetOft + fildes * unitOft + 3];
	offsetFilePointer = (offsetFilePointer << 8) + meta[offsetOft + fildes * unitOft + 2];
	fileLength = meta[offsetDirectory + fildes * unitDirectory + 2];
	fileLength = (fileLength << 8) + meta[offsetDirectory + fildes * unitDirectory + 1];
	// write nbyte data to data blocks.
	while (len < nbyte) {
		block_read(blockNo, blockBuf);
		while (offsetInBlock < BLOCK_SIZE && len < nbyte) {
			blockBuf[offsetInBlock] = buf[len];
			len++;
			offsetInBlock++;
			offsetFilePointer++;
		}
		block_write(blockNo, blockBuf);
		if (len == nbyte) {
			if (offsetInBlock < BLOCK_SIZE) {
				saveOft(fildes, blockNo, offsetInBlock, offsetFilePointer);
			} else {
				nextBlockNo = searchFreeBlockInFat();
				if (nextBlockNo != -1) {
					meta[offsetFat + (nextBlockNo - 32) * unitFat] = 1;
					meta[offsetFat + (blockNo - 32) * unitFat + 1] = nextBlockNo;
					saveOft(fildes, nextBlockNo, 0, offsetFilePointer);
				} else {
					meta[offsetFat + (blockNo - 32) * unitFat + 1] = 0;
					saveOft(fildes, blockNo, offsetInBlock - 1, offsetFilePointer - 1);
				}
			}
			if (fileLength < offsetFilePointer) fileLength = offsetFilePointer;
			meta[offsetDirectory + fildes * unitDirectory + 2] = fileLength >> 8;
			meta[offsetDirectory + fildes * unitDirectory + 1] = fileLength & 0xff;
			return len;
		} else {
			nextBlockNo = searchFreeBlockInFat();
			if (nextBlockNo != -1) {
				meta[offsetFat + (nextBlockNo - 32) * unitFat] = 1;
				meta[offsetFat + (blockNo - 32) * unitFat + 1] = nextBlockNo;
				blockNo = nextBlockNo;
				offsetInBlock = 0;
			} else {
				printf("*** Error: no free data block available for writing file ***\n");
				meta[offsetFat + (blockNo - 32) * unitFat + 1] = 0;
				saveOft(fildes, blockNo, offsetInBlock - 1, offsetFilePointer - 1);
				if (fileLength < offsetFilePointer) fileLength = offsetFilePointer;
				meta[offsetDirectory + fildes * unitDirectory + 2] = fileLength >> 8;
				meta[offsetDirectory + fildes * unitDirectory + 1] = fileLength & 0xff;
				return len;
			}
		}
	}
	return 0;
}

int fs_get_filesize(int fildes) {
	int fileLength;
	// Check the validation of fildes.
	if (fildes < 0 || fildes >= 8) {
		printf("*** Error: invalid fildes %d ***\n", fildes);
		return -1;
	}
	// check whether the file exists or not.
	if (meta[offsetDirectory + fildes * unitDirectory] == 0) {
		printf("*** Error: invalid fildes %d : the file does not exist in file system. ***\n", fildes);
		return -1;
	}
	// get file length from directory.
	fileLength = meta[offsetDirectory + fildes * unitDirectory + 2];
	fileLength = (fileLength << 8) + meta[offsetDirectory + fildes * unitDirectory + 1];
	return fileLength;
}

int fs_lseek(int fildes, off_t offset) {
	int blockNo, nextBlockNo, offsetInBlock, offsetFilePointer, fileLength, numBlock;
	// Check the validation of fildes.
	if (fildes < 0 || fildes >= 8) {
		printf("*** Error: invalid fildes %d ***\n", fildes);
		return -1;
	}
	// check whether the file exists or not.
	if (meta[offsetDirectory + fildes * unitDirectory] == 0) {
		printf("*** Error: invalid fildes %d : the file does not exist in file system. ***\n", fildes);
		return -1;
	}
	// check whether the file is opened or not.
	if ((meta[offsetOft + fildes * unitOft] & 1) == 0) {
		printf("*** Error: the file %d is not opened. ***\n", fildes);
		return -1;
	}
	// Obtain the current file pointer from OFT and get file length from directory.
	blockNo = meta[offsetOft + fildes * unitOft] >> 1;
	offsetInBlock = meta[offsetOft + fildes * unitOft + 1];
	offsetFilePointer = meta[offsetOft + fildes * unitOft + 3];
	offsetFilePointer = (offsetFilePointer << 8) + meta[offsetOft + fildes * unitOft + 2];
	fileLength = meta[offsetDirectory + fildes * unitDirectory + 2];
	fileLength = (fileLength << 8) + meta[offsetDirectory + fildes * unitDirectory + 1];
	if (offset == 0) return 0;
	// Check the boundary of file pointers.
	if (offsetFilePointer + offset < 0 || offsetFilePointer + offset > fileLength) {
		printf("*** Error: out of bounds ***\n");
		return -1;
	}
	// Calculate the new position of file pointer.
	offsetFilePointer += offset;
	numBlock = offsetFilePointer / BLOCK_SIZE;
	offsetInBlock = offsetFilePointer % BLOCK_SIZE;
	blockNo = meta[offsetDirectory + fildes * unitDirectory + 7];
	// find the new position as the style of block and offset in block.
	while (numBlock > 0) {
		nextBlockNo = blockNo;
		blockNo = meta[offsetFat + (blockNo - 32) * unitFat + 1];
		numBlock--;
	}
	if (blockNo == 0) saveOft(fildes, nextBlockNo, BLOCK_SIZE - 1, offsetFilePointer - 1);
	// update OFT with new file pointer.
	else saveOft(fildes, blockNo, offsetInBlock, offsetFilePointer);
	return 0;
}

int fs_truncate(int fildes, off_t length) {
	int i, fileLength, blockNo, numBlock, offsetInBlock, nextBlockNo;
	char blockBuf[BLOCK_SIZE];
	// Check the validation of fildes.
	if (fildes < 0 || fildes >= 8) {
		printf("*** Error: invalid fildes %d ***\n", fildes);
		return -1;
	}
	// check whether the file exists or not.
	if (meta[offsetDirectory + fildes * unitDirectory] == 0) {
		printf("*** Error: invalid fildes %d : the file does not exist in file system. ***\n", fildes);
		return -1;
	}
	// check whether the file is opened or not.
	if ((meta[offsetOft + fildes * unitOft] & 1) == 0) {
		printf("*** Error: the file %d is not opened. ***\n", fildes);
		return -1;
	}
	// check the validation of truncate length.
	if (length < 0) {
		printf("*** Error: invalid truncate length. it should be non-negative ***\n");
		return -1;
	}
	// Get file length from directory.
	fileLength = meta[offsetDirectory + fildes * unitDirectory + 2];
	fileLength = (fileLength << 8) + meta[offsetDirectory + fildes * unitDirectory + 1];
	if (length == fileLength) return 0;
	else if (length > fileLength) {  // Check the boundary of truncate.
		printf("*** Error: the truncate length is larger than the length of file ***\n");
		return -1;
	} else {						// Calculate the new file end of the truncated file.
		blockNo = meta[offsetDirectory + fildes * unitDirectory + 7];
		saveOft(fildes, blockNo, 0, 0);
		numBlock = length / BLOCK_SIZE;
		offsetInBlock = length % BLOCK_SIZE;
		while (numBlock > 0) {
			blockNo = meta[offsetFat + (blockNo - 32) * unitFat + 1];
			numBlock--;
		}
		nextBlockNo = meta[offsetFat + (blockNo - 32) * unitFat + 1];
		meta[offsetFat + (blockNo - 32) * unitFat + 1] = 0;
		// Free blocks of the truncated part.
		freeDataBlock(nextBlockNo);
		block_read(blockNo, blockBuf);
		for (i = offsetInBlock; i < BLOCK_SIZE; i++) {
			blockBuf[i] = 0;
		}
		block_write(blockNo, blockBuf);
		return 0;
	}
}

/*	additional functions designed by Chengjun Yuan */
int searchFile(char *fileName) {
	int i, j, len;
	len = strlen(fileName);
	for (i = 0; i < 8; i++) {
		for (j = 0; j < len; j++) {
			if (meta[offsetDirectory + i * unitDirectory + 3 + j] != fileName[j]) break;
		}
		if (j >= len) return i;
	}
	return -1;
}

int searchFreeBlockInFat(void) {
	int i, fatFlag;
	for (i = 0; i < 32; i++) {
		fatFlag = meta[offsetFat + i * unitFat];
		if (fatFlag == 0) return (i + 32);
	}
	printf("*** Error: No available data blocks in file system. ***\n");
	return -1;
}

void freeDataBlock(int numBlock) {
	char buf[BLOCK_SIZE];
	int nextBlock;
	memset(buf, 0, BLOCK_SIZE);
	while (numBlock != 0) {
		block_write(numBlock, buf);
		nextBlock = meta[offsetFat + (numBlock - 32) * unitFat + 1];
		// free the FAT entry.
		meta[offsetFat + (numBlock - 32) * unitFat] = 0;
		meta[offsetFat + (numBlock - 32) * unitFat + 1] = 0;
		numBlock = nextBlock;
	}
}

void saveOft(int fildes, int block, int offsetInBlock, int offsetFilePointer) {
	meta[offsetOft + fildes * unitOft] = (block << 1) | 1;
	meta[offsetOft + fildes * unitOft + 1] = offsetInBlock;
	meta[offsetOft + fildes * unitOft + 2] = offsetFilePointer & 0xff;
	meta[offsetOft + fildes * unitOft + 3] = offsetFilePointer >> 8;
}
