#ifndef __DB_LINKED_LIST_H__
#define __DB_LINKED_LIST_H__

typedef struct _DList DList;

/* Doubly linked lists  */
DList* d_list_append(DList *list, void *data);
DList* d_list_last(DList *list);
int d_list_length(DList *list);
DList* d_list_remove_with_data(DList* list, void *data, void(*free_data)(void *data));
DList* d_list_insert(DList *list, void *data, int n);
DList* d_list_nth_recursion(DList *list, int n);
DList* d_list_nth_for(DList *list, int n);
DList* d_list_next(DList *list);
DList* d_list_remove_nth(DList *list, int index, void(*free_data)(void *data));
void d_list_print_all_data(DList *list, void(*print_data)(void* data));
void d_list_free(DList* list, void(*free_data)(void *data));
int d_list_nth_with_data(DList *list, void *data);
void* d_list_get_data(DList *list);
DList* d_list_insert_first(DList* list, void *data);
DList* d_list_bubble_sort(DList *list, int(*comp)(void *data1, void *data2));
DList* d_list_remove(DList *remove, void(*free_data)(void *data));
DList* d_list_first(DList *list);
DList* d_list_insert_sort(DList *list, int(*comp)(void *data1, void *data2));
DList* d_list_switch_node(DList* node1, DList* node2);
DList* d_list_prepend_node(DList* sorted_node, DList* insert_node, DList *list);
DList* d_list_delete(DList *remove);
void* d_list_find_data(DList *list, int(*find_data)(void *data, void *user_data), void *user_data);
void d_list_foreach(DList *list, void (*func)(void *data, void *user_data), void *user_data);
DList* d_list_remove_with_user_data(DList *list, void *user_data, int(*find_data)(void *data, void *user_data), void(*free_data)(void *data));
#endif
