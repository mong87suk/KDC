#include <glib.h>
#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "btree.h"

int main() 
{
    //long int n;
    char *str;
    
    Node *root = NULL;
    Key *key, *cmp_key1, *cmp_key2, *cmp_key3, *cmp_key4;

    char *num = (char *) calloc(3, sizeof(char));
    assert(num);
    printf("insert start\n");

    GHashTable *hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    
    for (int i = 1; i <= 10000000; i++) {
        sprintf(num, "%d", i);
        assert(str);
        key = new_key(num);
        assert(key);
        root = btree_insert(key, root);
        assert(root);
        g_hash_table_insert(hash, g_strdup(num), g_strdup(num));
    }
    printf("insert finish\n");
    //btree_print(root);

    sprintf(num, "%d", 130);
    cmp_key1 = new_key(num);
    clock_t a1 = clock();
    key = btree_find(cmp_key1, root);
    clock_t a2 = clock();
    printf(" tree time %lf, ", ((double) (a2 - a1)) / (CLOCKS_PER_SEC / 1000000));
    a1 = clock();
    str = g_hash_table_lookup(hash, num);
    a2 = clock();
    printf("glib time %lf, ", ((double) (a2 - a1)) / (CLOCKS_PER_SEC / 1000000));
    printf(" glib find_key:%s, ", str);
    assert(key);
    str = btree_get_str(key);
    assert(str);
    assert(strcmp(str, num) == 0);
    printf(" tree find_key:%s\n", str);

    sprintf(num, "%d", 230);
    cmp_key2 = new_key(num);
    a1 = clock();
    key = btree_find(cmp_key2, root);
    a2 = clock();
    printf(" tree time %lf, ", ((double) (a2 - a1)) / (CLOCKS_PER_SEC / 1000000));
    a1 = clock();
    str = g_hash_table_lookup(hash, num);
    a2 = clock();
    printf("glib time %lf, ", ((double) (a2 - a1)) / (CLOCKS_PER_SEC / 1000000));
    printf(" glib find_key:%s, ", str);
    assert(key);
    str = btree_get_str(key);
    assert(str);
    assert(strcmp(str, num) == 0);
    printf(" tree find_key:%s\n", str);

    sprintf(num, "%d", 10);
    cmp_key3 = new_key(num);
    a1 = clock();
    key = btree_find(cmp_key3, root);
    a2 = clock();
    printf(" tree time %lf, ", ((double) (a2 - a1)) / (CLOCKS_PER_SEC / 1000000));
    a1 = clock();
    str = g_hash_table_lookup(hash, num);
    a2 = clock();
    printf("glib time %lf, ", ((double) (a2 - a1)) / (CLOCKS_PER_SEC / 1000000));
    printf(" glib find_key:%s, ", str);
    assert(key);
    str = btree_get_str(key);
    assert(str);
    assert(strcmp(str, num) == 0);
    printf(" tree find_key:%s\n", str);


    sprintf(num, "%d", 10);
    cmp_key4 = new_key(num);
    a1 = clock();
    key = btree_find(cmp_key3, root);
    a2 = clock();
    printf(" tree time %lf, ", ((double) (a2 - a1)) / (CLOCKS_PER_SEC / 1000000));
    a1 = clock();
    str = g_hash_table_lookup(hash, num);
    a2 = clock();
    printf("glib time %lf, ", ((double) (a2 - a1)) / (CLOCKS_PER_SEC / 1000000));
    printf(" glib find_key:%s, ", str);
    assert(key);
    str = btree_get_str(key);
    assert(str);
    assert(strcmp(str, num) == 0);
    printf(" tree find_key:%s\n", str);


    root = btree_delete(cmp_key1, root);
    assert(root);
    root = btree_delete(cmp_key2, root);
    assert(root);
    root = btree_delete(cmp_key3, root);
    assert(root);
    root = btree_delete(cmp_key4, root);
    assert(root);
    //btree_print(root);

    root = btree_insert(cmp_key1, root);
    assert(root);
    root = btree_insert(cmp_key2, root);
    assert(root);
    root = btree_insert(cmp_key3, root);
    assert(root);
    root = btree_insert(cmp_key4, root);
    assert(root);
    //btree_print(root); 

    return 0; 
}/*End of main()*/ 
