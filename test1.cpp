// This is a simple test program

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

#include "vmms.h"

int main(int argc, char** argv)
{
  int rc = 0;
  char *list;

  printf("\nTesting malloc 50\n");
  list = (char*) vmms_malloc(50, &rc);
  printf("Return code: %d\n", rc);
  system("pause");

  if (list == NULL) {
	  return rc;
  }
   
  printf("\nTesting memcpy (list)\n");
  rc = vmms_memcpy(list, "Testing", 7);
  printf("Return code: %d\n", rc);
  system("pause");

  printf("\nTesting memcpy (list + 10)\n");
  rc = vmms_memcpy(list+10, "memcpy", 3);
  printf("Return code: %d\n", rc);
  system("pause");

  printf("\nlist address = %8x; Name = %s; ID = %s\n", list, list, (char*)list+10);

  printf("\nTesting malloc 10\n");
  char *test = (char*) vmms_malloc(10, &rc);
  printf("Return code: %d\n", rc);
  system("pause");

  printf("\nTesting memset\n");
  rc = vmms_memset(test, 'c', 9);
  test[9] = '\0';
  printf("Return code: %d\n", rc);
  system("pause");

  printf("\nTesting print\n");
  rc = vmms_print(test, 9);
  printf("\n");
  printf("Return code: %d\n", rc);
  system("pause");

  printf("\nTesting free invalid ptr\n");
  test = (char*) malloc(10);
  rc = vmms_free(test);
  printf("Return code: %d\n", rc);
  system("pause");
  return rc;
}



	


