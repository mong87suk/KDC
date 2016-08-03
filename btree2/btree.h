#ifndef __B_TREE_H__
#define __B_TREE_H__

#define M 7
#define KEY_SIZE 12

typedef enum { 
    Duplicate = 0,
    Failure =1,
    Success = 2,
    InsertIt = 3,
    LessKeys = 4, 
} KeyStatus; 

typedef struct _Key Key;
typedef struct _Node Node;

Node *btree_insert(Key *key, Node *node); 
Node *btree_delete(Key *key, Node *root);
Key *btree_find(Key *key, Node *root); 
Key *new_key(char *k);
void btree_print(Node *root);
void destroy_key(Key *key);
char *btree_get_str(Key *key);

#endif