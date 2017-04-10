**********************************************************************
PROGRAMMING ASSIGNMENT 2 - BUFFER MANAGER

TEAM MEMBERS :
1.Tejas Sreenivasan  (A20377089)  
2.Madhav Mohan Dube (A20376417) 
3.Abhishek Vijhani (A20377670) 
4.Mandavi Verma (A20376197)

CONTENTS :
1)Instructions to Installation
2)Description of functions used
3)Additional Error Codes
4)Data Structures
************************************************************************

==================================
# Instructions for Installation #
==================================

1. In the terminal, type in "make -f make.mk", the makefile script will generate the output "test_out";
2. Type in "./test_out" to run the test_assign2_1.c test file for testing;

==================================
# Description of functions used #
==================================

	Function : init_Frame_Desc	
	-----------------------
		
It Initializes the Buffer Frame by creating the User requested Pools.
	
	Function : destroy_Frame
	--------------------------
	
Deallocates the memory assigned to the Buffer Frame.

	Function : Add_into_List
	---------------------------------------

Add to pool of objects for checking pages in Cache/Memory.

	Function : check_buffer
	--------------------------
	
Search function to check if Page in Memory


	Function : Map_Pageto_Pool
	--------------------------
	
Iterates over the buffer List until the page is found and the Pool index is returned.
	

	Function : FIFO
	--------------------------
	
The FIFO Page replacement is performed.


	Function : LRU_ReOrder
	----------------------------------
	
Modify the order of the Buffer poolby pushing the recently read page to the end of the Queue.

********************************************************************************************************************************************************
===========================
# Additional Error Codes #
===========================

#define RC_MEM_ALLOCATION_FAIL 304
#define RC_PAGE_NOT_FOUND_IN_CACHE 305
#define RC_ALL_PAGE_RESOURCE_OCCUPIED 306
#define RC_FAIL_SHUTDOWN_POOL 307
#define RC_FAIL_FORCE_PAGE 308


******************************************************************************************************************************************************

==================
# Data Structure #
==================

1)We defined the data structure which stores required information on Pages in memory, counts or Reads and Writes, Pool size and content data on pages in memory.
The Instance of this object is loaded with the mgmtData object of the bufferpool Object.
----------------------------------------
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

--------------------------------------------
2)We make us of a linked list to organize the cached-pages in the buffer pool. This facilitates replacement of pages for various policies which will be tested in the test cases.
------------------------------------------
typedef struct Page_Frame {
	int Pool_ID;
	int page_index; // the position of the page in the pagefile 

	struct Page_Frame * next;
	struct Page_Frame * prev;
}Page_Frame;
---------------------------------------------

*****************************************************************END OF FILE***************************************************************************