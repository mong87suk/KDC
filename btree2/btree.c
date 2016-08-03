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

static Node *new_node() {
    Node *node = (Node *) calloc(1, sizeof(Node));
    if (!node) {
        printf("Failed to new node\n");
        return NULL;
    }
    return node;
}

static void destroy_node(Node *node) {
    free(node);
}

static int btree_get_pos(Key *key, Key **key_arr, int n) {
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
}

static KeyStatus btree_ins(Node *ptr, Key *key, Key **upKey, Node **newnode) {
    Node *newPtr = NULL;
    Node *lastPtr = NULL;
    int pos = 0;
    int i = 0;
    int n,splitPos = 0;
    Key *newKey = 0;
    Key *lastKey = 0;
    KeyStatus value;

    if (ptr == NULL) {
        *newnode = NULL;
        *upKey = key;
        return InsertIt;
    }

    n = ptr->n;
    pos = btree_get_pos(key, ptr->keys, n);
    if (pos < n) {
        char *k = key->k;
        Key *cmp_key = ptr->keys[pos];
        char *cmp_k = cmp_key->k;

        if (strcmp(k, cmp_k) == 0) {
            return Duplicate;
        }
    }

    value = btree_ins(ptr->p[pos], key, &newKey, &newPtr);
    if (value != InsertIt) {
        return value;
    }
    /*If keys in node is less than M-1 where M is order of B tree*/
    if (n < M - 1) {
        pos = btree_get_pos(newKey, ptr->keys, n);
        /*Shifting the key and pointer right for inserting the new key*/
        for (i=n; i>pos; i--) {
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
    if (pos == M - 1) {
        lastKey = newKey;
        lastPtr = newPtr;
    }
    else {/*If keys in node are maximum and position of node to be inserted is not last*/
        lastKey = ptr->keys[M-2];
        lastPtr = ptr->p[M-1];
        for (i=M-2; i>pos; i--) {
            ptr->keys[i] = ptr->keys[i-1];
            ptr->p[i+1] = ptr->p[i];
        }
        ptr->keys[pos] = newKey;
        ptr->p[pos+1] = newPtr;
    }
    splitPos = (M - 1)/2;
    (*upKey) = ptr->keys[splitPos];

    (*newnode) = new_node();/*Right node after split*/
    if (!(*newnode)) {
        return Failure;
    }

    int key_count = ptr->n;
    ptr->n = splitPos; /*No. of keys for left splitted node*/
    (*newnode)->n = M-1-splitPos;/*No. of keys for right splitted node*/
    for (i = 0; i < (*newnode)->n; i++)
    { 
        (*newnode)->p[i] = ptr->p[i + splitPos + 1];
        ptr->p[i + splitPos + 1] = NULL;
        if (i < (*newnode)->n - 1) {
            (*newnode)->keys[i] = ptr->keys[i + splitPos + 1];
            ptr->keys[i + splitPos + 1] = NULL;
        } else {
            (*newnode)->keys[i] = lastKey;
        }
    }

    for (i = key_count - 1; i >= splitPos; i--) {
        ptr->keys[i] = NULL;
    }

    (*newnode)->p[(*newnode)->n] = lastPtr;
    return InsertIt; 
}

static void btree_print_node(Node *node) {
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

static KeyStatus btree_get_del_result(Node *ptr, Key *key)
{ 
    int pos, i, pivot, n ,min;
    Key **key_arr;
    KeyStatus value;
    Node **p,*lptr,*rptr;
    Node *root = ptr;

    if (ptr == NULL) {
        return Failure;
    }
    /*Assigns values of node*/
    n=ptr->n;
    key_arr = ptr->keys;
    p = ptr->p;
    min = (M - 1)/2;/*Minimum number of keys*/

    pos = btree_get_pos(key, key_arr, n);
    if (p[0] == NULL) {
        if (pos == n) {
            return Failure;
        } else { 
            char *k = key->k;
            Key *cmp_key = ptr->keys[pos];
            char *cmp_k = cmp_key->k;

            if (strcmp(k, cmp_k) < 0) {
                return Failure;
            }
        }
        /*Shift keys and pointers left*/
        --ptr->n;
        if (++pos < n) {
            for (i = pos; i < n; i++) {
                if (strcmp(key_arr[i -1]->k, key->k) == 0) {
                    destroy_key(key_arr[i -1]);
                } 
                key_arr[i-1] = key_arr[i];
                p[i] = p[i+1];
            }
        } else if (strcmp(key_arr[ptr->n]->k, key->k) == 0) {
            destroy_key(key_arr[ptr->n]);
        }
        key_arr[ptr->n] = NULL;
        return  ptr->n >= (ptr == root ? 1 : min) ? Success : LessKeys;
    }/*End of if */

    if (pos < n && (strcmp(key->k, key_arr[pos]->k) == 0)) {
        Node *qp = p[pos], *qp1;
        Key *tmp_key;

        int nkey;
        while(1) {
            nkey = qp->n;
            qp1 = qp->p[nkey];
            if (qp1 == NULL)
                break;
            qp = qp1;
        }/*End of while*/

        tmp_key = key_arr[pos];
        key_arr[pos] = qp->keys[nkey - 1];
        qp->keys[nkey - 1] = tmp_key; 
    }/*End of if */
    value = btree_get_del_result(p[pos], key);
    if (value != LessKeys) {
        return value;
    }

    if (pos > 0 && p[pos-1]->n > min) {
        pivot = pos - 1; /*pivot for left and right node*/
        lptr = p[pivot];
        rptr = p[pos];
        /*Assigns values for right node*/
        rptr->p[rptr->n + 1] = rptr->p[rptr->n];
        for (i=rptr->n; i>0; i--) {
            rptr->keys[i] = rptr->keys[i-1];
            rptr->p[i] = rptr->p[i-1];
        }
        rptr->n++;
        rptr->keys[0] = key_arr[pivot];
        rptr->p[0] = lptr->p[lptr->n];
        key_arr[pivot] = lptr->keys[--lptr->n];
        lptr->keys[--lptr->n] = NULL;
        return Success;
    }/*End of if */
    if (pos<n && p[pos+1]->n > min) {
        pivot = pos; /*pivot for left and right node*/
        lptr = p[pivot];
        rptr = p[pivot+1];
        /*Assigns values for left node*/
        lptr->keys[lptr->n] = key_arr[pivot];
        lptr->p[lptr->n + 1] = rptr->p[0];
        key_arr[pivot] = rptr->keys[0];
        lptr->n++;
        rptr->n--;
        for (i=0; i < rptr->n; i++) {
            rptr->keys[i] = rptr->keys[i+1];
            rptr->p[i] = rptr->p[i+1];
        }/*End of for*/
        rptr->p[rptr->n] = rptr->p[rptr->n + 1];
        return Success;
    }/*End of if */

    if (pos == n) {
        pivot = pos-1;
    }
    else {
        pivot = pos;
    } 

    lptr = p[pivot];
    rptr = p[pivot+1];
    /*merge right node with left node*/
    lptr->keys[lptr->n] = key_arr[pivot];
    key_arr[pivot] = NULL;
    lptr->p[lptr->n + 1] = rptr->p[0];
    for (i=0; i < rptr->n; i++)
    {
        lptr->keys[lptr->n + 1 + i] = rptr->keys[i];
        lptr->p[lptr->n + 2 + i] = rptr->p[i+1];
    }
    lptr->n = lptr->n + rptr->n +1;
    p[pivot+1] = NULL;
    destroy_node(rptr); /*Remove right node*/
    for (i = pos+1; i < n; i++) {
        key_arr[i-1] = key_arr[i];
        p[i] = p[i+1]; 
    }
    return --ptr->n >= (ptr == root ? 1 : min) ? Success : LessKeys;
}

Key *new_key(char *k) {
    Key *key = (Key *) malloc(sizeof(Key));
    if (!key) {
        printf("Failed to make buf\n");
        return NULL;
    }
    key-> id = -1;
    key->k = strdup(k);

    return key;
}

void destroy_key(Key *key) {
    if (!key) {
        printf("Can't delete key\n");
        return;
    }
    free(key->k);
    free(key);
}

char *btree_get_str(Key *key) {
    if (!key) {
        printf("Can't get the str\n");
        return NULL;
    }
    return key->k;
}

Node *btree_insert(Key *key, Node *root)
{ 
    Node *newnode = NULL;
    Key *upKey = 0;
    KeyStatus value;
    value = btree_ins(root, key, &upKey, &newnode);
    if (value == Duplicate) {
        printf("Key already available\n");
    } else if (value == InsertIt) {
        Node *uproot = root;
        root = new_node();
        if (root) {
            root->n = 1;
            root->keys[0] = upKey;
            root->p[0] = uproot;
            root->p[1] = newnode;
        }
    }
    /*End of if */
    return root;
}/*End of insert()*/
 

Key *btree_find(Key *key, Node *root)
{ 
    int pos, n;
    char *k;

    Node *ptr = root;
    Key *cmp_key = NULL;
    char *cmp_k;

    while (ptr) {
        n = ptr->n;
        pos = btree_get_pos(key, ptr->keys, n);
        if (pos < n) {
            k = key->k;
            cmp_key = ptr->keys[pos];
            cmp_k = cmp_key->k;

            if (strcmp(k, cmp_k) == 0) {
                return cmp_key;
            }
        }
        ptr = ptr->p[pos];
    } 
    printf("Key is not available Key: %s\n", key->k);
    return NULL;
}/*End of search()*/

Node *btree_delete(Key *key, Node *root)
{
    Node *uproot;
    KeyStatus value;
    value = btree_get_del_result(root,key);
    switch (value)
    {
    case Failure:
        printf("Find Faile\n");
        break;
    case LessKeys:
        uproot = root;
        root = root->p[0];
        destroy_node(uproot);
        break;
    case Success:
        break;
    default:
        printf("Can't delte\n");
    }
    return root;
}
/*End of del()*/

void btree_print(Node *root) {
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
        btree_print_node(node);
        i = 0;
        while (node->p[i] != NULL) {
            push(q, node->p[i]);
            i++;
        }
    }
}
