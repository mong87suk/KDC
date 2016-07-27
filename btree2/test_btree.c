#define _GNU_SOURCE
#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include "btree.h"

static char *new_str(char *str) {
    int len = strlen(str);
    char *new_str = (char*) malloc(4 + len);
    if (!new_str) {
        printf("Failed to make the buf\n");
        return NULL;
    }
    char *tmp = new_str;

    tmp = mempcpy(tmp, &len, 4);
    memcpy(tmp, str, 4);

    return new_str;    
}

int main() 
{ 
    Node *root;

    root = NULL;
    char *str;
    Key *key1, *key2, *key3, *key4, *key5, *key6;

    str = new_str("ace");
    key1 = new_key(str);
    root = insert(key1, root);

    str = new_str("bus");
    key2 = new_key(str);
    root = insert(key2, root);

    str = new_str("car");
    key3 = new_key(str);
    root = insert(key3, root);

    str = new_str("death");
    key4 = new_key(str);
    root = insert(key4, root);

    str = new_str("ebs");
    key5 = new_key(str);
    root = insert(key5, root); 

    str = new_str("food");
    key6 = new_key(str);
    root = insert(key6, root);


    display(root,0);
    /*
    DelNode(3, root);
    display(root,0);*/

    find(key5, root); 
    find(key3, root);  

    return 0; 
}/*End of main()*/ 