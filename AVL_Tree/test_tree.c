#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "tree.h"

int main() {
  
  int value;
  char *str;
  char *key = (char *) calloc(15, sizeof(char));

  Tree *tree = new_tree(1);
  assert(tree);
  GHashTable *hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
  
  printf("insert start\n");
  for (int i = 1; i <= 10; i++) {
      sprintf(key, "%d", i);
      tree_insert(tree, key, i);
      g_hash_table_insert(hash, g_strdup(key), g_strdup(key));
  }
  printf("insert finish\n");
  tree_print(tree);

  tree_delete(tree, "10");
  tree_delete(tree, "9");
  tree_delete(tree, "7");
  tree_print(tree);

  value = tree_find(tree, "10");
  tree_insert(tree, "10", 10);
  tree_insert(tree, "9", 9);
  tree_insert(tree, "7", 7);
  tree_print(tree);

  destroy_tree(tree);

  printf("insert start\n");
  Tree *tree2 = new_tree(2);
  for (int i = 1; i <= 10000000; i++) {
      sprintf(key, "%d", i);
      tree_insert(tree2, key, i);
      g_hash_table_insert(hash, g_strdup(key), g_strdup(key));
  }
  printf("insert finish\n");
  sprintf(key, "%d", 100);
  clock_t a1 = clock();
  value = tree_find(tree2, key);
  clock_t a2 = clock();
  printf(" tree time %lf, ", ((double) (a2 - a1)) / (CLOCKS_PER_SEC / 1000000));
  a1 = clock();
  str = g_hash_table_lookup(hash, key);
  a2 = clock();
  printf("glib time %lf, ", ((double) (a2 - a1)) / (CLOCKS_PER_SEC / 1000000));
  printf(" glib find_key:%s, ", key);
  assert(value == 100);
  printf(" tree find_value:%d\n", value);

  sprintf(key, "%d",200);
  a1 = clock();
  value = tree_find(tree2, key);
  a2 = clock();
  printf(" tree time %lf, ", ((double) (a2 - a1)) / (CLOCKS_PER_SEC / 1000000));
  a1 = clock();
  str = g_hash_table_lookup(hash, key);
  a2 = clock();
  printf("glib time %lf, ", ((double) (a2 - a1)) / (CLOCKS_PER_SEC / 1000000));
  printf(" glib find_key:%s, ", str);
  assert(value);
  assert(value == 200);
  printf(" tree find_key:%d\n", value);


  sprintf(key, "%d", 205);
  a1 = clock();
  value = tree_find(tree2, key);
  a2 = clock();
  printf(" tree time %lf, ", ((double) (a2 - a1)) / (CLOCKS_PER_SEC / 1000000));
  a1 = clock();
  str = g_hash_table_lookup(hash, key);
  a2 = clock();
  printf("glib time %lf, ", ((double) (a2 - a1)) / (CLOCKS_PER_SEC / 1000000));
  printf(" glib find_key:%s, ", str);
  assert(value);
  assert(value == 205);
  printf(" tree find_key:%d\n", value);
  
  sprintf(key, "%d", 99999);
  a1 = clock();
  value = tree_find(tree2, key);
  a2 = clock();
  printf(" tree time %lf, ", ((double) (a2 - a1)) / (CLOCKS_PER_SEC / 1000000));
  a1 = clock();
  str = g_hash_table_lookup(hash, key);
  a2 = clock();
  printf("glib time %lf, ", ((double) (a2 - a1)) / (CLOCKS_PER_SEC / 1000000));
  printf(" glib find_key:%s, ", str);
  assert(value);
  assert(value == 99999);
  printf(" tree find_key:%d\n", value);

  sprintf(key, "%d", 9999999);
  a1 = clock();
  value = tree_find(tree2, key);
  a2 = clock();
  printf(" tree time %lf, ", ((double) (a2 - a1)) / (CLOCKS_PER_SEC / 1000000));
  a1 = clock();
  str = g_hash_table_lookup(hash, key);
  a2 = clock();
  printf("glib time %lf, ", ((double) (a2 - a1)) / (CLOCKS_PER_SEC / 1000000));
  printf(" glib find_key:%s, ", str);
  assert(value);
  assert(value == 9999999);
  printf(" tree find_key:%d\n", value);
  tree_print(tree2);

  return 0;
}