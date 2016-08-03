#include <stdio.h>
#include <assert.h>
#include "tree.h"

int main() {
  Tree *tree;

  tree = new_tree(1);
  assert(tree);

  tree_insert(tree, 9);
  tree_insert(tree, 5);
  tree_insert(tree, 10);
  tree_insert(tree, 0);
  tree_insert(tree, 6);
  tree_insert(tree, 11);
  tree_insert(tree, -1);
  tree_insert(tree, 1);
  tree_insert(tree, 2);

  tree_delete(tree, 10);

  preOrder(tree);
  return 0;
}