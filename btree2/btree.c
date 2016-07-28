#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "btree.h"
#include <kdc/Queue.h>

struct _Key {
    char *k;
    int id;
};

struct _Node {
    int n; /* n < M No. of keys in node will always less than order of B tree */
    struct _Key *keys[M-1]; /*array of keys*/
    struct _Node *p[M];   /* (n+1 pointers will be in use) */
};

static void print_node(Node *node) {
    int n = node->n;

    char keys[30*M] = {0x00,};
    char child_addrs[15*M] = {0x00,};

    int i;
    for (i=0; i < n; i++) {
        int end_index = strlen(keys);
        sprintf(&keys[end_index], "%s,", node->keys[i]->k);
    }

    i = 0;
    while (node->p[i] != NULL) {
        int end_index = strlen(child_addrs);
        sprintf(&child_addrs[end_index], "%p,", node->p[i]);
        i++;
    }

    printf("{key:[%-40s]\tmyAddr:%p\tchildAddrs:[%s]}\n", keys, node, child_addrs);
}

Key *new_key(char *k) {
    Key *key = (Key *) malloc(KEY_SIZE);
    if (!key) {
        printf("Failed to make buf\n");
        return NULL;
    }
    key-> id = -1;
    key->k = k;

    return key;
}

Node *insert(Key *key, Node *root) 
{ 
    Node *newnode = NULL; 
    Key *upKey = 0; 
    KeyStatus value; 
    value = ins(root, key, &upKey, &newnode); 
    if (value == Duplicate) { 
        printf("Key already available\n");
    } else if (value == InsertIt) { 
        Node *uproot = root; 
        root = malloc(sizeof(Node)); 
        root->n = 1; 
        root->keys[0] = upKey; 
        root->p[0] = uproot; 
        root->p[1] = newnode;
    }/*End of if */
    return root;
}/*End of insert()*/ 

KeyStatus ins(Node *ptr, Key *key, Key **upKey, Node **newnode) 
{ 
    Node *newPtr = NULL;
    Node *lastPtr = NULL; 
    int pos = 0;
    int i = 0;
    int n,splitPos = 0; 
    Key *newKey = 0;
    Key *lastKey = 0; 
    KeyStatus value;

    if (ptr == NULL) 
    { 
        *newnode = NULL; 
        *upKey = key; 
        return InsertIt; 
    }

    n = ptr->n; 
    pos = searchPos(key, ptr->keys, n);
    if (pos < n) {
        char *k = key->k;
        Key *cmp_key = ptr->keys[pos];
        char *cmp_k = cmp_key->k;

        if (strcmp(k, cmp_k) == 0) {
            return Duplicate;
        }
    }

    value = ins(ptr->p[pos], key, &newKey, &newPtr); 
    if (value != InsertIt) 
        return value; 
    /*If keys in node is less than M-1 where M is order of B tree*/ 
    if (n < M - 1) 
    { 
        pos = searchPos(newKey, ptr->keys, n); 
        /*Shifting the key and pointer right for inserting the new key*/ 
        for (i=n; i>pos; i--) 
        { 
            ptr->keys[i] = ptr->keys[i-1]; 
            ptr->p[i+1] = ptr->p[i]; 
        } 
        /*Key is inserted at exact location*/ 
        ptr->keys[pos] = newKey; 
        ptr->p[pos+1] = newPtr; 
        ++ptr->n; /*incrementing the number of keys in node*/ 
        return Success; 
    }/*End of if */ 
    /*If keys in nodes are maximum and position of node to be inserted is last*/ 
    if (pos == M - 1) 
    { 
        lastKey = newKey; 
        lastPtr = newPtr; 
    } 
    else /*If keys in node are maximum and position of node to be inserted is not last*/ 
    { 
        lastKey = ptr->keys[M-2]; 
        lastPtr = ptr->p[M-1]; 
        for (i=M-2; i>pos; i--) 
        { 
            ptr->keys[i] = ptr->keys[i-1]; 
            ptr->p[i+1] = ptr->p[i]; 
        } 
        ptr->keys[pos] = newKey; 
        ptr->p[pos+1] = newPtr; 
    } 
    splitPos = (M - 1)/2; 
       (*upKey) = ptr->keys[splitPos]; 

    (*newnode)=malloc(sizeof(Node));/*Right node after split*/
    int key_count = ptr->n;
    ptr->n = splitPos; /*No. of keys for left splitted node*/ 
    (*newnode)->n = M-1-splitPos;/*No. of keys for right splitted node*/ 
    for (i=0; i < (*newnode)->n; i++) 
    { 
        (*newnode)->p[i] = ptr->p[i + splitPos + 1]; 
        if(i < (*newnode)->n - 1) 
            (*newnode)->keys[i] = ptr->keys[i + splitPos + 1]; 
        else 
            (*newnode)->keys[i] = lastKey; 
    }
    
    for (i = key_count - 1; i >= splitPos; i--) {
        ptr->keys[i] = NULL; 
    }

    (*newnode)->p[(*newnode)->n] = lastPtr; 
    return InsertIt; 
}/*End of ins()*/ 

void display(Node *ptr, int blanks) 
{
    char *k;

    Key *key;

    if (ptr) 
    { 
        int i; 
        for(i = 1;i <= blanks; i++) {
            printf(" "); 
        }

        for (i = 0; i < ptr->n; i++) { 
            key = ptr->keys[i];
            k = key->k;
            printf("%s", k);
            printf(" ");
        }

        printf("\n"); 
        for (i = 0; i <= ptr->n; i++) { 
            display(ptr->p[i], blanks + 10);
        } 
    }/*End of if*/ 
}/*End of display()*/ 

void find(Key *key, Node *root) 
{ 
    int pos, i, n;
    char *k;

    Node *ptr = root;
    Key *cmp_key;

    printf("Search path:\n"); 
    while (ptr) 
    { 
        n = ptr->n; 
        for (i = 0; i < ptr->n; i++) {
            cmp_key = ptr->keys[i];
            k = cmp_key->k;
            printf("%s", k);
            printf(" ");
        } 
        printf("\n"); 

        pos = searchPos(key, ptr->keys, n);

        if (pos < n) {
            k = key->k;
            cmp_key = ptr->keys[pos];
            char *cmp_k = cmp_key->k;

            if (strcmp(k, cmp_k) == 0) {
                printf("Key found in position %d key: ", i);
                printf("%s", k);
                printf("\n");
                return;
            }
        }

        ptr = ptr->p[pos]; 
    } 
    printf("Key is not available Key:");
    k = key->k;
    printf("%s", k);
    printf("\n");
}/*End of search()*/ 

int searchPos(Key *key, Key **key_arr, int n) 
{ 
    int pos = 0;
    char *k;
    char *cmp_k;

    Key *cmp_key = NULL;
    
    while (pos < n) {
        k = key->k;
        cmp_key = key_arr[pos];
        cmp_k = cmp_key->k;
        if (strcmp(k, cmp_k) > 0) {
            pos++;
        } else {
            break;
        }
    }
    return pos; 
}/*End of searchPos()*/

void DelNode(Key *key, Node *root) 
{
    char *k = NULL;

    Node *uproot; 
    KeyStatus value; 
    value = del(root,key); 
    switch (value) 
    { 
    case SearchFailure:     
        k = key->k;
        printf("%s", k);
        printf("\n"); 
        break; 
    case LessKeys: 
        uproot = root; 
        root = root->p[0]; 
        free(uproot); 
        break;
    case Success:
        printf("Succes\n");
        break;
    default:
        printf("failed:%d\n", value);
    }// End of switch
}/*End of delnode()*/  


KeyStatus del(Node *ptr, Key *key) 
{ 
    int pos, i, pivot, n ,min; 
    Key **key_arr; 
    KeyStatus value; 
    Node **p,*lptr,*rptr;
    Node *root = ptr; 

    if (ptr == NULL) 
        return SearchFailure; 
    /*Assigns values of node*/ 
    n=ptr->n; 
    key_arr = ptr->keys; 
    p = ptr->p; 
    min = (M - 1)/2;/*Minimum number of keys*/ 

    pos = searchPos(key, key_arr, n); 
    if (p[0] == NULL) 
    { 
        if (pos == n) {
            return SearchFailure;
        } else { 
            char *k = key->k;
            Key *cmp_key = ptr->keys[pos];
            char *cmp_k = cmp_key->k;

            if (strcmp(k, cmp_k) < 0) {
                return SearchFailure;
            }
        }
        /*Shift keys and pointers left*/ 
        for (i=pos+1; i < n; i++) 
        { 
            key_arr[i-1] = key_arr[i]; 
            p[i] = p[i+1]; 
        } 
        return  --ptr->n >= (ptr==root ? 1 : min) ? Success : LessKeys; 
    }/*End of if */ 

    if (pos < n && key == key_arr[pos]) 
    { 
        Node *qp = p[pos], *qp1; 
        int nkey; 
        while(1) 
        { 
            nkey = qp->n; 
            qp1 = qp->p[nkey]; 
            if (qp1 == NULL) 
                break; 
            qp = qp1; 
        }/*End of while*/ 
        key_arr[pos] = qp->keys[nkey-1]; 
        qp->keys[nkey - 1] = key; 
    }/*End of if */ 
    value = del(p[pos], key); 
    if (value != LessKeys) 
        return value; 

    if (pos > 0 && p[pos-1]->n > min) 
    { 
        pivot = pos - 1; /*pivot for left and right node*/ 
        lptr = p[pivot]; 
        rptr = p[pos]; 
        /*Assigns values for right node*/ 
        rptr->p[rptr->n + 1] = rptr->p[rptr->n]; 
        for (i=rptr->n; i>0; i--) 
        { 
            rptr->keys[i] = rptr->keys[i-1]; 
            rptr->p[i] = rptr->p[i-1]; 
        } 
        rptr->n++; 
        rptr->keys[0] = key_arr[pivot]; 
        rptr->p[0] = lptr->p[lptr->n]; 
        key_arr[pivot] = lptr->keys[--lptr->n]; 
        return Success; 
    }/*End of if */ 
    if (pos<n && p[pos+1]->n > min) 
    { 
        pivot = pos; /*pivot for left and right node*/ 
        lptr = p[pivot]; 
        rptr = p[pivot+1]; 
        /*Assigns values for left node*/ 
        lptr->keys[lptr->n] = key_arr[pivot]; 
        lptr->p[lptr->n + 1] = rptr->p[0]; 
        key_arr[pivot] = rptr->keys[0]; 
        lptr->n++; 
        rptr->n--; 
        for (i=0; i < rptr->n; i++) 
        { 
            rptr->keys[i] = rptr->keys[i+1]; 
            rptr->p[i] = rptr->p[i+1]; 
        }/*End of for*/ 
        rptr->p[rptr->n] = rptr->p[rptr->n + 1]; 
        return Success; 
    }/*End of if */ 

    if(pos == n) 
        pivot = pos-1; 
    else 
        pivot = pos; 

    lptr = p[pivot]; 
    rptr = p[pivot+1]; 
    /*merge right node with left node*/ 
    lptr->keys[lptr->n] = key_arr[pivot]; 
    lptr->p[lptr->n + 1] = rptr->p[0]; 
    for (i=0; i < rptr->n; i++) 
    { 
        lptr->keys[lptr->n + 1 + i] = rptr->keys[i]; 
        lptr->p[lptr->n + 2 + i] = rptr->p[i+1]; 
    } 
    lptr->n = lptr->n + rptr->n +1; 
    free(rptr); /*Remove right node*/ 
    for (i=pos+1; i < n; i++) 
    { 
        key_arr[i-1] = key_arr[i]; 
        p[i] = p[i+1]; 
    } 
    return  --ptr->n >= (ptr == root ? 1 : min) ? Success : LessKeys; 
}/*End of del()*/

void search(Node *root) {
    if (!root) {
        printf("Can't search\n");
        return;
    }
    Queue* q = queue_new();
    if (!q) {
        printf("Failed to new queue\n");
    }
    int result = push(q, root);
    if (!result) {
        printf("Failed to push\n");
    }
    
    Node *node = NULL;
    int i = 0;
    printf("--------------------------print start------------------------------------\n");
    while (empty(q)) {
        node = (Node *) pop(q);
        print_node(node);
        i = 0;
        while (node->p[i] != NULL) {
            push(q, node->p[i]);
            i++;
        }
    }
}
