#include<stdlib.h> 
#include<stdio.h> 
#include "btree.h"

int main() 
{ 
    Node *root;

    root = NULL;
    root = insert(1, root);
    root = insert(2, root);
    root = insert(3, root);
    root = insert(4, root);
    root = insert(5, root); 
    root = insert(6, root);

    display(root,0);
    DelNode(3, root);
    display(root,0);

    search(3, root); 
    search(5, root); 

    return 0; 
}/*End of main()*/ 