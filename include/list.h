#ifndef _INCLUDE_LIST_H

typedef struct Node {
    char *data;
    struct Node *pre;
    struct Node *next;
}Node;

typedef struct{
    struct Node *head;
    struct Node *tail;
    void (*free)(void *);
}List;

Node *new_node();
void list_append(List *list,char *data);
void list_pop(List *list, Node *node);

#endif // _INCLUDE_LIST_H
