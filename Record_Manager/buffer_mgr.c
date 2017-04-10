#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "storage_mgr.h"
#include "buffer_mgr.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

/*
    function : initBufferPool
    parameters:
        BM_BufferPool *const bm, const char *const pageFileName,
        const int numPages, ReplacementStrategy strategy,
        void *stratData)

*/
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,const int numPages, ReplacementStrategy strategy,void *stratData)
{
	bm->pageFile = (char *)pageFileName;
	bm->numPages = numPages;
	bm->strategy = strategy;
	bm->mgmtData = malloc_Frame_Desc();
	init_Frame_Desc(bm->mgmtData,bm->numPages);
	Frame_Desc *mgmtData = (Frame_Desc *)bm->mgmtData;
	return RC_OK;
}
/*
    function : init_Frame_Desc
    parameters:
        Frame_Desc *Frame, int init_size
*/
void init_Frame_Desc (Frame_Desc *Frame, int init_size)
{
	Frame->Page_num = (int *)malloc(init_size*sizeof(int));
	Frame->Dirty_Bits = (bool *)malloc (init_size*sizeof(bool));
	Frame->FixCounts = (int *)malloc(init_size*sizeof(int));
	Frame->IO_Read = 0;
	Frame->IO_Write = 0;
	Frame->Page_Data = (char *)malloc(init_size*PAGE_SIZE*sizeof(char));
	Frame->Pool_count = init_size;
	Frame->Head = NULL;
	Frame->Tail = NULL;
	Frame->Cur = NULL;
	int i;
	for (i=0; i< init_size; i++){
		*(Frame->Page_num + i) = NO_PAGE;
		*(Frame->Dirty_Bits + i) = false;
		*(Frame->FixCounts + i) = 0;
	}
}

/*
    function : destroy_Frame_Desc
    parameters:
        Frame_Desc * Frame
*/
void destroy_Frame(Frame_Desc * Frame){

    //free all the allocated memory
	free(Frame->Page_num);
	free(Frame->Dirty_Bits);
	free(Frame->FixCounts);
	free(Frame->Page_Data);
	Frame->Page_num = NULL;
	Frame->Dirty_Bits = NULL;
	Frame->FixCounts = NULL;
	Frame->IO_Read = 0;
	Frame->IO_Write = 0;
	Frame->Page_Data = NULL;
	Frame->Head = NULL;
	Frame->Tail = NULL;
	Frame->Cur = NULL;
}

/*
    function : pinPage
    parameters:
        BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum
*/
RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum)
{
    Frame_Desc *Frame=malloc_Frame_Desc();
    Frame=bm->mgmtData;

	/* initialize Storage manager*/
	SM_FileHandle F_Handle;

	openPageFile (bm->pageFile, &F_Handle);

	/* Check the SM Size*/
	if (F_Handle.totalNumPages < (pageNum+1)){
		ensureCapacity(pageNum+1,&F_Handle);
	}
	page->pageNum = pageNum;
    //Check if Page already in buffer, [if yes, Add the page at the end of the queue, this is Re-ordering for LRU replaces previous position of the page in buffer to the last]
	if (check_buffer(bm,pageNum)==0){
        if (bm->strategy == RS_LRU){
			LRU_ReOrder(bm,pageNum);
		}
		int Pool_ID = Map_Pageto_Pool(bm,pageNum);
		(*(Frame->FixCounts+Pool_ID))++;
		page->data = (Frame->Page_Data)+Pool_ID*PAGE_SIZE*sizeof(char);
		return RC_OK;

	}
	else{//if page not in buffer, add it to the the pool
		if (Frame->Pool_count>0){
			int Pool_ID = bm->numPages - Frame->Pool_count;
			Add_into_List(bm,pageNum, Pool_ID);
			Frame->Pool_count--;
			(*(Frame->FixCounts+Pool_ID))++;
			page->data = (Frame->Page_Data)+Pool_ID*PAGE_SIZE*sizeof(char);

		} else {
			int Pool_ID = FIFO(bm,pageNum, F_Handle);
			(*(Frame->FixCounts+Pool_ID))++;
			page->data = (Frame->Page_Data)+Pool_ID*PAGE_SIZE*sizeof(char);

		}
		Frame->IO_Read++;
		readBlock(page->pageNum, &F_Handle, page->data);
	}
	closePageFile(&F_Handle);
	return RC_OK;
}
/*
    function : unpinPage
    parameters:
        BM_BufferPool *const bm, BM_PageHandle *const page
*/
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	int Pool_ID = Map_Pageto_Pool(bm,page->pageNum);
	(*(((Frame_Desc *)bm->mgmtData)->FixCounts+Pool_ID))--;
	return RC_OK;
}
/*
    function : Add_into_List
    parameters:
        int page_index, int pool_index, BM_BufferPool *const bm
*/
void Add_into_List(BM_BufferPool *const bm,int page_index, int Pool_ID){
    Frame_Desc *Frame=malloc_Frame_Desc();
    Frame=bm->mgmtData;
	Page_Frame *page_handler = malloc_Page_Frame();
	page_handler->page_index = page_index;
	page_handler->Pool_ID = Pool_ID;
	page_handler->next = NULL;
	if(Frame->Head == NULL){
		get_head(bm) = page_handler;
		get_tail(bm) = page_handler;
		page_handler->prev = NULL;
		page_handler->next = NULL;
	} else {
		get_tail(bm)->next = page_handler;
		page_handler->prev = get_tail(bm);
		get_tail(bm) = page_handler;
	}
	*(Frame->Page_num+Pool_ID) = page_index;
}

/*
    function Name: check_buffer
    Parameters:
        BM_BufferPool *const bm,int page_index
*/
RC check_buffer(BM_BufferPool *const bm,int page_index)
{
	Page_Frame *List_Tranversal = NULL;
	List_Tranversal = get_head(bm);
	while (List_Tranversal!=NULL){
		if (List_Tranversal->page_index == page_index){
			return RC_OK;
		}
		List_Tranversal = List_Tranversal->next;
	}
	return RC_PAGE_NOT_FOUND_IN_CACHE;
}


/*
    function : pageindex_mapto_poolindex
    Parameters:
        BM_BufferPool *const bm,int page_index
*/
int Map_Pageto_Pool(BM_BufferPool *const bm,int page_index)
{
	Page_Frame *List_Tranversal = NULL;
	List_Tranversal = get_head(bm);
	while (List_Tranversal!=NULL){
		if (List_Tranversal->page_index == page_index){
			return List_Tranversal->Pool_ID;
		}
		List_Tranversal = List_Tranversal->next;
	}
	return RC_PAGE_NOT_FOUND_IN_CACHE;
}
/*
    function : FIFO
    parameters:
         BM_BufferPool *const bm,int pageNum, SM_FileHandle fHandle
*/
int FIFO( BM_BufferPool *const bm,int pageNum, SM_FileHandle fHandle)
{
    Frame_Desc *Frame=malloc_Frame_Desc();
    Frame=bm->mgmtData;
    Page_Frame * head_page = get_head(bm);

    while (head_page != NULL && *(Frame->FixCounts+head_page->Pool_ID)>0){
        head_page = head_page->next;
    }
    if (head_page == NULL) {
        return RC_ALL_PAGE_RESOURCE_OCCUPIED;
    } else {
        if(head_page != get_tail(bm)){
            get_tail(bm)->next = head_page;

            head_page->next->prev = head_page->prev;
            if(head_page == get_head(bm)){
                get_head(bm) = get_head(bm)->next;
            } else {
                head_page->prev->next = head_page->next;
            }
            head_page->prev = get_tail(bm);
            get_tail(bm) = get_tail(bm)->next;
            get_tail(bm)->next = NULL;
        }

        if(*(Frame->Dirty_Bits+get_tail(bm)->Pool_ID)== true){
            char * memory = Frame->Page_Data + get_tail(bm)->Pool_ID*PAGE_SIZE*sizeof(char);
            int old_pageNum = get_tail(bm)->page_index;
            writeBlock(old_pageNum, &fHandle, memory);
            *(Frame->Dirty_Bits+get_tail(bm)->Pool_ID)= false;
            Frame->IO_Write++;
        }
        get_tail(bm)->page_index = pageNum;
        *(Frame->Page_num+get_tail(bm)->Pool_ID) = pageNum;
        return get_tail(bm)->Pool_ID;
    }
	return -1;
}
/*
    function : LRU_ReOrder
    parameters:
        BM_BufferPool *const bmint page_index
*/

RC LRU_ReOrder(BM_BufferPool *const bm,int page_index)
{
	Page_Frame *List_Tranversal = NULL;
	Page_Frame *tail = NULL;
	List_Tranversal = get_head(bm);
	tail = get_tail(bm);
	while (List_Tranversal != NULL){
		if (List_Tranversal->page_index == page_index){
			break;
		}
		List_Tranversal = List_Tranversal->next;
	}
	if (List_Tranversal != tail){
		get_tail(bm)->next = List_Tranversal;
		List_Tranversal->next->prev = List_Tranversal->prev;
		if(List_Tranversal == get_head(bm)){
			get_head(bm) = get_head(bm)->next;
		} else {
			List_Tranversal->prev->next = List_Tranversal->next;
		}
		List_Tranversal->prev = get_tail(bm);
		get_tail(bm) = get_tail(bm)->next;
		get_tail(bm)->next = NULL;
	}

}
/*
    function : shutdownBufferPool
    parameters:
        BM_BufferPool *const bm
*/
RC shutdownBufferPool(BM_BufferPool *const bm)
{
	Frame_Desc *Frame = (Frame_Desc *)bm->mgmtData;
	int i;
	for (i=0; i< bm->numPages; i++){
	  if (*(Frame->FixCounts+i)> 0) {
	  	return RC_FAIL_SHUTDOWN_POOL;
	  }
	 }
	forceFlushPool(bm);
	destroy_Frame(bm->mgmtData);
	free(bm->mgmtData);
	bm->mgmtData = NULL;
	return RC_OK;
}
/*
    function : markDirty
    parameters:
        BM_BufferPool *const bm, BM_PageHandle *const page

*/
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    Frame_Desc *Frame=malloc_Frame_Desc();
    Frame=bm->mgmtData;
	int Pool_ID = Map_Pageto_Pool(bm,page->pageNum);
	*(Frame->Dirty_Bits+Pool_ID) = true;
	return RC_OK;
}
/*
    function : forcePage
    parameters:
        BM_BufferPool *const bm, BM_PageHandle *const page
*/
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	/* open the page file in the disk first */
    Frame_Desc *Frame=malloc_Frame_Desc();
    Frame=bm->mgmtData;
	int pageNum = page->pageNum;
	int Pool_ID = Map_Pageto_Pool(bm,pageNum);
	if (*(Frame->FixCounts+Pool_ID)> 0){
		return RC_FAIL_FORCE_PAGE;
	} else {
		SM_FileHandle F_Handle;
		openPageFile (bm->pageFile, &F_Handle);
		char * memory = page->data;
		writeBlock(pageNum, &F_Handle, memory);
		*(Frame->Dirty_Bits+Pool_ID)= false; /* set dirty flag to false after flashing into disk */
		Frame->IO_Write++;
		closePageFile(&F_Handle);
		return RC_OK;
	}
}
/*
    function : forceFlushPool
    parameters:
        BM_BufferPool *const bm
*/
RC forceFlushPool(BM_BufferPool *const bm)
{
	/* open the page file in the disk first */
    Frame_Desc *Frame=malloc_Frame_Desc();
    Frame=bm->mgmtData;
	SM_FileHandle F_Handle;
	char * memory;
	int page_index;
	openPageFile (bm->pageFile, &F_Handle);
	int i;
	for (i=0; i< bm->numPages; i++){
		if (*(Frame->Dirty_Bits+i)== true) {
			page_index = *(Frame->Page_num+i);
			memory = Frame->Page_Data + i*PAGE_SIZE*sizeof(char);
			writeBlock(page_index, &F_Handle, memory);
			*(Frame->Dirty_Bits+i)= false; /* set dirty flag to false after flashing into disk */
			Frame->IO_Write++;
		}
	}
	closePageFile(&F_Handle);
	return RC_OK;
}
// Statistics Interface
/*
    function : getFrameContents
    parameters:
        BM_BufferPool *const bm
*/
PageNumber *getFrameContents (BM_BufferPool *const bm)
{
    Frame_Desc *Frame=malloc_Frame_Desc();
    Frame=bm->mgmtData;
	return Frame->Page_num;
}
/*
    function : getDirtyFlags
    parameters:
        BM_BufferPool *const bm
*/
bool *getDirtyFlags (BM_BufferPool *const bm)
{
    Frame_Desc *Frame=malloc_Frame_Desc();
    Frame=bm->mgmtData;
	return Frame->Dirty_Bits;
}
/*
    function : getFixCounts
    parameters:
        BM_BufferPool *const bm

*/
int *getFixCounts (BM_BufferPool *const bm)
{
    Frame_Desc *Frame=malloc_Frame_Desc();
    Frame=bm->mgmtData;
	return Frame->FixCounts;
}
/*
    function : getIO_Read
    parameters:
        BM_BufferPool *const bm

*/
int getNumReadIO (BM_BufferPool *const bm)
{
    Frame_Desc *Frame=malloc_Frame_Desc();
    Frame=bm->mgmtData;
	return Frame->IO_Read;
}
/*
    function : getIO_Write
    parameters:
        BM_BufferPool *const bm
*/
int getNumWriteIO (BM_BufferPool *const bm)
{
    Frame_Desc *Frame=malloc_Frame_Desc();
    Frame=bm->mgmtData;
	return Frame->IO_Write;
}
