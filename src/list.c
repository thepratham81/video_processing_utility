#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Node *new_node(){
    Node *node = malloc(sizeof(*node));

    if(!node){
        fprintf(stderr,"Unable to allocate memory\n");
        return NULL;
    }
    memset(node,0, sizeof(*node));
    return node;
}


void list_append(List *list,char *data){
    Node *node = new_node();
    if(!node){
        fprintf(stderr,"Unable to allocate memory\n");
        return;
    }
    
    node->data = data;

    if(!list->head){
        list->head = node;
        list->tail = node;
    }else{
        Node *temp = list->tail;
        list->tail->next = node;
        list->tail = node;
        node->pre = temp;
    }
}


void list_pop(List *list, Node *node) {
    if (!node) node = list->tail;  // If node is NULL, remove the last item
    if (!node) return;             // List is empty

    if (list->head == list->tail) {
        list->head = NULL;
        list->tail = NULL;
    } else if (node == list->head) {
        list->head = node->next;
        list->head->pre = NULL;
    } else if (node == list->tail) {
        list->tail = node->pre;
        list->tail->next = NULL;
    } else {
        node->pre->next = node->next;
        node->next->pre = node->pre;
    }
    
    if(list->free){
        list->free(node->data);
    }

    free(node);
}


#ifdef TEST

void static print_list(List *list){
    if(!list->head) return;
    Node *node = list->head;
    while(node){
        Node *pre = node->pre;
        if(pre){
            printf("pre = %s ",pre->data);
        }
        puts(node->data);
        node = node->next;
    }
}
int main(void){
    List list = {0};
    list_append(&list,"data1");
    list_append(&list,"data2");
    list_append(&list,"data3");
    list_append(&list,"data4");
    list_pop(&list,NULL);
    list_pop(&list,list.head);
    print_list(&list);
}
#endif // TEST


