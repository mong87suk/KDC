#ifndef __B_TREE_H__
#define __B_TREE_H__

#define M 3

typedef enum { 
    Duplicate = 0,
    SearchFailure =1,
    Success = 2,
    InsertIt = 3,
    LessKeys = 4 
} KeyStatus; 

typedef struct _Key Key;
typedef struct _Node Node;

Node *insert(Key *key, Node *node); 
void display(Node *root,int); 
void DelNode(int x, Node *root); 
void search(int x, Node *root); 
KeyStatus ins(Node *ptr, Key *key, Key **upKey, Node **newnode); 
int searchPos(Key *key, Key **key_arr, int n);
KeyStatus del(Node *r, int x); 

#endif