**********************************************************************
PROGRAMMING ASSIGNMENT 3 - RECORD MANAGER

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

1. In the terminal, type in "make -f make_Test1", the makefile script will generate the output "test_assign";
	Type in "./test_assign" to run the test_assign3_1.c test file for testing;

	
2. In the terminal, type in "make -f make_Testexpr", the makefile script will generate the output "test_expr";
	Type in "./test_expr" to run the test_expr.c test file for testing;


==================================
# Description of functions used #
==================================

	Function : SchemaContentWriter
	-----------------------------
		
It Initializes the File by storing the contents of the given schema in the header of the file
	

	Function : createTable
	-----------------------
This function creates a new table with given name. While creating it will check if the file exists, if exists it will throw an error. If a new file created , table info is saved on inital pages. The recordStartPage is set to page 2.


	Function : SchemaContentReader
        --------------------------------

This function retrieves the schema content back into the memory

	Function : openTable
	---------------------
	
This function will open the file with given name. It checks if the file already exists. If not, it throws the error.It initializes the buffer manager with given filename. It then loads the tableInfo and schema into memory.


	Function : createSchema
	------------------------
	
This function will create the schema object and number of attributes, their datatypes and the size is stored.
	

	Function : getSchemaContentSize 
	--------------------------------
	
This function will applies the replacement policy designated in the function parameters.


	Function : getRecordSize
	-------------------------
	
This function will return the size of the record based on the given schema ,that is, datatype of each attribute is considered for this calculation.


	Function : freeSchema
	---------------------
	
This function will frees the memory assigned to data type, attributes names, attribute size, keys size and in the end it will free memory for schema.


	Function : createRecord
	-----------------------
	
This function is used  for memory allocation which takes place for record and record data and for creating a new record. It happens as per the schema.


	Function : freeRecord
	----------------------
	
This function is used to free the memory space allocated to data and record. 


	Function : getAttr
	------------------
	
This function will basically allocates the space to the value data structre where the attribute values is to be fetched. The attrOffset function is used to get the offset value of different attributes as per the attribute numbers. Then attribute data is assigned to value pointer as per different data types.


	Function : setAttr
	------------------
	
This function calls the attrOffset function to get the value of the offset of different attributes as per their attribute numbers. The attribute values will be set with the values provided by the client as per the attributes datatype.


	Function : initRecordManager
	----------------------------
	
This function will initialises the record manager


	Function : shutdownRecordManager
	---------------------------------
	
This function will shutdown the record manager and hence it will free all the resources 


	Function : startScan
	---------------------
	
This function initializes the RM_ScanHandle data structure passed as an argument to it. It will iterate through the records to be searched and the condition to be evaluated.


	Function : next
	----------------
	
This function will starts by fetching a record as per the page and slot id.It checks tombstone id for a deleted record if he bit is set and the record is a deleted one then it checks for the slot number of the record to check if it is the last record. Incase the record is not the last one, the slot number is increased by one to proceed to the next record.The records which are updated ecord id parameters are assigned to the scan mgmtData and next function is called.After verifying the tombstone parameters of the record, the conditions are evaluated to check if the record is the one which is required. If the record fetched is not the required one then next function is called a with the updated record id as its argument.


	Function : closeScan
	---------------------
	
This function will be used to clean all the resources being used in a particular scan.


	Function : destroy
	-------------------
	
This function will destroy the buffer manager.


	Function : closeTable
	----------------------
	
This function will closes the opened file and the buffermanager of a given file. If the file doesn't exist, it throws error and will release all the resources assigned with table.


	Function : deleteTable
	----------------------
	
This function will delete the table file with given name. If the table doesn't exist, it throws error. It deletes the file.


	Function : getNumTuples
	-----------------------
	
his function will return the total number of Tuples in a given table.


	Function : insertRecord
	-----------------------
	
Function will insert a new record if there is any available slot for it.


	Function : deleteRecord
	-----------------------
	
This function will delete the record with given table name and information and RID.


	Function : updateRecord
	------------------------
	
This function will update the Record with given table and record.


	Function : getRecord
	---------------------
	
Function will iterate to all the pages and will return the record with given RID

********************************************************************************************************************************************************


===========================
# Additional Error Codes #
===========================

No Additional Error codes used

******************************************************************************************************************************************************

==================
# Data Structure #
==================

1)

Datastructure to store minimal information for each page of  the table

typedef struct pageheader {
	int max_slot;  // the number of max slots on each page
	int used_slot; // the number of used slots on each page
	char * pntr; // next available free space.
}pageheader;
--------------------------------------------

2)
a scanner manager,  aiding the iteration of tuples

typedef struct Scanner {
	int page_index; //current page index for tuple iteration
	int slot_ID; //current slot index for tuple iteration
    Expr *expr; //search condition
}Scanner;

--------------------------------------------

3)

Table manager storing the Buffer pool and the page list along with the record index within the file. ALso storing the page counts for the table, updated incrementally when new pages are occupied

typedef struct table_manager {
	int page_count;
	int record_start_page_index;
	int record_end_page_index;
	BM_PageHandle *pagelist;
	BM_BufferPool *bm;
}table_manager;
*****************************************************************END OF FILE***************************************************************************