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
    Key *cmp_key1;
    Key *cmp_key2;

    for (int i = 0; i < 9; i++) {
        n = random() % 100;
        printf("n:%ld \n", n);
        sprintf(num, "%ld", n);
        str = new_str(num);
        key = new_key(str);
        root = insert(key, root);
    }
    search(root);

    sprintf(num, "%d", 83);
    str = new_str(num);
    cmp_key1 = new_key(str);
    DelNode(cmp_key1, root);
    search(root);

    return 0; 
}/*End of main()*/ 