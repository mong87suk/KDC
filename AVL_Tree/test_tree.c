#include "tree.h"

int main()
{
  Node *root = NULL;
 
  /* Constructing tree given in the above figure */
    root = tree_insert(root, 9);
    root = tree_insert(root, 5);
    root = tree_insert(root, 10);
    root = tree_insert(root, 0);
    root = tree_insert(root, 6);
    root = tree_insert(root, 11);
    root = tree_insert(root, -1);
    root = tree_insert(root, 1);
    root = tree_insert(root, 2);

    root = tree_delete(root, 10);
 
    /* The AVL Tree after deletion of 10
            1
           /  \
          0    9
        /     /  \
       -1    5     11
           /  \
          2    6
    */
    preOrder(root);
    return 0;
}