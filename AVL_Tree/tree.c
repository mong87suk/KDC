#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kdc/Queue.h>

#include "tree.h"

struct _Node
{
    int value;
    char *key;
    Node *left;
    Node *right;
    int height;
};

struct _Tree
{
    int index;
    Node *node;
};
 
// A utility function to get height of the tree
static int tree_height(Node *N)
{
    if (N == NULL)
        return 0;
    return N->height;
}
 
// A utility function to get maximum of two integers
static int tree_max(int a, int b)
{
    return (a > b)? a : b;
}
 
/* Helper function that allocates a new node with the given key and
    NULL left and right pointers. */
static Node *new_Node(char *key, int value)
{
    Node *node = (Node *) malloc(sizeof(Node));
    if (!node) {
        printf("Failed to make the Node\n");
        return NULL;
    }
    
    node->key = strdup(key);
    if (!node->key) {
        printf("Failed to make key\n");
        free(node);
        return NULL;
    }
    node->value = value;
    node->left   = NULL;
    node->right  = NULL;
    node->height = 1;  // new node is initially added at leaf
    return(node);
}

static void destroy_node(Node *node) {
    if (node) {
        free(node->key);
        free(node);
    }
}
 
// A utility function to right rotate subtree rooted with y
// See the diagram given above.
static Node *tree_rightRotate(Node *y)
{
    Node *x = y->left;
    Node *T2 = x->right;
 
    // Perform rotation
    x->right = y;
    y->left = T2;
 
    // Update tree_heights
    y->height = tree_max(tree_height(y->left), tree_height(y->right))+1;
    x->height = tree_max(tree_height(x->left), tree_height(x->right))+1;
 
    // Return new root
    return x;
}
 
// A utility function to left rotate subtree rooted with x
// See the diagram given above.
static Node *tree_leftRotate(Node *x)
{
    Node *y = x->right;
    Node *T2 = y->left;
 
    // Perform rotation
    y->left = x;
    x->right = T2;
 
    //  Update heights
    x->height = tree_max(tree_height(x->left), tree_height(x->right))+1;
    y->height = tree_max(tree_height(y->left), tree_height(y->right))+1;
 
    // Return new root
    return y;
}
 
// Get Balance factor of node N
static int tree_getBalance(Node *N)
{
    if (N == NULL)
        return 0;
    return tree_height(N->left) - tree_height(N->right);
}
 
 
/* Given a non-empty binary search tree, return the node with minimum
   key value found in that tree. Note that the entire tree does not
   need to be searched. */
static Node *tree_minValueNode(Node *node)
{
    Node * current = node;
 
    /* loop down to find the leftmost leaf */
    while (current->left != NULL)
        current = current->left;
 
    return current;
}

static Node *tree_insert_internal(Node *node, char *key, int value)
{
    /* 1.  Perform the normal BST rotation */
    if (node == NULL)
        return(new_Node(key, value));
 
    if (strcmp(key, node->key) < 0) {
        node->left = tree_insert_internal(node->left, key, value);
    } else if (strcmp(key, node->key) > 0) {
        node->right = tree_insert_internal(node->right, key, value);
    } else {
        printf("Can't insert this key:%s\n", key);  
    }
 
    /* 2. Update height of this ancestor node */
    node->height = tree_max(tree_height(node->left), tree_height(node->right)) + 1;
 
    /* 3. Get the balance factor of this ancestor node to check whether
       this node became unbalanced */
    int balance = tree_getBalance(node);
 
    // If this node becomes unbalanced, then there are 4 cases
 
    // Left Left Case
    if (balance > 1 && strcmp(key, node->left->key) < 0)
        return tree_rightRotate(node);
 
    // Right Right Case
    if (balance < -1 && strcmp(key, node->right->key) > 0)
        return tree_leftRotate(node);
 
    // Left Right Case
    if (balance > 1 && strcmp(key, node->left->key) > 0)
    {
        node->left =  tree_leftRotate(node->left);
        return tree_rightRotate(node);
    }
 
    // Right Left Case
    if (balance < -1 && strcmp(key, node->right->key) < 0)
    {
        node->right = tree_rightRotate(node->right);
        return tree_leftRotate(node);
    }
 
    /* return the (unchanged) node pointer */
    return node;
}
 
static Node *tree_delete_internal(Node *root, char *key)
{
    // STEP 1: PERFORM STANDARD BST DELETE
 
    if (root == NULL)
        return root;
 
    // If the key to be deleted is smaller than the root's key,
    // then it lies in left subtree
    if (strcmp(key, root->key) < 0)
        root->left = tree_delete_internal(root->left, key);
 
    // If the key to be deleted is greater than the root's key,
    // then it lies in right subtree
    else if (strcmp(key, root->key) > 0)
        root->right = tree_delete_internal(root->right, key);
 
    // if key is same as root's key, then This is the node
    // to be deleted
    else
    {
        // node with only one child or no child
        if((root->left == NULL) || (root->right == NULL))
        {
            Node *temp = root->left ? root->left : root->right;
 
            // No child case
            if(temp == NULL)
            {
                temp = root;
                root = NULL;
            }
            else // One child case
             *root = *temp; // Copy the contents of the non-empty child
 
            destroy_node(temp);
        }
        else
        {
            // node with two children: Get the inorder successor (smallest
            // in the right subtree)
            Node *temp = tree_minValueNode(root->right);
 
            // Copy the inorder successor's data to this node
            root = temp;
 
            // Delete the inorder successor
            root->right = tree_delete_internal(root->right, temp->key);
        }
    }
 
    // If the tree had only one node then return
    if (root == NULL)
      return root;
 
    // STEP 2: UPDATE HEIGHT OF THE CURRENT NODE
    root->height = tree_max(tree_height(root->left), tree_height(root->right)) + 1;
 
    // STEP 3: GET THE BALANCE FACTOR OF THIS NODE (to check whether
    //  this node became unbalanced)
    int balance = tree_getBalance(root);
 
    // If this node becomes unbalanced, then there are 4 cases
 
    // Left Left Case
    if (balance > 1 && tree_getBalance(root->left) >= 0)
        return tree_rightRotate(root);
 
    // Left Right Case
    if (balance > 1 && tree_getBalance(root->left) < 0)
    {
        root->left =  tree_leftRotate(root->left);
        return tree_rightRotate(root);
    }
 
    // Right Right Case
    if (balance < -1 && tree_getBalance(root->right) <= 0)
        return tree_leftRotate(root);
 
    // Right Left Case
    if (balance < -1 && tree_getBalance(root->right) > 0)
    {
        root->right = tree_rightRotate(root->right);
        return tree_leftRotate(root);
    }
 
    return root;
}

static Node *tree_find_interval(Node *node, char *key) {

    if (node) {  
        if (strcmp(key, node->key) < 0) {
            node = tree_find_interval(node->left, key);
        } else if (strcmp(key, node->key) > 0) { 
            node = tree_find_interval(node->right, key);
        } else if (strcmp(key, node->key) == 0) {
            return node;
        }
    }
    return node; 
}

static void tree_print_node(Node *node) {
    char child_addrs[ADDRESS_SIZE * CHILD_NUM] = {0x00,};
    
    int end_index = strlen(child_addrs);
    sprintf(&child_addrs[end_index], "%p,", node->left);
    end_index = strlen(child_addrs);
    sprintf(&child_addrs[end_index], "%p,", node->right);
        
    printf("{key:[%-40s]\tvalue[%d]\tmyAddr:%p\tchildAddrs:[%s]}\n", node->key, node->value, node, child_addrs);
}

static void destroy_internel(Node *node) {
    if (node != NULL) {
        Node *right_node = node->right;
        destroy_internel(node->left);
        destroy_node(node);
        destroy_internel(right_node);
    }
}

Tree *new_tree(int index) {
    if (index < 0) {
        printf("Can't new tree\n");
        return NULL;
    }

    Tree *tree = (Tree*) malloc(sizeof(Tree));
    tree->node = NULL;
    tree->index = index;

    return tree;
}

void tree_delete(Tree *tree, char *key) {
    if (!tree) {
        printf("Can't delete\n");
        return;
    }

    tree->node = tree_delete_internal(tree->node, key);
}

void tree_insert(Tree *tree, char *key, int value) {
    if (!tree) {
        printf("Can't insert key\n");
        return;
    }

    tree->node = tree_insert_internal(tree->node, key, value);
}

int tree_find(Tree *tree, char *key) {
    if (!tree || !key) {
        printf("Can't search this key:%s\n", key);
        return -1;
    }

    if (!strlen(key)) {
        printf("Can't search this key:%s\n", key);
        return -1;
    }

    Node *node = tree_find_interval(tree->node, key);
    if (!node) {
        printf("Can't search this key:%s\n", key);
        return -1;
    } 
    return node->value; 
}

void tree_print(Tree *tree) {
    if (!tree) {
        printf("Can't print tree\n");
        return;
    }
    Queue* q = queue_new();
    if (!q) {
        printf("Failed to new queue\n");
    }
    int result = push(q, tree->node);
    if (!result) {
        printf("Failed to push\n");
    }

    Node *node = NULL;
    printf("--------------------------print start------------------------------------\n");
    while (empty(q)) {
        node = (Node *) pop(q);
        tree_print_node(node);
        
        if (node->left) {
            push(q, node->left);
        }
        if (node->right) {
           push(q, node->right); 
        }
    }
}

void destroy_tree(Tree *tree) {
    if (!tree) {
        printf("Can't destroy tree\n");
        return;
    }

    destroy_internel(tree->node); 
    free(tree);
}

int tree_get_index(Tree *tree) {
    if (!tree) {
        printf("Can't get index\n");
        return -1;
    }

    return tree->index;
}

void tree_update(Tree *tree, char *key, int id) {
    if (tree) {
        printf("Can't update key\n");
        return;
    }
    tree_delete(tree, key);
    tree_insert(tree, key, id);
}