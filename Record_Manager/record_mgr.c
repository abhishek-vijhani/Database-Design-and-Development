#include <stdlib.h>
#include <string.h>
#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "record_mgr.h"
#include "tables.h"
#include "expr.h"

int i,j;

/*
Function: createSchema
Arguments : numAttr,attrNames,dataTypes,typeLength,keySize,keys
Description: To create a new schema with the given arguments
*/

Schema *createSchema(int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys) {
    Schema *CSchema = (Schema *) malloc(sizeof(Schema));
    CSchema->numAttr = numAttr;
    CSchema->attrNames = attrNames;
    CSchema->dataTypes = dataTypes;
    CSchema->typeLength = typeLength;
    CSchema->keySize = keySize;
    CSchema->keyAttrs = keys;
    return CSchema;
}

RC createTable(char *name, Schema *schema){
    SM_FileHandle fh;
	SM_PageHandle ph=(char *) malloc(sizeof(char) * PAGE_SIZE);
    createPageFile(name);
    openPageFile(name, &fh);
	ensureCapacity(50, &fh); //creating ample pages to handle at least 20k records
	SchemaContentWriter(ph,schema);
	//Write schema Content on the first page of the file
	writeBlock(0, &fh, ph);
	closePageFile(&fh);
}
/*
Function: SchemaContentWriter
Arguments:mem,schema
Description: To store the contents of the given schema
*/

void SchemaContentWriter(char *mem, Schema *schema){

	char *String;
	int attrlength;
	memcpy(mem,&(schema->numAttr),sizeof(int));
	mem += sizeof(int);

	for (i=0; i<schema->numAttr; i++){
		attrlength = strlen(schema->attrNames[i]);
		//Adding length of attributes
		memcpy(mem, &attrlength, sizeof(int));
		//move index to the next position
		mem += sizeof(int);
    }
	for (i=0; i<schema->numAttr; i++){
		memcpy(mem, schema->attrNames[i], strlen(schema->attrNames[i]));
		//write Attribute names to the Header string
		String = (char *)malloc(strlen(schema->attrNames[i]));
		strncpy(String, mem, strlen(schema->attrNames[i]));
		mem += strlen(schema->attrNames[i]);
        //Appending Datatype
		memcpy(mem, &(schema->dataTypes[i]),sizeof(DataType));
		mem += sizeof(DataType);
		//Appending Datatype length
		memcpy(mem, &(schema->typeLength[i]),sizeof(int));
		mem += sizeof(int);
		}
	memcpy(mem, &(schema->keySize), sizeof(int));
	mem += sizeof(int);
	for(i = 0; i < schema->keySize; i++){
        //Appending key attributes if any
		memcpy(mem, &(schema->keyAttrs[i]), sizeof(int));
		mem += sizeof(int);
	}

}
/*
Function Name: SchemaContentReader
Arguments :mem, schema
Description: To retrieve schema content back into the memory
*/
void SchemaContentReader(char *mem, Schema *schema){

	int attrlength;
	int keysize;
	memcpy(&(attrlength), mem, sizeof(int));
    //Reading the Schema back from the page in memory
	char **cpNames = (char **) malloc(sizeof(char*) * attrlength);
	DataType *cpDt = (DataType *) malloc(sizeof(DataType) * attrlength);
	int *cpSizes = (int *) malloc(sizeof(int) * attrlength);
	int *nameSizes = (int *) malloc(sizeof(int) * attrlength);
 	mem = mem+sizeof(int);
	for (i=0; i<attrlength; i++){
		memcpy(&j, mem, sizeof(int));
		nameSizes[i] = j;
		mem += sizeof(int);
	}
	for(i = 0; i < attrlength; i++){
      cpNames[i] = (char *) malloc(nameSizes[i]+1);
      strncpy(cpNames[i], mem, nameSizes[i]);
	  mem +=nameSizes[i];
	  memcpy(&cpDt[i],mem, sizeof(DataType));
	  mem = mem+sizeof(DataType);
	  memcpy(&cpSizes[i],mem, sizeof(int));
	  mem = mem+sizeof(int);
    }
	memcpy(&(keysize), mem, sizeof(int));
	int *cpKeys = (int *) malloc(sizeof(int)*keysize);
	mem = mem+sizeof(int);
	for(i=0; i<keysize; i++){
		memcpy(&(cpKeys[i]), mem, sizeof(int));
		mem += sizeof(int);
	}
	/*relate Schema fields to data read from page*/
	schema->numAttr = attrlength;
    schema->attrNames = cpNames;
    schema->dataTypes = cpDt;
    schema->typeLength = cpSizes;
    schema->keyAttrs = cpKeys;
    schema->keySize = keysize;

}

/*
Function :openTable
Arguments:rel,name
Description: To open a table with given name
*/
RC openTable(RM_TableData *rel, char *name){
	BM_BufferPool *bm = ((BM_BufferPool *) malloc (sizeof(BM_BufferPool)));
    initBufferPool(bm, name, 50, RS_FIFO, NULL);
	table_manager *TM = ((table_manager *)malloc (sizeof(table_manager)));
	TM->record_start_page_index = -1;
	TM->record_end_page_index = -1;
	TM->page_count = 0;
	TM->bm = bm;

	SM_FileHandle fh;
	openPageFile(name, &fh);
	TM->page_count = fh.totalNumPages;
	closePageFile(&fh);
	int i, maxslots, usedslots=0, record_size;
	BM_PageHandle *Pagelist = (BM_PageHandle *) malloc(sizeof(BM_PageHandle) * TM->page_count);
	TM->pagelist= Pagelist;
	if (TM->page_count > 1){
		TM->record_start_page_index = 1; // the page number to store schema data is 0
	}

	rel->mgmtData = TM;
	rel->schema = (Schema *)malloc(sizeof(Schema));
	rel->name = name;
	BM_PageHandle *page;

	for (i=0; i< TM->page_count; i++){

		page = &(TM->pagelist[i]);
		pinPage(TM->bm, page, i);
		if(i == 0){
            //Read page header information from the first page
			SchemaContentReader(page->data, rel->schema);
		} else {
			record_size = getRecordSize(rel->schema);
			memcpy(&maxslots,page->data, sizeof(int));
			if(maxslots == 0){
				//New page, without header info
				maxslots=(PAGE_SIZE-sizeof(pageheader))/record_size;
				memcpy(page->data, &maxslots, sizeof(int));
				memcpy(page->data+sizeof(int), &usedslots, sizeof(int)); // write back
			}
		}
	}
	return RC_OK;
}
/*
Function : destroy
Arguments :mgmt
Description: destructor for freeing Table information memory
*/
void destroy(table_manager *mgmt){
	for (i=0; i<mgmt->page_count; i++){
		/*force write into disk */
		unpinPage(mgmt->bm, &(mgmt->pagelist[i]));
		forcePage(mgmt->bm, &(mgmt->pagelist[i]));
	}
	shutdownBufferPool(mgmt->bm);
	free(mgmt->bm);
	free(mgmt->pagelist);
	mgmt->bm = NULL;
	mgmt->pagelist = NULL;
}
/*
Function : closeTable
Arguments :rel
Description: To close the table
*/
RC closeTable(RM_TableData *rel){
	table_manager *mgmt = (table_manager *)rel->mgmtData;
	destroy(mgmt);
	free(rel->mgmtData);
	rel->mgmtData = NULL;
    freeSchema(rel->schema);
	rel->schema = NULL;
    return RC_OK;
}
/*
Function : deleteTable
Arguments :name
Description: To delete the table with given name
*/
RC deleteTable (char *name){
	return destroyPageFile(name);
}
/*
Function:initRecordManager
Arguments mgmtData
Description:Intialize the record Manager
*/

RC initRecordManager(void *mgmtData){
	return RC_OK;
}
/*
Function:shutdownRecordManager
Arguments:
Description:Shut down the record Manager
*/
RC shutdownRecordManager(){
	return RC_OK;
}
/*
Function :getSchemaContentSize
Argument: schema
Description : calculating the Schema structure size of data
*/
int getSchemaContentSize(Schema *schema){
    int Schema_ContentSize=0;
    //Number of Attributes {int}
    Schema_ContentSize+=sizeof(int);
    //Attribute name lengths
	for (i=0; i<schema->numAttr; i++){
		Schema_ContentSize += strlen(schema->attrNames[i]);
	}
	//Schema data type lengths for all attributes
	Schema_ContentSize += sizeof(DataType)*(schema->numAttr);
	//collating the Length of all attributes
	Schema_ContentSize += sizeof(int)*(schema->numAttr);
	//key attribute
    Schema_ContentSize+=sizeof(int);
    //key attribute size
	Schema_ContentSize+= sizeof(int)*(schema->keySize);
	return Schema_ContentSize;

}

/*
Function : getRecordSize
Arguments: schema
Description: Return the size of record from the schema structure
*/
int getRecordSize(Schema *schema){
	int recordsize = 0,i=0;
	for (i=0; i<schema->numAttr; i++){
		switch(schema->dataTypes[i]){
			case DT_INT:
				recordsize += sizeof(int);
				break;
			case DT_STRING:
				recordsize += schema->typeLength[i];// String has a different Typelength
				break;
			case DT_FLOAT:
				recordsize +=  sizeof(float);
				break;
			case DT_BOOL:
				recordsize += sizeof(bool);
				break;
		}
	}
	return recordsize;
}

/*
Function :freeSchema
Arguments : schema
Description: Free the memory allocated to given schema
*/
RC freeSchema(Schema *schema) {
	for (i=0; i<schema->numAttr ; i++)
		free(schema->attrNames[i]);
    free(schema->attrNames);
    free(schema->dataTypes);
    free(schema->typeLength);
    free(schema->keyAttrs);
    free(schema);
    return RC_OK;
}

/*
Function : createRecord
Arguments : record,schema
Description: To create a new Record given the schema
*/
RC createRecord(Record **record, Schema *schema){
	*record = (Record *)malloc(sizeof(Record));
	(*record)->data = (char *)malloc(sizeof(char)*getRecordSize(schema));
	return RC_OK;
}
/*Function : insertRecord
Aguments: rel,record
Description:Inserting a record
*/
RC insertRecord (RM_TableData *rel, Record *record){

	table_manager *mgmt = (table_manager *)rel->mgmtData;
	int max_slots = (PAGE_SIZE-sizeof(pageheader))/getRecordSize(rel->schema);
	int slot_count;
    //page->data contains the starting Memory address of the pool
	BM_PageHandle * page;
	for (i=1; i<mgmt->page_count; i++){
		page = &(mgmt->pagelist[i]);
	    memcpy(&slot_count,page->data+sizeof(int), sizeof(int));
		if((max_slots-slot_count)> 0){
			memcpy(page->data+sizeof(pageheader)+slot_count*getRecordSize(rel->schema), record->data, getRecordSize(rel->schema));
			record->id.page = page->pageNum;
			record->id.slot = slot_count;
			slot_count ++;
        	memcpy(page->data+sizeof(int), &slot_count, sizeof(int));
			return RC_OK;
		}
	}
}
/*
Function:deleteRecord
Arguments:rel,id
Description: To delete the record with given table and rid
*/
RC deleteRecord (RM_TableData *rel, RID id){
	table_manager *mgmt = (table_manager *)rel->mgmtData;
	//page->data contains the starting Memory address of the pool
	BM_PageHandle * page;
	for (i=0; i<mgmt->page_count; i++){
		page = &(mgmt->pagelist[i]);
		if(page->pageNum == id.page)
			memset(page->data+sizeof(pageheader)+id.slot*getRecordSize(rel->schema), '\0', getRecordSize(rel->schema));
	}
	return RC_OK;
}
/*
Function:updateRecord
Arguments:rel,id
Description: To update the record with given table and record
*/
RC updateRecord (RM_TableData *rel, Record *record){
	table_manager *mgmt = (table_manager *)rel->mgmtData;
	RID id = record->id;
	//page->data contains the starting Memory address of the pool
	BM_PageHandle * page;
	for (i=0; i<mgmt->page_count; i++){
		page = &(mgmt->pagelist[i]);
		if(page->pageNum == id.page)
			memcpy(page->data+sizeof(pageheader)+id.slot*getRecordSize(rel->schema), record->data, getRecordSize(rel->schema));
	}
	return RC_OK;
}
/*
Function:getRecord
Argument:rel,id,record
Description: to get the record with given id
*/
RC getRecord (RM_TableData *rel, RID id, Record *record){
	table_manager *mgmt = (table_manager *)rel->mgmtData;
	record->id = id;
	//page->data contains the starting Memory address of the pool
	BM_PageHandle * page;
	for (i=0; i<mgmt->page_count; i++){
		page = &(mgmt->pagelist[i]);
		if(page->pageNum == id.page)
			memcpy(record->data,page->data+sizeof(pageheader)+id.slot*getRecordSize(rel->schema),getRecordSize(rel->schema));
	}
	return RC_OK;
}
/*
Function: freeRecord
Argument: record
Description : To free the allocated memory to given record
*/
RC freeRecord(Record *record){
	free(record->data);
	free(record);
	return RC_OK;
}

/*Function name :getAttr
Argument: record, schema, attrNum,value
Description : get the values of the given records and attributes
*/
RC getAttr(Record *record, Schema *schema, int attrNum, Value **value){
	int intvar, strlength,offset = 0;
	float floatvar; bool boolvar;
	char * String;
	for (i=0; i<schema->numAttr; i++){
		if (i == attrNum){
			switch(schema->dataTypes[i]){
				case DT_STRING:
					strlength = schema->typeLength[i];
					String = (char *)malloc(strlength);
					memcpy(String, (record->data+offset), strlength);
					MAKE_STRING_VALUE(*value,String);
					break;
				case DT_INT:
					memcpy(&intvar,(record->data+offset),sizeof(int));
					MAKE_VALUE(*value, schema->dataTypes[i], intvar);
					break;
				case DT_FLOAT:
					memcpy(&floatvar,(record->data+offset),sizeof(float));
					MAKE_VALUE(*value, schema->dataTypes[i], floatvar);
					break;
				case DT_BOOL:
					memcpy(&boolvar,(record->data+offset),sizeof(bool));
					MAKE_VALUE(*value, schema->dataTypes[i], boolvar);
					break;
			}
		}
		else{
			switch(schema->dataTypes[i]){
			case DT_INT:
				offset += sizeof(int);
				break;
			case DT_STRING:
				offset += schema->typeLength[i];
				break;
			case DT_FLOAT:
				offset +=  sizeof(float);
				break;
			case DT_BOOL:
				offset += sizeof(bool);
				break;
			}
		}
	}
	return RC_OK;
}
/*
Function : setAttr
Arguments:record,schema,attrNum,value
Description: To update the value of a attributes of given schema and record
*/
RC setAttr(Record *record, Schema *schema, int attrNum, Value *value){
	int length,offset = 0;
	for (i=0; i<schema->numAttr; i++){
		if (i == attrNum){
			switch(schema->dataTypes[i]){
				case DT_STRING:
					length = schema->typeLength[i];
					memcpy((record->data+offset),value->v.stringV, length);
					break;
				case DT_INT:
					memcpy((record->data+offset),&(value->v.intV), sizeof(int));
						break;
				case DT_FLOAT:
					memcpy((record->data+offset),&(value->v.floatV), sizeof(float));
					break;
				case DT_BOOL:
					memcpy((record->data+offset),&(value->v.boolV), sizeof(bool));
					break;
			}
		} else {
			switch(schema->dataTypes[i]){
			case DT_INT:
				offset += sizeof(int);
				break;
			case DT_STRING:
				offset += schema->typeLength[i];
				break;
			case DT_FLOAT:
				offset +=  sizeof(float);
				break;
			case DT_BOOL:
				offset += sizeof(bool);
				break;
			}
		}
	}
	return RC_OK;
}
/*
Function:startScan
Arguments:rel,scan,cond
Description:Start the scan
*/
RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond){

	Scanner *SM_Handle = (Scanner *)malloc(sizeof(Scanner));
	SM_Handle->page_index = 1;
	SM_Handle->slot_ID = 0;
	SM_Handle->expr = cond;
	scan->rel = rel;
	scan->mgmtData= SM_Handle;
	return RC_OK;
}
/*
Function:next
Arguments:scan,record
Description:Traversing to the next record
*/
RC next(RM_ScanHandle *scan, Record *record){
	Value *value;
	BM_PageHandle * page;
	int usedslots;
	table_manager *mgmt = (table_manager *)scan->rel->mgmtData;
	Scanner *SM_Handle = (Scanner *)scan->mgmtData;

	record->id.page = SM_Handle->page_index;
	record->id.slot = SM_Handle->slot_ID;

	/* grasp the record info from the table */
	getRecord(scan->rel, record->id, record);
	/* evaluate the record info matches the search condition */
	evalExpr(record, scan->rel->schema, SM_Handle->expr, &value);
	/* get used slots number info for position trace */
	page = &(mgmt->pagelist[SM_Handle->page_index]);
	memcpy(&usedslots, page->data+sizeof(int), sizeof(int));

	if(usedslots == record->id.slot)
	{
		if (SM_Handle->page_index == (mgmt->page_count-1)){
			return RC_RM_NO_MORE_TUPLES;	// running to the end
		}else {
		SM_Handle->page_index++;
		SM_Handle->slot_ID = 0; // switch to the next page
		}
	} else {
		SM_Handle->slot_ID++;
	}
	if (value->v.boolV != 1){
		/* the result is not what we want, continue */
		return next(scan,record);
	} else {

		return RC_OK;
	}
}
/*
Function :closeScan
Arguments:scan
Description: To close the scan operation
*/
RC closeScan (RM_ScanHandle *scan){

	Scanner *SM_Handle = (Scanner *)scan->mgmtData;
	free(SM_Handle);
	scan->mgmtData = NULL;
	return RC_OK;
}
/*
Function : getNumTuples
Arguments :rel
Description: To get the number of tuples
*/
int getNumTuples (RM_TableData *rel){
	table_manager *mgmt = (table_manager *)rel->mgmtData;
	BM_PageHandle * page;
	int N = 0,tuple_count;
	/* First page contains Header, Starting from Page2*/
	for (i=1; i<mgmt->page_count; i++){
		page = &(mgmt->pagelist[i]);
		memcpy(&tuple_count, page->data+sizeof(int), sizeof(int));
		N += tuple_count;
	}
	return N;
}


