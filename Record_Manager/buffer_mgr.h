#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

// Include return codes and methods for logging errors
#include "dberror.h"

// Include bool DT
#include "dt.h"

// Replacement Strategies
typedef enum ReplacementStrategy {
  RS_FIFO = 0,
  RS_LRU = 1,
  RS_CLOCK = 2,
  RS_LFU = 3,
  RS_LRU_K = 4
} ReplacementStrategy;

// Data Types and Structures
typedef int PageNumber;
#define NO_PAGE -1

typedef struct BM_BufferPool {
  char *pageFile;
  int numPages;
  ReplacementStrategy strategy;
  void *mgmtData; // use this one to store the bookkeeping info your buffer
                  // manager needs for a buffer pool
} BM_BufferPool;

typedef struct BM_PageHandle {
  PageNumber pageNum;
  char *data;
} BM_PageHandle;


//Data Structure to store the Buffer and the Pages in Memory
typedef struct Page_Frame {
	int Pool_ID;
	int page_index; // the position of the page in the pagefile

	struct Page_Frame * next;
	struct Page_Frame * prev;
}Page_Frame;


typedef struct Frame_Desc {
	int * Page_num; // an array of page numbers
	bool * Dirty_Bits; // an array of Dirty bits of the Frame pool
	int * FixCounts;  // an array of FixCounts for the buffer pool
	int  IO_Read;  // the count of ReadIO's on during the process
	int  IO_Write; // the count of WriteIO 's during the process
	char * Page_Data;  // pointer to point to the actual page_frame in the memory pool
	int Pool_count; // the number of empty/available page frames left in the memory pool
    int * Freq; // Array for frequency of reads for each page(For LFU)
	Page_Frame *Head;
	Page_Frame *Tail;
	Page_Frame *Cur;
}Frame_Desc;

// convenience macros
#define MAKE_POOL()					\
  ((BM_BufferPool *) malloc (sizeof(BM_BufferPool)))

#define MAKE_PAGE_HANDLE()				\
  ((BM_PageHandle *) malloc (sizeof(BM_PageHandle)))

//User Defined Macros
#define malloc_Frame_Desc() \
	((Frame_Desc *)malloc (sizeof(Frame_Desc)))
#define malloc_Page_Frame() \
	((Page_Frame *)malloc (sizeof(Page_Frame)))
#define get_head(bm) \
	(((Frame_Desc *)bm->mgmtData)->Head)
#define get_tail(bm) \
	(((Frame_Desc *)bm->mgmtData)->Tail)
// Buffer Manager Interface Pool Handling
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
		  const int numPages, ReplacementStrategy strategy,
		  void *stratData);
RC shutdownBufferPool(BM_BufferPool *const bm);
RC forceFlushPool(BM_BufferPool *const bm);

// Buffer Manager Interface Access Pages
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page);
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page);
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page);
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
	    const PageNumber pageNum);

// Statistics Interface
PageNumber *getFrameContents (BM_BufferPool *const bm);
bool *getDirtyFlags (BM_BufferPool *const bm);
int *getFixCounts (BM_BufferPool *const bm);
int getNumReadIO (BM_BufferPool *const bm);
int getNumWriteIO (BM_BufferPool *const bm);

#endif
