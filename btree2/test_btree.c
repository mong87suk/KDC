#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "btree.h"

static char *new_str(char *str) {
    int len = strlen(str);
    char *new_str = (char*) calloc(len + 1, sizeof(char));
    if (!new_str) {
        printf("Failed to make the buf\n");
        return NULL;
    }
    strcpy(new_str, str);
    return new_str;    
}

int main() 
{ 
    long int n;
    char *str;
    
    Node *root = NULL;
    Key *key, *cmp_key1, *cmp_key2, *cmp_key3, *cmp_key4;

    char *num = (char *) calloc(2, sizeof(char));
    assert(num);

    for (int i = 0; i < 12; i++) {
        n = random() % 100;
        sprintf(num, "%ld", n);
        str = new_str(num);
        assert(str);
        key = new_key(str);
        assert(key);
        root = btree_insert(key, root);
        assert(root);
    }
    btree_search(root);

    sprintf(num, "%d", 83);
    str = new_str(num);
    cmp_key1 = new_key(str);
    key = btree_find(cmp_key1, root);
    assert(key);
    str = btree_get_str(key);
    assert(str);
    assert(strcmp(str, num) == 0);

    sprintf(num, "%d", 15);
    str = new_str(num);
    cmp_key2 = new_key(str);
    key = btree_find(cmp_key2, root);
    assert(key);
    str = btree_get_str(key);
    assert(str);
    assert(strcmp(str, num) == 0);

    sprintf(num, "%d", 77);
    str = new_str(num);
    cmp_key3 = new_key(str);
    key = btree_find(cmp_key3, root);
    assert(key);
    str = btree_get_str(key);
    assert(str);
    assert(strcmp(str, num) == 0);

    sprintf(num, "%d", 93);
    str = new_str(num);
    cmp_key4 = new_key(str);
    key = btree_find(cmp_key4, root);
    assert(key);
    str = btree_get_str(key);
    assert(str);
    assert(strcmp(str, num) == 0);

    root = btree_delete(cmp_key1, root);
    assert(root);
    root = btree_delete(cmp_key2, root);
    assert(root);
    root = btree_delete(cmp_key3, root);
    assert(root);
    root = btree_delete(cmp_key4, root);
    assert(root);
    btree_search(root);

    root = btree_insert(cmp_key1, root);
    assert(root);
    root = btree_insert(cmp_key2, root);
    assert(root);
    root = btree_insert(cmp_key3, root);
    assert(root);
    root = btree_insert(cmp_key4, root);
    assert(root);
    btree_search(root); 

    return 0; 
}/*End of main()*/ 