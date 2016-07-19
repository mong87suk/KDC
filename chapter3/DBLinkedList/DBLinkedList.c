#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "DBLinkedList.h"

struct _DList
{
    DList *next;
    DList *prev;
    void *data;
};

DList* d_list_append(DList *list, void *data) {
    DList *new_list;
    DList *last;
    new_list = (DList*) malloc(sizeof(DList));
	if (!new_list) {
		return NULL;
	}
    new_list->next = NULL;
    new_list->data = data;

    if (list) {
        last = d_list_last(list);
        last->next = new_list;
        new_list->prev = last;
        return list;
    } else {
        new_list->prev = NULL;
        return new_list;
    }
}

DList* d_list_insert_first(DList *list, void *data) {
    DList *new_list;
    new_list = (DList*) malloc(sizeof(DList));
    if (!new_list) {
        return new_list;
    }
    new_list->prev = NULL;
    new_list->data = data;

    if (list) {
        list->prev = new_list;
        new_list->next = list;
    } else {
        new_list->next = NULL;
    }
    return new_list;
}

DList* d_list_last(DList *list) {
    if (list) {
        while (list->next) {
            list = list->next;
        }
    }
    return list;
}

DList* d_list_first(DList *list) {
	if (list) {
		while (list->prev) {
			list = list->prev;
		}
	}
	return list;
}

int d_list_length(DList *list) {
    int lenght = 0;
    while (list) {
        lenght++;
        list = list->next;
    }
    return lenght;
}

DList* d_list_remove_with_data(DList *list, void *data, void(*free_data)(void *data)) {
    DList *tmp;
    tmp = list;
    while (tmp) {
        if (tmp->data != data) {
            tmp = tmp->next;
        } else {
            list = d_list_remove(tmp, free_data);
            break;
        }
    }
    return list;
}

DList* d_list_remove_with_user_data(DList *list, void *user_data, int(*match_data)(void *data, void *user_data), void(*free_data)(void *data)) {
    DList *remove_data;

    if (!list) {
        printf("There is nothing to point DList\n");
        return NULL;
    }
    remove_data = d_list_find_data(list, match_data, user_data);

    if (!remove_data) {
        printf("There is nothing to point  DList to remove\n");
        return NULL;
    }

    list = d_list_remove_with_data(list, remove_data, free_data);
    return list;
}

void d_list_print_all_data(DList* list, void(*print_data)(void* data)) {
    while(list) {
        print_data(list->data);
        list = list->next;
    }
}

DList* d_list_insert(DList *list, void *data, int n) {
    DList *new_list;
    DList *tmp_list;

    tmp_list = d_list_nth_for(list, n);
    if (tmp_list == NULL) {
        printf("Can't insert data");
        return list;
    }

    new_list = (DList*) malloc(sizeof(DList));
    new_list->next = NULL;
    new_list->prev = NULL;
    new_list->data = data;

    if (tmp_list->prev) {
        tmp_list->prev->next = new_list;
        new_list->prev = tmp_list->prev;
    }
    new_list->next = tmp_list;
    tmp_list->prev = new_list;

    if (tmp_list == list) {
        return new_list;
    } else {
        return list;
    }
}

DList* d_list_nth_recursion(DList *list, int n) {
    if (n <= 0) {
        if (!list) {
            printf("Can't find Node\n");
        }
        return list;
    }
    return d_list_nth_recursion(d_list_next(list), n-1);
}

DList* d_list_nth_for(DList *list, int n) {
    int i;
    for (i = 0; i < n && list != NULL; i++) {
        list = list->next;
    }

    if (!list) {
        printf("Can't find Node\n");
    }
    return list;
}

void* d_list_get_data(DList *list) {
    if (!list) {
        printf("Can't get data\n");
        return 0;
    }
    return list->data;
}

DList* d_list_remove_nth(DList *list, int n, void(*free_data)(void *data)) {
    DList *remove;
    remove = d_list_nth_for(list, n);
    list = d_list_remove(remove, free_data);
    free_data(remove->data);
    free(remove);
    return list;
}

DList* d_list_remove(DList *remove, void(*free_data)(void *data)) {
    DList* first_node;
    if (!remove) {
        printf("No data\n");
        return remove;
    }
    first_node = d_list_first(remove);

    if (remove->prev) {
        remove->prev->next = remove->next;
    }

    if (remove->next) {
        remove->next->prev = remove->prev;
    }

    if (remove == first_node) {
        first_node = first_node->next;
    }

    if (free_data) {
        free_data(remove->data);
    }
    free(remove);

	return first_node;
}

DList* d_list_delete(DList *remove) {
    DList* first_node;
    if (!remove) {
        printf("No data\n");
        return remove;
    }
    first_node = d_list_first(remove);

    if (remove->prev) {
        remove->prev->next = remove->next;
    }

    if (remove->next) {
        remove->next->prev = remove->prev;
    }

    if (remove == first_node) {
        first_node = first_node->next;
    }
    free(remove);
	return first_node;
}

void d_list_free(DList *list, void(*free_data)(void *data)) {
    DList* remove;
    while (list) {
        remove = list;
        list = list->next;
        free_data(remove->data);
        free(remove);
    }
    return;
}

int d_list_nth_with_data(DList *list, void *data) {
    int i = 0;
    while (list) {
        if (list->data == data) {
            return i;
        }
        i++;
        list = list->next;
    }
    return -1;
}

DList* d_list_bubble_sort(DList* list, int(*comp)(void *data1, void *data2)) {
	bool is_switched = false;
	DList* node;
	DList* last_node;

	if (!(list) || !(list->next)) {
		printf("Can't sort the list\n");
		return list;
	}

	last_node = d_list_last(list);
    node = list;
	while (last_node) {
		while (node != last_node) {
			if (!node->next) {
				last_node = node;
				break;
			}
			if (comp(node->data, node->next->data)) {
				node = d_list_switch_node(node, node->next);
				is_switched = true;
			}
			node = node->next;
		}
		node = d_list_first(last_node);
		if (!is_switched) {
			break;
		}
		is_switched = false;
		last_node = last_node->prev;
	}
	return node;
}

DList* d_list_insert_sort(DList* list, int(*comp)(void *data1, void *data2)) {
	void* insert_data;
	DList* remember_node;
	DList* insert_node;
	DList* sorted_node;
	bool has_misaligned = false;
	remember_node = list->next;

	while (remember_node != NULL) {
		insert_node = remember_node;
		sorted_node = remember_node;
		insert_data = insert_node->data;
		remember_node = remember_node->next;
		while (((sorted_node = sorted_node->prev) != NULL) && comp(sorted_node->data, insert_data)) {
			has_misaligned = true;
		}
		if (has_misaligned) {
			list = d_list_prepend_node(sorted_node, insert_node, list);
			has_misaligned = false;
		}
	}
	return list;
}

DList* d_list_switch_node(DList* node1, DList* node2) {
	DList *tmp1 = node1->next;
	DList *tmp2 = node2->prev;

	node1->next = node2->next;
	if (node2->next) {
		node2->next->prev = node1;
	}

	node2->prev = node1->prev;

	if (node1->prev) {
		node1->prev->next = node2;
	}

	node1->prev = tmp1;
	node2->next = tmp2;
	return node2;
}

DList* d_list_prepend_node(DList* sorted_node, DList* insert_node, DList* list) {
    DList* sorted_node_next;
    DList* insert_node_prev = insert_node->prev;
    DList* insert_node_next = insert_node->next;

    if (sorted_node) {
        sorted_node_next = sorted_node->next;
        sorted_node->next = insert_node;
        insert_node->next = sorted_node_next;
        sorted_node_next->prev = insert_node;
    } else {
        insert_node->next = list;
        list->prev = insert_node;
        list = insert_node;
    }
    insert_node->prev = sorted_node;
    insert_node_prev->next = insert_node_next;
    if (insert_node_next) {
        insert_node_next->prev = insert_node_prev;
    }
    return list;
}

void *d_list_find_data(DList *list, int(*match_data)(void *data1, void *data2), void *data) {
    while (list) {
        if (match_data(list->data, data)) {
            return list->data;
        }
        list = list->next;
    }
    return NULL;
}

DList* d_list_next(DList *list) {
    if (list) {
        return list->next;
    }
    return list;
}

void d_list_foreach(DList *list, void (*func)(void *data, void *user_data), void *user_data) {
    DList *next;

    while (list) {
        next = list->next;
        func(list->data, user_data);
        list = next;
    }
}