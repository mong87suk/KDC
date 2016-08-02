#ifndef __TREE_H__
#define __TREE_H__

typedef struct _Node Node;

Node *tree_insert(Node *node, int key);
Node *tree_deleteNode(Node *root, int key);

#endif