/*
  vmms.cpp - This file contains the code for each of the memory functions as well as initialization of the "shared" memory.
*/

#include "vmms_error.h"
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <string.h>
#include <process.h>
#include <stdarg.h>
#include <time.h>

#define MAX_PHY_SIZE 8196    // 8K Bytes     ** Hardcode for testing !!
#define MAX_TABLE_SIZE 1024  // 1K entries
#define DEFAULT_BOUNDRY 8    // minimum 8 byte allocation block
#define TIME_STAMP_LENGTH 14 // time stamp length 
#define NULL 0               // null pointer
#define DEBUG 0              // debugging 
#define LOG_FILE "VMMS.LOG"  // log file name 

// a struct that represents a free list entry with its size
typedef struct {
	char *addr;
	int size;
} FreeListEntry;

// a struct that represents a mapping table entry
// 32 bytes each entry
typedef struct {
	int pid; // client process id
	int request_size; // client request size
	int actual_size; // actually allocated size
	char *client_address; // address returned by malloc
	char time_stamp[15]; // timestamp in a form of YYYYMMDDHHMMSS
} TableEntry;

// ************************************************************************
// Global Shared variables (shared by multiple instances of the DLL)
// ************************************************************************

/* Global shared memory section */
#pragma data_seg (".SHARED")  // Simulated Physical Mem // Hardcoded for now !!
int byte_boundry = DEFAULT_BOUNDRY;
int mem_size = MAX_PHY_SIZE;            // size of simulated phy mem (in bytes)
char mem_start[MAX_PHY_SIZE] = { 0 };  	// simulated Phy Memory
int num_mapping_entries = 0;            // number of mapping entries used so far
int num_list_entries = 0;
TableEntry mapping_table[MAX_TABLE_SIZE] = { 0 }; // memory mapping table
FreeListEntry free_list[MAX_TABLE_SIZE] = { 0 }; // free list 
TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
#pragma data_seg ()

// Map table Structures/Entries
// 

/* Here are the 5 exported functions for the application programs to use */
__declspec(dllexport) char* vmms_malloc(int size, int* error_code);
__declspec(dllexport) int vmms_memset(char* dest_ptr, char c, int size);
__declspec(dllexport) int vmms_memcpy(char* dest_ptr, char* src_ptr, int size);
__declspec(dllexport) int vmms_print(char* src_ptr, int size);
__declspec(dllexport) int vmms_free(char* mem_ptr);

/* Here are several exported functions specifically for mmc.cpp */
__declspec(dllexport) int mmc_initialize(int boundry_size);
__declspec(dllexport) int mmc_display_memtable(char* filename);
__declspec(dllexport) int mmc_display_memory(char* filename);


/* Make a timestamp using a buffer passed to the function */
void make_time_stamp(char *time_buffer) {
	time_t timer;
	struct tm *time_info;

	time(&timer);
	time_info = localtime(&timer);
	strftime(time_buffer, TIME_STAMP_LENGTH + 1, "%Y%m%d%H%M%S", time_info);
}

/* Make process name using a buffer & pid passed to the function */
void make_process_name(DWORD processID) {
	// Get a handle to the process.
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
		PROCESS_VM_READ,
		FALSE, processID);

	// Get the process name.
	if (NULL != hProcess) {
		HMODULE hMod;
		DWORD cbNeeded;

		if (EnumProcessModules(hProcess, &hMod, sizeof(hMod),
			&cbNeeded)) {
			GetModuleBaseName(hProcess, hMod, szProcessName,
				sizeof(szProcessName) / sizeof(TCHAR));
		}
	}

	// Release the handle to the process.
	CloseHandle(hProcess);
}

/* Add a table entry into mapping table */
void add_table_entry(int pid, int rsize, int asize, char *addr) {
	mapping_table[num_mapping_entries].pid = pid;
	mapping_table[num_mapping_entries].request_size = rsize;
	mapping_table[num_mapping_entries].actual_size = asize;
	mapping_table[num_mapping_entries].client_address = addr;
	make_time_stamp(mapping_table[num_mapping_entries].time_stamp);
	num_mapping_entries++;
}

/* Log a request into VMMS.LOG file */
void log_request(const char *format, ...) {
	FILE *log;
	if ((log = fopen(LOG_FILE, "a+")) == NULL) {
		// error opening the log file
		printf("--Log Request: Error opening the log file--\n");
	} else {
		va_list args;
		va_start(args, format);
		vfprintf(log, format, args);
		va_end(args);
		fclose(log);
	}
}

/* flush content of memory pool to disk (VMMS.MEM) */
void flush_to_disk(char *filename) {
	FILE *vmms_file;
	if ((vmms_file = fopen(filename, "wb")) == NULL) {
		printf("--Error opening %s--\n", filename);
	} else {
		fwrite(mem_start, MAX_PHY_SIZE, 1, vmms_file);

		if (strcmp(filename, "VMMS.MEM") == 0) {
			fwrite(mapping_table, sizeof(mapping_table), 1, vmms_file);
			fwrite(free_list, sizeof(free_list), 1, vmms_file);
		}
		fflush(vmms_file);
		fclose(vmms_file);
	}
}

__declspec(dllexport) int mmc_initialize(int boundry_size) {
	int rc = VMMS_SUCCESS;
	// make sure that byte boundary is multiple of DEFAULT_BOUNDRY
	if (boundry_size > DEFAULT_BOUNDRY) {
		if (boundry_size % DEFAULT_BOUNDRY) byte_boundry = boundry_size;
		else byte_boundry = boundry_size + DEFAULT_BOUNDRY - (boundry_size % DEFAULT_BOUNDRY);
	}

	// initialize first entry of free list
	free_list[0] = { mem_start, mem_size };
	num_list_entries++;
	flush_to_disk("VMMS.MEM");
	return rc;
}

__declspec(dllexport) int mmc_display_memtable(char* filename) {
	int rc = VMMS_SUCCESS;
	int i, j;

	/* Put your source code here */
	if (strcmp(filename, "") == 0) { // file name is empty or not given
		printf("Mapping Table Entry\n");
		printf("+---------------+---------------+---------------+---------------+---------------+\n");
		printf("|%-*s|%-*s|%-*s|%-*s|%-*s|\n", 15, "PID", 15, "Address", 15, "Request Size", 15, "Actual Size", 15, "Timestamp");
		printf("+---------------+---------------+---------------+---------------+---------------+\n");
		for (i = 0; i < num_mapping_entries; i++) {
			printf("|%-*d|%-*p|%-*d|%-*d|%-*s|\n", 15, mapping_table[i].pid, 15, mapping_table[i].client_address,
				15, mapping_table[i].request_size, 15, mapping_table[i].actual_size, 15, mapping_table[i].time_stamp);
		}
		printf("+---------------+---------------+---------------+---------------+---------------+\n");
		printf("%d row(s) displayed\n\n", num_mapping_entries);

		printf("Free List Table\n");
		printf("+---------------+---------------+\n");
		printf("|%-*s|%-*s|\n", 15, "Address", 15, "Size");
		printf("+---------------+---------------+\n");
		for (i = 0; i < num_list_entries; i++) {
			printf("|%-*p|%-*d|\n", 15, free_list[i].addr, 15, free_list[i].size);
		}
		printf("+---------------+---------------+\n");
		printf("%d row(s) displayed\n", num_list_entries);


	} else {
		FILE *vmms_file;
		if ((vmms_file = fopen(filename, "w")) == NULL) {
			printf("--%s could not be opened for write--\n", filename);
		} else {
			fprintf(vmms_file, "Mapping Table Entry\n");
			fprintf(vmms_file, "+---------------+---------------+---------------+---------------+---------------+\n");
			fprintf(vmms_file, "|%-*s|%-*s|%-*s|%-*s|%-*s|\n", 15, "PID", 15, "Address", 15, "Request Size", 15, "Actual Size", 15, "Timestamp");
			fprintf(vmms_file, "+---------------+---------------+---------------+---------------+---------------+\n");
			for (i = 0; i < num_mapping_entries; i++) {
				fprintf(vmms_file, "|%-*d|%-*p|%-*d|%-*d|%-*s|\n", 15, mapping_table[i].pid, 15, mapping_table[i].client_address,
					15, mapping_table[i].request_size, 15, mapping_table[i].actual_size, 15, mapping_table[i].time_stamp);
			}
			fprintf(vmms_file, "+---------------+---------------+---------------+---------------+---------------+\n");
			fprintf(vmms_file, "%d row(s) displayed\n\n", num_mapping_entries);

			fprintf(vmms_file, "Free List Table\n");
			fprintf(vmms_file, "+---------------+---------------+\n");
			fprintf(vmms_file, "|%-*s|%-*s|\n", 15, "Address", 15, "Size");
			fprintf(vmms_file, "+---------------+---------------+\n");
			for (i = 0; i < num_list_entries; i++) {
				fprintf(vmms_file, "|%-*p|%-*d|\n", 15, free_list[i].addr, 15, free_list[i].size);
			}
			fprintf(vmms_file, "+---------------+---------------+\n");
			fprintf(vmms_file, "%d row(s) displayed\n", num_list_entries);

			fclose(vmms_file);
		}
	}

	return rc;
}

__declspec(dllexport) int mmc_display_memory(char* filename) {
	int rc = VMMS_SUCCESS;

	/* Put your source code here */
	if (strcmp(filename, "") == 0) { // file name is empty or not given
		int i = 0, j = 0, k = 0, count = 0;
		char *temp = mem_start;
		for (; temp < mem_start + MAX_PHY_SIZE;) {
			printf("%p  ", temp); // print address
			if ((mem_start + MAX_PHY_SIZE - temp > 16) ||
				(((mem_start + MAX_PHY_SIZE - temp) % 16) == 0)) {
				count = 16;
			} else {
				count = (mem_start + MAX_PHY_SIZE - temp) % 16;
			}
			
			for (j = 0; j < count; j++) {
				if (j == 8) {
					printf(" ");
				}
			
				printf("%02X ", (unsigned char)*temp); // print hex bytes
				temp += 1;
			}
			if (count < 16) {
				for (k = 0; k < (16 - count); k++) {
					printf("   ");
				}
				printf(" ");
			}
			printf(" ");
			temp -= count;
			for (j = 0; j < count; j++) {
				if (*temp == NULL || (unsigned char)*temp > 127) {
					printf("."); // print . for character rep if extended ascii or null
				} else {
					printf("%c", *temp); // print character rep
				}
				temp += 1;
			}
			
			printf("\n");
		}
	} else {
		if (strstr(filename, "bin")) {
			flush_to_disk(filename);
		} else {
			FILE *vmms_file;
			if ((vmms_file = fopen(filename, "w")) == NULL) {
				printf("--%s could not be opened for write--\n", filename);
			} else {
				int i = 0, j;
				char *temp = mem_start;
				for (; temp < mem_start + MAX_PHY_SIZE;) {
					fprintf(vmms_file, "%p  ", temp); // print address
					for (j = 0; j < 16; j++) {
						if (j == 8) {
							fprintf(vmms_file, " ");
						}
						fprintf(vmms_file, "%02X ", (unsigned char) *temp); // print hex bytes
						temp += 1;
					}
					fprintf(vmms_file, " ");
					temp -= 16;
					for (j = 0; j < 16; j++) {
						if (*temp == NULL || (unsigned char) *temp > 127) {
							fprintf(vmms_file, "."); // print . for character rep if extended ascii or null
						} else {
							fprintf(vmms_file, "%c", *temp); // print character rep
						}
						temp += 1;
					}

					fprintf(vmms_file, "\n");
				}
			}
			fclose(vmms_file);
		}
		
	}

	return rc;
}

__declspec(dllexport) char* vmms_malloc(int size, int* error_code) {
	/* Put your source code here */
	if (error_code == NULL || size <= 0) { // error code is not given or size is non-positive
		return NULL;
	}

	*error_code = VMMS_SUCCESS;
	int rounded_size = size;
	char *returnMem = NULL;
	int pid = _getpid();

	// round size to a multiple of byte_boundry
	if (rounded_size % byte_boundry != 0) {
		rounded_size += byte_boundry - (rounded_size % byte_boundry);
	}
#if DEBUG
	printf("#list entries = %d\n", num_list_entries);
	printf("#table entries = %d\n", num_mapping_entries);
#endif

#if DEBUG
	printf("Free list is not empty\n");
#endif
	// search for a best fit in free list
	int i, min_index, min_size = MAX_PHY_SIZE;
	FreeListEntry *min_entry = NULL;

	for (i = 0; i < num_list_entries; i++) {
		if (free_list[i].size == rounded_size) { // exact size, then return to client 
			returnMem = free_list[i].addr; // temp store address of free list
			memcpy(&free_list[i], &free_list[i + 1], sizeof(FreeListEntry) * (num_list_entries - i - 1)); // remove the free list entry
			memset(&free_list[num_mapping_entries - 1], NULL, sizeof(FreeListEntry)); // null the last entry
			num_list_entries--; // removed one entry
			//free_list[i].size -= rounded_size;
			
			add_table_entry(pid, size, rounded_size, returnMem); // add an entry to mapping table 
			flush_to_disk("VMMS.MEM");
			return returnMem;
		} else if (free_list[i].size > rounded_size) { // big enough
			if (min_entry == NULL || free_list[i].size < min_size) {
				min_entry = &(free_list[i]);
				min_size = min_entry->size;
				min_index = i;
			}
		}
	}

	if (min_entry != NULL) { // found the best fit
		if (free_list[num_list_entries].addr + free_list[num_list_entries].size == mem_start + MAX_PHY_SIZE &&
			free_list[num_list_entries].size > rounded_size) {
			returnMem = free_list[num_list_entries].addr;
			free_list[num_list_entries].size -= rounded_size;
			free_list[num_list_entries].addr += rounded_size;
			add_table_entry(pid, size, rounded_size, returnMem);
			flush_to_disk("VMMS.MEM");
			return returnMem;
		} else {
			returnMem = min_entry->addr;
			int size_difference = min_entry->size - rounded_size;
			if (size_difference >= byte_boundry) { // right chunk might be useful for next use
				min_entry->size = size_difference;
				min_entry->addr += rounded_size;
			} else {
				memcpy(&min_entry, &free_list[i + 1], sizeof(FreeListEntry) * (num_list_entries - i - 1)); // remove the free list entry
				memset(&free_list[num_mapping_entries - 1], NULL, sizeof(FreeListEntry)); // null the last entry
				num_list_entries--; // removed one entry
				rounded_size += size_difference;
			}
			add_table_entry(pid, size, rounded_size, returnMem);
		}
#if DEBUG
		printf("Request size is %d\nActual size is %d\n", size, rounded_size);
		printf("Return address is %p\n", returnMem);
		printf("Next free block is %p\n", free_list[num_list_entries - 1].addr);
		printf("Available space is %d\n", free_list[num_list_entries - 1].size);
#endif
		flush_to_disk("VMMS.MEM");
	} else { // can't find a free block or out of memory
		*error_code = OUT_OF_MEM;
		printf("--Malloc Error: Out of memory.--\n");
	}

	//TCHAR process_name[MAX_PATH] = TEXT("<unknown>");
	make_process_name(pid);
	log_request("%14s %s %d %s %p %d %d\n", mapping_table[num_mapping_entries - 1].time_stamp,
		szProcessName, pid, "vmms_malloc", returnMem, size, *error_code);

#if DEBUG
	printf("#list entries = %d\n", num_list_entries);
	printf("#table entries = %d\n", num_mapping_entries);
#endif

	return returnMem;
}

__declspec(dllexport) int vmms_memset(char* dest_ptr, char c, int size) {
	int rc = VMMS_SUCCESS;
	int pid = _getpid();
	int temp_size = size;

	/* Put your source code here */
	if (dest_ptr == NULL) {
		rc = INVALID_DEST_ADDR;
		printf("--Memset Error: Invalid destination address.--\n");
	} else if (size < 0) {
		rc = INVALID_REQUEST_SIZE;
		printf("--Memset Error: Invalid request size.--\n");
	} else {
		int i, found = 0, available_size = 0;
		// search to see if dest_ptr is a valid address
		for (i = 0; i < num_mapping_entries && !found; i++) {
			char *client_address = mapping_table[i].client_address;
			int client_size = mapping_table[i].request_size;
			if (mapping_table[i].pid == pid &&
				(client_address <= dest_ptr &&
				client_address + client_size > dest_ptr)) { // dest_ptr is within client_address range
				found = 1;
				available_size = client_address + client_size - dest_ptr;
			}
		}
		if (found) {
			i--; // move i back to the correct index
			if (available_size < size) { // memory too small
				rc = MEM_TOO_SMALL;
				printf("--Memset Error: Memory allocated < size.--\n");
			} else {
				while (temp_size--) { *dest_ptr++ = c; }
				// update time for last write request
				make_time_stamp(mapping_table[i].time_stamp);
				flush_to_disk("VMMS.MEM");
			}
		} else { // not found in the mapping table
			rc = INVALID_DEST_ADDR;
			printf("--Memset Error: Pointer was not found in the mapping table.--\n");
		}
	}

	char time_buffer[15] = { 0 };
	make_process_name(pid);
	make_time_stamp(time_buffer);
	log_request("%14s %s %d %s %d %p %c %d\n", time_buffer, szProcessName, pid, "vmms_memset", rc, dest_ptr, c, size);

	return rc;
}


__declspec(dllexport) int vmms_memcpy(char* dest_ptr, char* src_ptr, int size) {
	int rc = VMMS_SUCCESS;
	int pid = _getpid();
	int temp_size = size;

	/* Put your source code here */
	if (dest_ptr == NULL && src_ptr == NULL) {
		rc = INVALID_CPY_ADDR;
		printf("--Memcopy Error: Invalid copy address.--\n");
	} else if (size < 0) {
		rc = INVALID_REQUEST_SIZE;
		printf("--Memcopy Error: Invalid request size.--\n");
	} else {
		int i, found = 0, available_size = 0;
		// search to see if src_ptr is a valid address
		for (i = 0; i < num_mapping_entries && !found; i++) {
			char *client_address = mapping_table[i].client_address;
			int client_size = mapping_table[i].request_size;
			if (mapping_table[i].pid == pid &&
				(client_address <= src_ptr &&
				client_address + client_size > src_ptr)) { // src_ptr is within client_address range
				found = 1;
				available_size = client_address + client_size - src_ptr;
			}
		}

		int j, found1 = 0, available_size1 = 0;
		// search to see if dest_ptr is a valid address
		for (j = 0; j < num_mapping_entries && !found; j++) {
			char *client_address = mapping_table[j].client_address;
			int client_size = mapping_table[j].request_size;
			if (mapping_table[j].pid == pid &&
				(client_address <= dest_ptr &&
				client_address + client_size > dest_ptr)) { // dest_ptr is within client_address range
				found1 = 1;
				available_size1 = client_address + client_size - dest_ptr;
			}
		}

		if (found || found1) {
			if (found && available_size < size) { // memory too small
				rc = MEM_TOO_SMALL;
				printf("--Memcopy Error: Source size is less than available allocated size.--\n");
			}
			
			if (found1 && available_size1 < size) { // dest_ptr is in the pool but its available size < size
				rc = MEM_TOO_SMALL;
				printf("--Memcopy Error: Destination size is less than available allocated size.--\n");
			}
		} 

		if (!rc) {
			while (temp_size--) { *dest_ptr++ = *src_ptr++; }
			if (found) {
				i--;
				make_time_stamp(mapping_table[i].time_stamp);
			}
			if (found1) {
				j--;
				make_time_stamp(mapping_table[j].time_stamp);
			}
			flush_to_disk("VMMS.MEM");
		}
	}

	char time_buffer[15] = { 0 };
	make_process_name(pid);
	make_time_stamp(time_buffer);
	log_request("%14s %s %d %s %d %p %p %d\n", time_buffer, szProcessName, pid, "vmms_memcpy", rc, dest_ptr, src_ptr, size);

	return rc;
}

__declspec(dllexport) int vmms_print(char* src_ptr, int size) {
	int rc = VMMS_SUCCESS;
	int pid = _getpid();
	int temp_size = size;

	/* Put your source code here */
	if (src_ptr == NULL) {
		rc = INVALID_CPY_ADDR;
		printf("--Print Error: Invalid source address.--\n");
	} else if (size < 0) {
		rc = INVALID_REQUEST_SIZE;
		printf("--Print Error: Invalid size.--\n");
	} else if (size > 0) {
		// check if src_ptr is a valid address in pool
		// if it is, then compare its available size with size
		int i, found = 0, available_size = 0;

		for (i = 0; i < num_mapping_entries && !found; i++) {
			char *client_address = mapping_table[i].client_address;
			int client_size = mapping_table[i].request_size;
			if (mapping_table[i].pid == pid &&
				(client_address <= src_ptr &&
				client_address + client_size > src_ptr)) { // src_ptr is within client_address range
				found = 1;
				available_size = client_address + client_size - src_ptr;
			}
		}
		if (found) { // internal buffer 
			if (available_size < size) { // mem too small
				rc = MEM_TOO_SMALL;
				printf("--Print Error: Size is less than available allocated size.--\n");
			}
		} 

		if (!rc) {
			while (temp_size--) { printf("%c", *src_ptr++); }
		}
	} else {
		while (*src_ptr) { printf("%c", *src_ptr++); }
	}

	char time_buffer[15] = { 0 };
	make_process_name(pid);
	make_time_stamp(time_buffer);
	log_request("%14s %s %d %s %d %p %d\n", time_buffer, szProcessName, pid, "vmms_print", rc, src_ptr, size);

	return rc;
}

__declspec(dllexport) int vmms_free(char* mem_ptr) {
	int rc = VMMS_SUCCESS;
	int pid = _getpid();

	/* Put your source code here */
	int i, j, k = 0, l, found = 0, size = 0;
	char *end;
	for (i = 0; i < num_mapping_entries; i++) { // iterate mapping table
		if (mapping_table[i].client_address == mem_ptr) { // found entry
			size = mapping_table[i].actual_size; // get size of table entry
			end = mem_ptr + size; // get the last memory location allocated
			memset(mem_ptr, 0xFF, size); // set freed memory to xFF
			if (i != num_mapping_entries - 1) { // move entries up if not last entry
				memcpy(&mapping_table[i], &mapping_table[i + 1], sizeof(TableEntry) * (num_mapping_entries - i - 1));
				memset(&mapping_table[num_mapping_entries - 1], NULL, sizeof(TableEntry));
			}
			found = 1;
			num_mapping_entries--; // decrement number of mapping entries


			if (end <= free_list[0].addr) {
				if (end == free_list[0].addr) { // merge with head
					free_list[0].addr = mem_ptr;
					free_list[0].size = free_list[0].size + size;
				} else { // insert head of list
					memcpy(&free_list[1], &free_list[0], sizeof(FreeListEntry) * num_list_entries);
					free_list[0].addr = mem_ptr;
					free_list[0].size = size;
					num_list_entries++;
				}
			} else if (mem_ptr >= (free_list[num_list_entries - 1].addr + free_list[num_list_entries - 1].size)) { // check if mem_ptr is greater than last entry
				if (mem_ptr == (free_list[num_list_entries - 1].addr + free_list[num_list_entries - 1].size)) { // merge with last
					free_list[num_list_entries - 1].size += size;
				} else { // insert end of list
					free_list[num_list_entries].addr = mem_ptr;
					free_list[num_list_entries].size = size;
					num_list_entries++;
				}
			} else { // insert between
				for (k = 0; k < num_list_entries - 1; k++) { // check between first and last entries
					if ((mem_ptr > free_list[k].addr + free_list[k].size) // k.addr + size < mem_ptr
						&& (mem_ptr + size < free_list[k + 1].addr)) { // k+1.addr > mem_ptr + size
						if (mem_ptr == free_list[k].addr) { // merge left adjacent entry
							free_list[k].size += size;
							if (end == free_list[k + 1].addr) { // merge right adjacent entry
								free_list[k].size += free_list[k + 1].size;
								memcpy(&free_list[k + 1], &free_list[k + 2], sizeof(FreeListEntry) * (num_list_entries - k - 2)); // move entries from k + 1 to k + 2
							}
						} else if (end == free_list[k + 1].addr) {
							free_list[k + 1].addr = mem_ptr;
							free_list[k + 1].size += size;
						} else {
							// k + 1 and beyond move up one entry
							memcpy(&free_list[k + 2], &free_list[k + 1], sizeof(FreeListEntry) * (num_list_entries - k - 1)); // move entries from k + 1 to k + 2

						   // entry goes into location k + 1
							free_list[k + 1].addr = mem_ptr;
							free_list[k + 1].size = size;

							num_list_entries++;
						}
					}
				}
			}
			flush_to_disk("VMMS.MEM");
			break;
		}
	}

	if (!found) {
		rc = INVALID_MEM_ADDR;
		printf("--Free Error: Entry was not found.--\n");
	}

	char time_buffer[15] = { 0 };
	make_process_name(pid);
	make_time_stamp(time_buffer);
	log_request("%14s %s %d %s %d %p\n", time_buffer, szProcessName, pid, "vmms_free", rc, mem_ptr);

	return rc;
}

