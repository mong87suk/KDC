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

typedef struct node Node;

Node *insert(int key, Node *node); 
void display(Node *root,int); 
void DelNode(int x, Node *root); 
void search(int x, Node *root); 
KeyStatus ins(Node *r, int x, int* y, Node** u); 
int searchPos(int x,int *key_arr, int n); 
KeyStatus del(Node *r, int x); 

#endif