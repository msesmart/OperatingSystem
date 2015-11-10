/*
	====================================================
	Function	: Virtual Memory management. 
	Description	: Page faults. TLB. LRU.
	Author		: Chengjun Yuan @UVa  
	Time		: Nov.08.2015              
	====================================================	
*/

#define PAGE_SIZE 256
#define PAGE_ENTRIES 16
#define FRAME_SIZE 256
#define FRAME_ENTRIES 8
#define MEMORY_SIZE (FRAME_SIZE*FRAME_ENTRIES)
#define TLB_ENTRIES 4
#define	DEBUG false
int numAddresses=0;

typedef struct T_L_B{
	unsigned int pageNum[TLB_ENTRIES];
	unsigned int frameNum[TLB_ENTRIES];
	int lastUseTime[TLB_ENTRIES];
	bool inUse[TLB_ENTRIES];
}TLB;

typedef struct PAGE_TABLE{
	unsigned int frameNum[PAGE_ENTRIES];
	bool inUse[PAGE_ENTRIES];
}PageTable;

typedef struct PHYSICAL_MEMORY{
	char memory[FRAME_ENTRIES][FRAME_SIZE];
	unsigned int pageNum[FRAME_ENTRIES];
	int lastUseTime[FRAME_ENTRIES];
	bool inUse[FRAME_ENTRIES];
	int freeFrames;
}PhysicalMemory;

void initiateMemory(PhysicalMemory *physicalMemory, PageTable *pageTable, TLB *tlb);
unsigned int extractPageNum(unsigned int address);
unsigned int extractOffset(unsigned int address);
int searchTLB(TLB *tlb, unsigned int pageNum);
int searchPageTable(PageTable *pageTable, unsigned int pageNum);
void insertTLB(TLB *tlb, unsigned int pageNum, unsigned int frameNum);
void insertPageTable(PageTable *pageTable, unsigned int pageNum, unsigned int frameNum);
void updatePhysicalMemoryLastUseTime(PhysicalMemory *physicalMemory, unsigned int frameNum);
unsigned int allocateFrameNum(PhysicalMemory *physicalMemory, bool *replacement);
void loadPageDataFromBackingStore(PhysicalMemory *physicalMemory, unsigned int pageNum, unsigned int frameNum);
void outputStatisticResults(PhysicalMemory *physicalMemory, PageTable *pageTable, int numTLBHits, int numPageFaults);