#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "BF.h"        
#include "heap.h"                                        

/* Creates a heap file */
int HP_CreateFile(char *fileName){
	int fileDesc, blockNo = 0;
	char heap_characteristic[] = "HEAP";
	void* block;

	/* Creation of a heap file */
	if (BF_CreateFile(fileName)<0){ 
		BF_PrintError("Error creating File");
		return -1;
	}

	/* Opening heap file */
	if ( (fileDesc=BF_OpenFile(fileName)) < 0){ 
		BF_PrintError("Error Opening File");
		return -1;
	}

	/* Allocation of 1st block */
	if (BF_AllocateBlock(fileDesc)<0){
		BF_PrintError("Error Allocating First Block");
		return -1;
	}

	/* Reading from 1st block */
	if (BF_ReadBlock(fileDesc, 0, &block) < 0){ 
		BF_PrintError("Error reading block");
		return -1;
	}

	/* Writing characteristic of heap file */
	memcpy(block, heap_characteristic, sizeof(CHARACTERISTIC_SIZE)); 
	/* Writing the total of blocks with records (currently 0) */
	memcpy(block + sizeof(CHARACTERISTIC_SIZE), &blockNo, sizeof(int)); 
	
	if (BF_WriteBlock(fileDesc,0) < 0)	{
		BF_PrintError("Error writing block back");
		return -1;
	}
	
	if (BF_CloseFile(fileDesc)){
		BF_PrintError("Error Closing File");
		return -1;
	}

	printf("\nHeap File Created!\n");

	return 0;
}

/* Opens a heap file */
int HP_OpenFile(char*fileName){
	int fileDesc;
	char *heap_check;
	void *block;

	if ((fileDesc = BF_OpenFile(fileName)) < 0){
		BF_PrintError("Error Opening File");
		return -1;
	}
	if (BF_ReadBlock(fileDesc, 0, &block) < 0){
		BF_PrintError("Error reading block");
		return -1;
	}	
	heap_check = (char*) malloc (CHARACTERISTIC_SIZE);

	/* Checking if the file is a heap file */
	memcpy(heap_check,block,CHARACTERISTIC_SIZE);

	/* If not a heap file, return -1 */
	if(strcmp(heap_check, "HEAP") != 0){ 
		if (BF_CloseFile(fileDesc)){
			BF_PrintError("Error Closing File");
			return -1;
		}
		printf("File is not a Heap File!\n");
		return -1;
	}
	printf("Heap File Opened!\n");
	free(heap_check);
	return fileDesc;
}

/* Closes a heap file */
int HP_CloseFile(int fileDesc){
	if (BF_CloseFile(fileDesc) < 0){
		BF_PrintError("Error Closing File");
		return -1;
	}
	printf("\nHeap File Closed!\n\n");
	return 0;
}

/* Inserts a new record to the heap file */
int HP_InsertEntry(int fileDesc, Record record){
	int BlockNo;
	short block_records;
	void *block;
	
	/*Reading from the 1st block of the file */
	if (BF_ReadBlock(fileDesc, 0, &block) < 0){
		BF_PrintError("Error reading block");
		return -1;
	}

	/* Reading the total of blocks with records */
	memcpy(&BlockNo,block+sizeof(CHARACTERISTIC_SIZE),sizeof(int)); 

	/* If no blocks with records */
	if( BlockNo == 0 ){ 
		if ( BF_AllocateBlock(fileDesc) < 0 ){
			BF_PrintError("Error Allocating First Block");
			return -1;
		}

		/* Increment the total of blocks with records */
		BlockNo++; 

		/* Writing to the 1st block the total of record blocks */
		memcpy(block+sizeof(char[4]), &BlockNo,sizeof(int)); 
		if (BF_WriteBlock(fileDesc, 0) < 0)	{
			BF_PrintError("Error writing block back");
			return -1;
		}
		
		if (BF_ReadBlock(fileDesc, BlockNo, &block) < 0){
			BF_PrintError("Error reading block");
			return -1;
		}

		block_records = 1;
		
		/* Writing the total of record blocks */
		memcpy(block, &block_records,sizeof(short)); 

		/* Adding record */
		memcpy(block+sizeof(short), &record,sizeof(Record)); 
		if (BF_WriteBlock(fileDesc, BlockNo) < 0)	{
			BF_PrintError("Error writing block back");
			return -1;
		}
		
	} else {
		if (BF_ReadBlock(fileDesc, BlockNo, &block) < 0){
			BF_PrintError("Error reading block");
			return -1;
		}
		memcpy(&block_records, block, sizeof(short));
		
		/* If we have the max number of records in a block, allocate a new one */
		if(block_records == MAX_BLOCK_RECORDS){ 
			if (BF_ReadBlock(fileDesc, 0, &block) < 0){
				BF_PrintError("Error reading block");
				return -1;
			}
			
			if ( BF_AllocateBlock(fileDesc) < 0 ){
				BF_PrintError("Error Allocating First Block");
				return -1;
			}
			
			/* Increment the number of record blocks */
			BlockNo++; 

			/* Writing to the 1st block the total of record blocks */
			memcpy(block+sizeof(char[4]), &BlockNo,sizeof(int)); 
			if (BF_WriteBlock(fileDesc, 0) < 0)	{
				BF_PrintError("Error writing block back");
				return -1;
			}
			
			if (BF_ReadBlock(fileDesc, BlockNo, &block) < 0){
				BF_PrintError("Error reading block");
				return -1;
			}
			block_records = 1;
			/* Writing the total of records in the current block */
			memcpy(block, &block_records,sizeof(short)); 
			/* Adding record */
			memcpy(block+sizeof(short), &record,sizeof(Record)); 
			if (BF_WriteBlock(fileDesc, BlockNo) < 0)	{
				BF_PrintError("Error writing block back");
				return -1;
			}
		} else{
			memcpy(block+sizeof(short)+sizeof(Record)*block_records, &record,sizeof(Record));
			block_records++;
			/* Incrementing the number of records in the block */
			memcpy(block, &block_records,sizeof(short)); 
			if (BF_WriteBlock(fileDesc, BlockNo) < 0){
				BF_PrintError("Error writing block back");
				return -1;
			}
		}
	}
	return 0;
}

/* Printing all entries, or searching a printing according to fieldName search */
void HP_GetAllEntries(int fileDesc, char* fieldName, void *value){
	int BlockNo, i, j, fieldName_flag, records_found = 0;
	short block_records;
	void *block;
	size_t offset, compare_size;
	Record record;
	
	if (BF_ReadBlock(fileDesc, 0, &block) < 0){
		BF_PrintError("Error reading block");
		return;
	}	
	/* Reading the total of blocks with records */
	memcpy(&BlockNo,block+sizeof(CHARACTERISTIC_SIZE),sizeof(int)); 
	printf("\nSearch and Print...\n");
	if(BlockNo != 0){
		if(strcmp(fieldName, "all") == 0){
			for(i = 1; i <= BlockNo; i++){
				if (BF_ReadBlock(fileDesc, i, &block) < 0){
					BF_PrintError("Error reading block");
					return;
				}	
				printf("-------------------\nReading Block %d\n-------------------\n", i);
				memcpy(&block_records, block, sizeof(short));
				for(j = 0; j < block_records; j++){
					records_found++;
					memcpy(&record, block+sizeof(short)+sizeof(Record)*j, sizeof(Record));
					printf("id: %d \t Name: %s \t Surname: %s \t City: %s \n", record.id, record.name, record.surname, record.city);
				}
			}
			printf("---------------------\nBlocks Read: %d!\nRecords Found: %d\n---------------------\n", i-1, records_found);
			
		} else {
			fieldName_flag = 0;
			
			if (strncmp(fieldName, "id", 3) == 0){
				offset = 0;
				fieldName_flag = 1;
			}
			else if (strncmp(fieldName, "name", 5) == 0){
				offset = sizeof(int);
				compare_size = sizeof(char)*15;
				fieldName_flag = 2;
			}
			else if (strncmp(fieldName, "surname", 8) == 0){
				offset = sizeof(int) + sizeof(char)*15;
				compare_size = sizeof(char)*20;
				fieldName_flag = 2;
			}
			else if (strncmp(fieldName, "city", 5) == 0){
				offset = sizeof(int) + sizeof(char)*(15+20);
				compare_size = sizeof(char)*10;
				fieldName_flag = 2;
			}
			
			if(fieldName_flag == 1){
				/* UNCOMMENT IF YOU ARE PASSING ID AS A STRING*/
				/*
				long int converted_value = strtol( (char *)value, NULL, 10);
				if(errno != 0){
					perror("Problem Converting with strtol: ");
				}
				*/
				for(i = 1; i <= BlockNo; i++){
					if (BF_ReadBlock(fileDesc, i, &block) < 0){
						BF_PrintError("Error reading block");
						return;
					}	
					memcpy(&block_records, block, sizeof(short));
					//printf("-------------------\nReading Block %d\n-------------------\n", i);
					for(j = 0; j < block_records; j++){
						/*SWITCH THE "IF" STATEMENTS IF YOU ARE PASSING INT AS A STRING*/
						//if( *(int *)((char *)block+sizeof(short)+sizeof(Record)*j+offset) == converted_value) {
						if( *(int *)((char *)block+sizeof(short)+sizeof(Record)*j+offset) == *(int*)value) {
							records_found++;
							memcpy(&record, block+sizeof(short)+sizeof(Record)*j, sizeof(Record));
							printf("Reading from Block %d: id: %d \t Name: %s \t Surname: %s \t City: %s \n", i, record.id, record.name, record.surname, record.city);
						}
					}
				}
				printf("---------------------\nBlocks Read: %d!\nRecords Found: %d\n---------------------\n", i-1, records_found);
				
			} else if(fieldName_flag == 2){
				for(i = 1; i <= BlockNo; i++){
					if (BF_ReadBlock(fileDesc, i, &block) < 0){
						BF_PrintError("Error reading block");
						return;
					}	
					memcpy(&block_records, block, sizeof(short));
					for(j = 0; j < block_records; j++){
						if(strncmp(((char *)block+sizeof(short)+sizeof(Record)*j+offset), (char *) value, compare_size) == 0) {
							records_found++;
							memcpy(&record, block+sizeof(short)+sizeof(Record)*j, sizeof(Record));
							printf("Reading from Block %d: id: %d \t Name: %s \t Surname: %s \t City: %s \n", i, record.id, record.name, record.surname, record.city);
						}
					}
				}
				printf("---------------------\nBlocks Read: %d!\nRecords Found: %d\n---------------------\n", i-1, records_found);
				
			} else {
				printf("----------------\nWrong FieldName!\n----------------\n");
			}
		}
	} else {
		printf("--------------\nNo records!!!\nBlocks Read 0!\n--------------\n");
	}
}