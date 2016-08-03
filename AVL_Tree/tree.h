#ifndef __TREE_H__
#define __TREE_H__

typedef struct _Node Node;
typedef struct _Tree Tree;

void tree_insert(Tree *tree, int key);
void tree_delete(Tree *tree, int key);
void preOrder(Tree *tree);
Tree *new_tree(int index);

#endif