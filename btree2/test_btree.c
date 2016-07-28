#define _GNU_SOURCE
#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include "btree.h"

static char *new_str(char *str) {
    int len = strlen(str);
    char *new_str = (char*) calloc(len + 1, sizeof(char));
    if (!new_str) {
        printf("Failed to make the buf\n");
        return NULL;
    }
    
    strcpy(new_str, str);

    return new_str;    
}

int main() 
{ 
    Node *root;
    long int n;
    root = NULL;
    char *str;
    char *num = (char *) calloc(2, sizeof(char));
    Key *key;

    for (int i = 0; i < 9; i++) {
        n = random() % 100;
        printf("n:%ld \n", n);
        sprintf(num, "%ld", n);
        str = new_str(num);
        key = new_key(str);
        root = insert(key, root);
    }
    //display(root,0);
    /*
    DelNode(3, root);
    display(root,0);*/

    //find(key5, root); 
    //find(key3, root);

    search(root);

    return 0; 
}/*End of main()*/ 