/* 
  vmms.h - This file contains the function prototype for the memory functions.
*/

#include "vmms_error.h"

/* Here are the 5 exported functions for the application programs to use */
extern char* vmms_malloc (  int size, int* error_code );
extern int vmms_memset ( char* dest_ptr, char c, int size );
extern int vmms_memcpy ( char* dest_ptr, char* src_ptr, int size );
extern int vmms_print ( char* src_ptr, int size );
extern int vmms_free ( char* mem_ptr );

/* Here are several exported functions specifically for mmc.cpp */
extern int mmc_initialize (  int boundry_size );
extern int mmc_display_memtable ( char* filename );
extern int mmc_display_memory ( char* filename );
 


