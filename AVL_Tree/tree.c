#include <stdio.h>
#include <stdlib.h>

#include "tree.h"

struct _Node
{
    int key;
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
static Node *newNode(int key)
{
    Node *node = (Node *) malloc(sizeof(Node));
    node->key   = key;
    node->left   = NULL;
    node->right  = NULL;
    node->height = 1;  // new node is initially added at leaf
    return(node);
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

static Node *tree_insert_internal(Node *node, int key)
{
    /* 1.  Perform the normal BST rotation */
    if (node == NULL)
        return(newNode(key));
 
    if (key < node->key)
        node->left = tree_insert_internal(node->left, key);
    else
        node->right = tree_insert_internal(node->right, key);
 
    /* 2. Update height of this ancestor node */
    node->height = tree_max(tree_height(node->left), tree_height(node->right)) + 1;
 
    /* 3. Get the balance factor of this ancestor node to check whether
       this node became unbalanced */
    int balance = tree_getBalance(node);
 
    // If this node becomes unbalanced, then there are 4 cases
 
    // Left Left Case
    if (balance > 1 && key < node->left->key)
        return tree_rightRotate(node);
 
    // Right Right Case
    if (balance < -1 && key > node->right->key)
        return tree_leftRotate(node);
 
    // Left Right Case
    if (balance > 1 && key > node->left->key)
    {
        node->left =  tree_leftRotate(node->left);
        return tree_rightRotate(node);
    }
 
    // Right Left Case
    if (balance < -1 && key < node->right->key)
    {
        node->right = tree_rightRotate(node->right);
        return tree_leftRotate(node);
    }
 
    /* return the (unchanged) node pointer */
    return node;
}
 
static Node *tree_delete_internal(Node *root, int key)
{
    // STEP 1: PERFORM STANDARD BST DELETE
 
    if (root == NULL)
        return root;
 
    // If the key to be deleted is smaller than the root's key,
    // then it lies in left subtree
    if (key < root->key)
        root->left = tree_delete_internal(root->left, key);
 
    // If the key to be deleted is greater than the root's key,
    // then it lies in right subtree
    else if( key > root->key )
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
 
            free(temp);
        }
        else
        {
            // node with two children: Get the inorder successor (smallest
            // in the right subtree)
            Node *temp = tree_minValueNode(root->right);
 
            // Copy the inorder successor's data to this node
            root->key = temp->key;
 
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

static void preOrder_internal(Node *root)
{
    if(root != NULL)
    {
        printf("%d ", root->key);
        preOrder_internal(root->left);
        preOrder_internal(root->right);
    }
}

static void tree_print_node(Node *node) {
    sprintf(&child_addrs[0], "%p,", node->left);
    sprintf(&child_addrs[1], "%p,", node->right);
        
    printf("{key:[%-40s]\tmyAddr:%p\tchildAddrs:[%s]}\n", node->key, node, child_addrs);
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

void tree_delete(Tree *tree, int key) {
    if (!tree) {
        printf("Can't delete\n");
        return;
    }

    tree->node = tree_delete_internal(tree->node, key);
}

void tree_insert(Tree *tree, int key) {
    if (!tree) {
        printf("Can't insert key\n");
        return;
    }

    tree->node = tree_insert_internal(tree->node, key);
}

void preOrder(Tree *tree) {
    preOrder_internal(tree->node);
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
    int i = 0;
    printf("--------------------------print start------------------------------------\n");
    while (empty(q)) {
        node = (Node *) pop(q);
        tree_print_node(node);
        i = 0;
        if (node->left) {
            push(q, node->left);
        }
        while (node->p[i] != NULL) {
            push(q, node->p[i]);
            i++;
        }
    }
}
