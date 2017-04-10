/*
 * storage_mgr.c
 *
 *  Created on: 2016-9-12
 *      Author: Tejas ,Madhav, Mandavi, Abhishek


 *History:::
     2016-9-12      Initialization of storage manager and understanding of the overall assignment
     2016-9-15      Creation of reading method of blocks and getting block data from memory
     2016-9-18      creating of writing methods to blocks
     2016-9-24      New methods for ensure capacity implemented
     2016-9-25      Testing for code against test case 1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage_mgr.h"
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

static int open_fds;
/*Linked list for traversal
 *
 *
 *Newly Added; Post A1 Submission as it was required for Buffer Management
 *
 *
 */
typedef struct SM_Handle {
	SM_FileHandle *fHandle;
	struct SM_Handle *next;
}SM_Handle;
SM_Handle *head = NULL;
SM_Handle *end = NULL;

int init;

FILE *fp;   // variable: Global File pointer

//function : storage manager intialisation function
void initStorageManager(void){
	init= 1;
}

//Function : creates a new page file,
RC createPageFile(char *fileName){
    FILE *file;
	file = fopen(fileName, "w+b");                     //Write a binary file data
	if (file==NULL)
		return RC_FILE_NOT_FOUND;                      //Error while creating a new file
	char * page = (char*) calloc (PAGE_SIZE, sizeof(char));
	if (page == NULL) {
		return RC_MEM_ALLOCATION_FAIL;
	}
	memset(page, '\0', PAGE_SIZE);
	fwrite(page, sizeof(char), PAGE_SIZE,file);
	fclose(file);
	free(page);
	page=NULL;
	return RC_OK;
}

/* Insert fd into list of open fd's
 * Input:
 * @ fHandle : SM_FileHandle data structure
 *
 * Return:
 * RC : Return code of the operation
 */
RC add_to_list (SM_FileHandle *fHandle) {
	SM_Handle *SM = calloc(1, sizeof(SM_Handle));
	SM->fHandle = fHandle;
	SM->next = NULL;
	if (head == NULL) {
		head = SM;
		end = head;
	} else {
		end->next = SM;
		end = SM;
	}
	return RC_OK;
}

/* Delete fd from list of open fd's
 * Input:
 * @ fileName : Name of the file to be deleted
 *
 * Return:
 * RC : Return code of the operation
 */
RC delete_from_list (SM_FileHandle *fHandle) {
	SM_Handle *cur, *prev;
	cur = head;
	prev = head;
	while (cur != NULL) {
		if (cur->fHandle == fHandle) {
			if (cur == head) {
				head = head->next;
			} else if (cur == end) {
				end = prev;
				end->next = NULL;
			} else {
				prev->next = cur->next;
			}
			free(cur); /* Free memory allocation */
		}
		prev = cur;
		cur = cur->next;
	}
	return RC_OK;
}

/* Check if fd is open/valid
 * Input:
 * @ fHandle : SM_FileHandle data structure
 *
 * Return:
 * RC : Return code of the operation
 */
RC check_list (SM_FileHandle *fHandle) {
	SM_Handle *SM;
	SM = head;
	while (SM != NULL && (FILE *)SM->fHandle->mgmtInfo != (FILE *)fHandle->mgmtInfo) {
		SM = SM->next;
	}
	if (SM == NULL) {
		return RC_FILE_HANDLE_NOT_INIT;
	}
	return RC_OK;
}
/* Function : open an existing page file
   parameters----------------
   fileName   :    To Open a file with file name
   fHandle    :    File Handle struct to handle any file operations

*/
RC openPageFile(char *fileName, SM_FileHandle *fHandle){
	long fileLen;                                   //the total length of the file(the total bytes of the file)
    fp=fopen(fileName,"r+b");                       //open any binary file
	if(fp) {
			fseek (fp,0,SEEK_END);
			int fsize = ftell(fp);
			fHandle->fileName=fileName;
			fHandle->totalNumPages=fsize/PAGE_SIZE;
			fHandle->curPagePos=0;
			fHandle->mgmtInfo=(void *)fp;
			open_fds = open_fds + 1;
			add_to_list(fHandle);
			return RC_OK;
		} else {
			return RC_FILE_NOT_FOUND;
		}
}

/*
    function : closePageFile
    parameters
    SM_FileHandle *fHandle : File handle
*/
RC closePageFile(SM_FileHandle *fHandle){
	fclose((FILE *)fHandle->mgmtInfo);
	delete_from_list(fHandle);
	open_fds = open_fds - 1;
	return RC_OK;
}

/*
    function : destroyPageFile

    parameters
    char *fileName : File Name
*/
RC destroyPageFile(char *fileName){
	remove(fileName);                     // Break the link of the file
	return RC_OK;
}


/*
    function : readBlock

    parameters
    SM_FileHandle *fHandle: File Handler
    int pageNum :  Page number to read
    SM_PageHandle memPagev : Page handler to manipulate page data in and out of memory
*/
RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){
    	FILE *fh = (FILE *)fHandle->mgmtInfo;
	if ( pageNum > fHandle->totalNumPages -1 || pageNum < 0) {
		return RC_READ_NON_EXISTING_PAGE;
	}
	fHandle->curPagePos=pageNum;
if (check_list(fHandle) == RC_OK) {
		if (fseek(fh, pageNum*PAGE_SIZE*sizeof(char), SEEK_SET) ==0) {
			fread(memPage, sizeof(char), PAGE_SIZE, fh);
			fHandle->curPagePos = pageNum;
			return RC_OK;
		} else {
			return RC_READ_NON_EXISTING_PAGE;
		}
	} else {
		return RC_FILE_HANDLE_NOT_INIT;
	}
}


/*
    function : getBlockPos

    parameters
    SM_FileHandle *fHandle: File Handler
*/
int getBlockPos (SM_FileHandle *fHandle){
	return fHandle->curPagePos;
}


/*
    function : readFirstBlock

    parameters
    SM_FileHandle *fHandle: File Handler
    SM_PageHandle memPage : Page Handler
*/
RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){

    FILE *fh = (FILE *)fHandle->mgmtInfo;
	int pageNum =0;
	if ( pageNum > fHandle->totalNumPages -1 || pageNum < 0) {
		return RC_READ_NON_EXISTING_PAGE;
	}
if (check_list(fHandle) == RC_OK) {
		if (fseek(fh, pageNum*PAGE_SIZE*sizeof(char), SEEK_SET) ==0) {
			fread(memPage, sizeof(char), PAGE_SIZE, fh);
			fHandle->curPagePos = pageNum;
			return RC_OK;
		} else {
			return RC_READ_NON_EXISTING_PAGE;
		}
	} else {
		return RC_FILE_HANDLE_NOT_INIT;
	}
}


/*
    function : readPreviousBlock

    parameters
    SM_FileHandle *fHandle: File Handler
    SM_PageHandle memPage : Page Handler
*/
RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){

	FILE *fh = (FILE *)fHandle->mgmtInfo;
	int pageNum =fHandle ->curPagePos-1;
	if ( pageNum > fHandle->totalNumPages -1 || pageNum < 0) {
		return RC_READ_NON_EXISTING_PAGE;
	}
if (check_list(fHandle) == RC_OK) {
		if (fseek(fh, pageNum*PAGE_SIZE*sizeof(char), SEEK_SET) ==0) {
			fread(memPage, sizeof(char), PAGE_SIZE, fh);
			fHandle->curPagePos = pageNum;
			return RC_OK;
		} else {
			return RC_READ_NON_EXISTING_PAGE;
		}
	} else {
		return RC_FILE_HANDLE_NOT_INIT;
	}
}

/*
    function : readCurrentBlock

    parameters
    SM_FileHandle *fHandle: File Handler
    SM_PageHandle memPage : Page Handler
*/
RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
	FILE *fh = (FILE *)fHandle->mgmtInfo;
	int pageNum =fHandle ->curPagePos;
	if ( pageNum > fHandle->totalNumPages -1 || pageNum < 0) {
		return RC_READ_NON_EXISTING_PAGE;
	}
if (check_list(fHandle) == RC_OK) {
		if (fseek(fh, pageNum*PAGE_SIZE*sizeof(char), SEEK_SET) ==0) {
			fread(memPage, sizeof(char), PAGE_SIZE, fh);
			fHandle->curPagePos = pageNum;
			return RC_OK;
		} else {
			return RC_READ_NON_EXISTING_PAGE;
		}
	} else {
		return RC_FILE_HANDLE_NOT_INIT;
	}
}


/*
    function : readNextBlock

    parameters
    SM_FileHandle *fHandle: File Handler
    SM_PageHandle memPage : Page Handler
*/
RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){

     	FILE *fh = (FILE *)fHandle->mgmtInfo;
	int pageNum =fHandle ->curPagePos+1;                     //move pointer to point the beginning of next page
	if ( pageNum > fHandle->totalNumPages -1 || pageNum < 0) {
		return RC_READ_NON_EXISTING_PAGE;
	}
if (check_list(fHandle) == RC_OK) {
		if (fseek(fh, pageNum*PAGE_SIZE*sizeof(char), SEEK_SET) ==0) {
			fread(memPage, sizeof(char), PAGE_SIZE, fh);
			fHandle->curPagePos = pageNum;
			return RC_OK;
		} else {
			return RC_READ_NON_EXISTING_PAGE;
		}
	} else {
		return RC_FILE_HANDLE_NOT_INIT;
	}


}
/*
    function : readLastBlock

    parameters
    SM_FileHandle *fHandle: File Handler
    SM_PageHandle memPage : Page Handler
*/
RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
	FILE *fh = (FILE *)fHandle->mgmtInfo;
	int pageNum =fHandle ->totalNumPages -1;                              ////point to the start of last PAGE
	if ( pageNum > fHandle->totalNumPages -1 || pageNum < 0) {
		return RC_READ_NON_EXISTING_PAGE;
	}
if (check_list(fHandle) == RC_OK) {
		if (fseek(fh, pageNum*PAGE_SIZE*sizeof(char), SEEK_SET) ==0) {
			fread(memPage, sizeof(char), PAGE_SIZE, fh);
			fHandle->curPagePos = pageNum;
			return RC_OK;
		} else {
			return RC_READ_NON_EXISTING_PAGE;
		}
	} else {
		return RC_FILE_HANDLE_NOT_INIT;
	}
}
/*
    function : writeBlock

    parameters
    int pageNum : Page number to write to the page number
    SM_FileHandle *fHandle: File Handler
    SM_PageHandle memPage : Page Handler
*/
RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){
FILE *FH = (FILE *)fHandle->mgmtInfo;
	if ( pageNum > fHandle->totalNumPages -1 || pageNum < 0) {
		return RC_READ_NON_EXISTING_PAGE;
	}
	if (fseek(FH, pageNum*PAGE_SIZE*sizeof(char), SEEK_SET) == 0) {//Pointer to the start of the Page

		fwrite(memPage, sizeof(char), PAGE_SIZE, FH);          //Write characters in memPage to File opened in FH
		fHandle->curPagePos = pageNum;                         //Re-assign Current position to Current page
		return RC_OK;
	} else {
		return RC_WRITE_FAILED;
	}
}

/*
    function : writeCurrentBlock

    parameters
    SM_FileHandle *fHandle: File Handler
    SM_PageHandle memPage : Page Handler
*/
RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    int pageNum=fHandle->curPagePos;
	FILE *fh = (FILE *)fHandle->mgmtInfo;
	if ( pageNum > fHandle->totalNumPages -1 || pageNum < 0) {                 //To check dor existing page
		return RC_READ_NON_EXISTING_PAGE;
	}
	if (fseek(fh, pageNum*PAGE_SIZE*sizeof(char), SEEK_SET) == 0) {        //move pointer to point the current page
		fwrite (memPage, sizeof(char), PAGE_SIZE, fh);                      //write content of memPage into block
		fHandle->curPagePos = pageNum;
		return RC_OK;
	} else {
		return RC_WRITE_FAILED;
	}
}

/*
    function : appendEmptyBlock

    parameters
    SM_FileHandle *fHandle: File Handler
*/
RC appendEmptyBlock (SM_FileHandle *fHandle){
		FILE *FH = (FILE *)fHandle->mgmtInfo;
        SM_PageHandle PH = (char*)calloc(PAGE_SIZE, sizeof(char));
        if(PH==NULL)
            return RC_MEM_ALLOCATION_FAIL;
        int Total_Pages=fHandle->totalNumPages;
        memset(PH, '\0', PAGE_SIZE);                                                      //the new last page should be filled with zero bytes
        if (fseek(FH, fHandle->totalNumPages*PAGE_SIZE*sizeof(char), SEEK_SET) ==0) {     //move pointer to point the beginning of the appending block
            fwrite(PH, sizeof(char), PAGE_SIZE, FH);                                      //write new  page and append at the last of the file
            fHandle->totalNumPages ++;
            free(PH);
            PH = NULL;
        }else {
			free(PH);
			PH = NULL;
			return RC_WRITE_FAILED;
		}
}

/*
    function : ensureCapacity

    parameters
    int numberOfPages : Number of pages
    SM_FileHandle *fHandle: File Handler
*/
RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle){
		if (numberOfPages > fHandle->totalNumPages) {
		int counter = numberOfPages - fHandle->totalNumPages;
		while (counter--) {
            //Append page one at a time
			appendEmptyBlock(fHandle);
		}
	}
	return RC_OK;
}


//End of file//




