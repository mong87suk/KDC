#ifndef __TREE_H__
#define __TREE_H__

#define ADDRESS_SIZE 15
#define CHILD_NUM     2

typedef struct _Node Node;
typedef struct _Tree Tree;

void tree_insert(Tree *tree, char *key, int value);
void tree_delete(Tree *tree, char *key);
void tree_print(Tree *tree);
int tree_find(Tree *tree, char *key);
Tree *new_tree(int index);
void destroy_tree(Tree *tree);
int tree_get_index(Tree *tree);
void tree_update(Tree *tree, char *key, int id);

#endif