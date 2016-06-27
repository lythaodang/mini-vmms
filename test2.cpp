// This is a simple test program

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

#include "vmms.h"

#define MAX_NUM_STUDENTS 20 // max number of students

typedef struct student {
	int	ID;
	char	name[20];
	struct student *next;
} Student;

typedef struct grades {
	char examName[20];
	int grade;
	struct student *student;
} GradeBook;

Student *student_list = NULL;
int num_of_students = 0;
void add_student(int id, char *name, int *rc);

int main(int argc, char** argv) {
	int rc = 0;

	printf("\nMalloc 20 bytes for temp name holder...\n");
	char *name = vmms_malloc(20, &rc);
	printf("Return code: %d\n", rc);
	system("pause");

	printf("\nTesting invalid memcpy size.\n");
	printf("Memcpy 21 bytes into 20 byte allocation...\n");
	rc = vmms_memcpy(name, "Ly Dang", 21);
	printf("Return code: %d\n", rc);
	system("pause");

	printf("\nTesting memcpy out of range.\n");
	printf("Memcpy 11 byte into name + 10\n");
	rc = vmms_memcpy(name + 10, "Ly Dang", 11);
	printf("Return code: %d\n", rc);
	system("pause");

	printf("\nTesting valid memcpy.\n");
	printf("Memcpy 20 bytes into 20 byte allocation\n");
	rc = vmms_memcpy(name, "Ly Dang", 7);
	printf("Return code: %d\n", rc);
	system("pause");

	printf("\nFree name allocation.\n");
	rc = vmms_free(name);
	printf("Return code: %d\n", rc);
	system("pause");

	printf("\nAdd a node to linked list.\n");
	printf("add_student(101, name, &rc);\n");
	add_student(101, "Ly Dang", &rc);
	printf("Return code: %d\n", rc);
	system("pause");

	printf("Name = %s; ID = %d\n", student_list->name, student_list->ID);
	return rc;
}

void add_student(int id, char *name, int *rc) {
	Student *temp;
	if (student_list != NULL) {
		temp = student_list;
		while (temp->next != NULL) {
			temp = temp->next;
		}
		printf("\nMalloc new student node...\n");
		temp->next = (Student*) vmms_malloc(sizeof(Student), rc);
		system("pause");
		if (*rc != 0) {
			return;
		}
		temp = temp->next;
	} else {
		printf("\nMalloc new student node...\n");
		student_list = (Student*) vmms_malloc(sizeof(Student), rc);
		system("pause");
		if (*rc != 0) {
			return;
		}
		temp = student_list;
	}
	
	temp->ID = id;

	printf("\nMemcpy student name...\n");
	*rc = vmms_memcpy(temp->name, name, strlen(name)); // set name
	system("pause");
	temp->next = NULL;
}





