/*
	====================================================
	Function	: Virtual Memory management. 
	Description	: Page faults. TLB. LRU.
	Author		: Chengjun Yuan @UVa  
	Time		: Nov.08.2015              
	====================================================	
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "p5cyuan.h"

int main(int argc,char *argv[]){
	FILE *fp,*fpWrite; int numTLBHits=0,numPageFaults=0;
	unsigned int address,pageNum,frameNum,offset,replacedPageNum,physicalAddress;
	bool replacement=false;
	PhysicalMemory physicalMemory;
	TLB tlb;
	PageTable pageTable;
	
	initiateMemory(&physicalMemory,&pageTable,&tlb);
	fp=fopen("addresses.txt","r");
	fpWrite=fopen("result.txt","w");
	while((!feof(fp))&&numAddresses<1000){
		fscanf(fp,"%u\n",&address);
		pageNum=extractPageNum(address);
		offset=extractOffset(address);
		if(DEBUG)printf("read virtual address %u pageNum %u offset %u \n",address,pageNum,offset);
		frameNum=searchTLB(&tlb,pageNum);
		if(frameNum==-1){
			frameNum=searchPageTable(&pageTable,pageNum);
			if(frameNum==-1){
				numPageFaults++;
				printf("virtual address %u contained in page %u causes a page fault \n",address,pageNum);
				frameNum=allocateFrameNum(&physicalMemory,&replacement);
				printf("page %u is loaded into frame %u \n",pageNum,frameNum);
				if(replacement){
					replacedPageNum=physicalMemory.pageNum[frameNum];
					pageTable.inUse[replacedPageNum]=false;
				}
				physicalMemory.pageNum[frameNum]=pageNum;
				insertPageTable(&pageTable,pageNum,frameNum);
				loadPageDataFromBackingStore(&physicalMemory,pageNum,frameNum);
			}
			updatePhysicalMemoryLastUseTime(&physicalMemory,frameNum);
			insertTLB(&tlb,pageNum,frameNum);
		}else{
			updatePhysicalMemoryLastUseTime(&physicalMemory,frameNum);
			numTLBHits++;
		}
		fprintf(fpWrite,"Virtual address: %u Physical address: %u Value: %d \n",address,((frameNum<<8)|offset),physicalMemory.memory[frameNum][offset]);
		numAddresses++;
	}
	fclose(fpWrite);
	fclose(fp);
	outputStatisticResults(&physicalMemory,&pageTable,numTLBHits,numPageFaults);
}


void initiateMemory(PhysicalMemory *physicalMemory, PageTable *pageTable, TLB *tlb){
	int i;
	for(i=0;i<FRAME_ENTRIES;i++){physicalMemory->inUse[i]=false; physicalMemory->lastUseTime[i]=0;}
	physicalMemory->freeFrames=FRAME_ENTRIES;
	for(i=0;i<PAGE_ENTRIES;i++)pageTable->inUse[i]=false;
	for(i=0;i<TLB_ENTRIES;i++){tlb->inUse[i]=false; tlb->lastUseTime[i]=0;}
}

unsigned int extractPageNum(unsigned int address){
	return ((address&0xffff)>>8);
}

unsigned int extractOffset(unsigned int address){
	return (address&0x00ff);
}

int searchTLB(TLB *tlb, unsigned int pageNum){
	int i;
	for(i=0;i<TLB_ENTRIES;i++){
		if(tlb->inUse[i]&&tlb->pageNum[i]==pageNum){
			tlb->lastUseTime[i]=numAddresses;
			printf("page %u is stored in frame %u which is stored in entry %d of the TLB \n",pageNum,tlb->frameNum[i],i);
			return tlb->frameNum[i];
		}
	}
	printf("frame number for page %u is missing in the TLB \n",pageNum); return -1;
}

int searchPageTable(PageTable *pageTable, unsigned int pageNum){
	if(pageTable->inUse[pageNum]){
		printf("page %u is contained in frame %u \n",pageNum,pageTable->frameNum[pageNum]);
		return pageTable->frameNum[pageNum];
	}else return -1;
}

void insertTLB(TLB *tlb, unsigned int pageNum, unsigned int frameNum){
	int i,longestUseTime=0,longestUseTLB;
	// first fit placement algorithm.
	for(i=0;i<TLB_ENTRIES;i++){
		if(!(tlb->inUse[i])){
			tlb->inUse[i]=true;
			tlb->pageNum[i]=pageNum;
			tlb->frameNum[i]=frameNum;
			tlb->lastUseTime[i]=numAddresses;
			printf("frame %u containing page %u is stored in entry %d of the TLB \n",frameNum,pageNum,i);
			return;
		}
	}
	// LRU replacement algorithm.
	for(i=0;i<TLB_ENTRIES;i++){
		if(numAddresses-tlb->lastUseTime[i]>longestUseTime){
			longestUseTime=numAddresses-tlb->lastUseTime[i];
			longestUseTLB=i;
		}
	}
	tlb->pageNum[longestUseTLB]=pageNum;
	tlb->frameNum[longestUseTLB]=frameNum;
	tlb->lastUseTime[longestUseTLB]=numAddresses;
	printf("frame %u containing page %u is stored in entry %d of the TLB \n",frameNum,pageNum,longestUseTLB);
	return;
}

void insertPageTable(PageTable *pageTable, unsigned int pageNum, unsigned int frameNum){
	pageTable->frameNum[pageNum]=frameNum;
	pageTable->inUse[pageNum]=true;
}

void updatePhysicalMemoryLastUseTime(PhysicalMemory *physicalMemory, unsigned int frameNum){
	physicalMemory->lastUseTime[frameNum]=numAddresses;
}

unsigned int allocateFrameNum(PhysicalMemory *physicalMemory,bool *replacement){
	int i,longestUseTime=0,longestUseFrame=0;
	if(physicalMemory->freeFrames>0){
		*replacement=false;
		for(i=0;i<FRAME_ENTRIES;i++){
			if(!physicalMemory->inUse[i]){
				physicalMemory->lastUseTime[i]=numAddresses;
				physicalMemory->inUse[i]=true;
				physicalMemory->freeFrames--;
				return i;
			}
		}
	}else{ // LRU replacement.
		*replacement=true;
		for(i=0;i<FRAME_ENTRIES;i++){
			if(numAddresses-physicalMemory->lastUseTime[i]>longestUseTime){
				longestUseTime=numAddresses-physicalMemory->lastUseTime[i];
				longestUseFrame=i;
			}
		}
		physicalMemory->lastUseTime[longestUseFrame]=numAddresses;
		physicalMemory->inUse[longestUseFrame]=true;
		return longestUseFrame;
	}
}

void loadPageDataFromBackingStore(PhysicalMemory *physicalMemory, unsigned int pageNum, unsigned int frameNum){
	FILE *fp; char buffer[PAGE_SIZE]; int i;
	fp=fopen("BACKING_STORE.bin","r");
	fseek(fp,pageNum*PAGE_SIZE,SEEK_SET);
	fread(buffer,1,FRAME_SIZE,fp);
	for(i=0;i<FRAME_SIZE;i++){
		physicalMemory->memory[frameNum][i]=buffer[i];
	}
	fclose(fp);
}

void outputStatisticResults(PhysicalMemory *physicalMemory, PageTable *pageTable, int numTLBHits, int numPageFaults){
	int i; int numUsedPages=0,numUsedFrames=0;
	printf("Contents of page table: \n");
	for(i=0;i<PAGE_ENTRIES;i++){
		if(pageTable->inUse[i]){printf("page %d: frame %u, ",i,pageTable->frameNum[i]); numUsedPages++;}
		else printf("page %d: not in memory, ",i);
	}
	printf("\nContents of page frames: \n");
	for(i=0;i<FRAME_ENTRIES;i++){
		if(physicalMemory->inUse[i]){printf("frame %d: page %u, ",i,physicalMemory->pageNum[i]); numUsedFrames++;}
		else printf("frame %d: empty, ",i);
	}
	printf("\n %d page faults out of %d references \n %d TLB hits out of %d references \n",numPageFaults,numAddresses,numTLBHits,numAddresses);
	//printf("numUsedPages %d numUsedFrames %d \n",numUsedPages,numUsedFrames);
}