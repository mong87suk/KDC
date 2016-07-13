#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "DBLinkedList.h"

int set_sort_rule(void *data1, void *data2) {
    int *d1 = (int*)data1;
    int *d2 = (int*)data2;
    if (*d1 > *d2) {
        return 1;
    } else {
        return 0;
    }
}

int set_string_sort_rule(void *data1, void *data2) {
    char *d1 = (char*)data1;
    char *d2 = (char*)data2;
    if (strcasecmp(d1, d2) > 0) {
        return 1;   
    } else {
        return 0;
    }
}


void print_all_data(void *data) {
    int *d = (int*)data;

    printf("data:%d\n", *d);
}

void print_all_string_data(void *data) {
    char *s = (char*)data;
    printf("%s\n", s);
}


void free_data(void *data) {
    int *remove = (int*)data;
    free(remove);
}

void free_string_data(void *data) {
    char *remove = (char*)data;
    free(remove);
}

void test();
DList* insert_data(int in);
DList* insert_string_data(const char *data, DList *list);

int main(void) {
    DList *list = NULL;
    DList *list2 = NULL;
    DList *list3 = NULL;
    DList *tmp;
    void* data;
    int* test_data;
    int length;
    list = insert_data(10);
    printf("before bubble sort\n");
    d_list_print_all_data(list, print_all_data);
    printf("after bubble sort\n");
    list = d_list_bubble_sort(list, set_sort_rule);
    d_list_print_all_data(list, print_all_data);
    tmp = d_list_last(list);
    data = d_list_get_data(tmp);
    test_data = (int*)data;
    printf("tmp:%d\n", *test_data);
    printf("after remove data\n");
    list = d_list_remove(tmp, free_data);
    d_list_print_all_data(list, print_all_data);
    tmp = d_list_first(list);
    list = d_list_remove(tmp, free_data);
    printf("after remove data\n");
    d_list_print_all_data(list, print_all_data);
    length = d_list_length(list);
    printf("list length:%d\n", length);
    tmp = d_list_nth_for(list, 5);
    data = d_list_get_data(tmp);
    test_data = (int*)data;
    printf("test_data: %d\n", *test_data);
    list = d_list_remove_with_data(list, data, free_data);
    printf("after_remove_nth_with_data\n");
    d_list_print_all_data(list, print_all_data);
    list2 = insert_data(10);
    printf("before insert sort\n");
    d_list_print_all_data(list2, print_all_data);
    printf("after insert sort\n");
    list2 = d_list_insert_sort(list2, set_sort_rule);
    d_list_print_all_data(list2, print_all_data);
    d_list_free(list2, free_data);
    printf("insert string\n");
    list3 = insert_string_data("A", list3);
    list3 = insert_string_data("E", list3);
    list3 = insert_string_data("z", list3);
    list3 = insert_string_data("f", list3);
    list3 = insert_string_data("o", list3);
    list3 = insert_string_data("d", list3);
    d_list_print_all_data(list3, print_all_string_data);
    printf("after sort string data\n");
    list3 = d_list_insert_sort(list3, set_string_sort_rule);
    d_list_print_all_data(list3, print_all_string_data);
    d_list_free(list3, free_string_data);
    test();
    return 0;
}

DList* insert_data(int in) {
    int *data;
    int i;
    int random_number;
    DList* list;
    list = NULL;
    srand((unsigned int)time(NULL));

    for (i = 0; i < in; i++) {  
        data = (int*)malloc(sizeof(int));
        random_number = rand() % in + 1;
        *data = random_number;
        printf("random number:%d\n", random_number);
        list = d_list_append(list, data);
    }
    return list;
}

DList* insert_string_data(const char *data, DList* list) {
    char *s = NULL;
    s = strdup(data);
    list = d_list_append(list, s);
    return list;
}

void test() {
	int max = 100;
	float gap;
	time_t start, end;
	DList *list = NULL;
	
	list = insert_data(max);
 	printf("Start to find index in for loop\n");
	start = clock();
	d_list_nth_for(list, max-1);
	end = clock();
	printf("End to find index in for loop\n");
	gap = (float)(end-start)/(CLOCKS_PER_SEC);
	printf("Total time in for loop: %f sec\n", gap); 

	printf("Start to find index in recursion function\n");
	start = clock();
	d_list_nth_recursion(list, max-1);
	end = clock();
	printf("End to find index in recurtion function\n");
	gap = (float)(end-start)/(CLOCKS_PER_SEC);
	printf("Total time in recursion function: %f sec\n", gap); 
    d_list_free(list, free_data);
	return;
} 
