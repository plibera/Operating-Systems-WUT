#ifndef UTILS_H
#define UTILS_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>

#define N 4
#define MSG_LEN 100


#define STANDARD 0
#define VIP 1


char verb[6][8] = {" like", " love", " hate", " prefer", " admire", " fear"};
char noun[19][14] = {" kittens", " dogs", " spiders", " scorpions", " elephants", " lions",
                 " giraffes", " snakes", " tigers", " fish", " dolphins", " pigeons", " mice",
                   " rats", " deers", " birds", " bees", " wasps", " ants"};

char* random_text()
{
    char *newText = calloc(MSG_LEN, sizeof(char));
    newText[0] = 'I';
    int x = rand()%6;
    int l = strlen(newText);
    strcpy(newText+l, verb[x]);
    x = rand()%19;
    l = strlen(newText);
    strcpy(newText+l, noun[x]);
    char middle[7] = " and I";
    l = strlen(newText);
    strcpy(newText+l, middle);
    x = rand()%6;
    l = strlen(newText);
    strcpy(newText+l, verb[x]);
    x = rand()%19;
    l = strlen(newText);
    strcpy(newText+l, noun[x]);
    return newText;
}


sem_t mutex;
sem_t full;
sem_t empty;

typedef struct Message{
    char text[MSG_LEN];
    int priority;
}Message;

typedef struct Node{
    Message* msg;
    struct Node* next;
}Node;

typedef struct List{
    struct Node* head;
    struct Node* tail;
    int size;
    int errors;
}List;


List* buffer;

int init(List** list)
{
    if(*list != NULL)
    {
        if((*list)->head != NULL)
            clear(*list);
    }
    else
    {
        *list = malloc(sizeof(struct List));
        if(list == NULL)
        {
          return 1;
        }
    }

    (*list)->head = NULL;
    (*list)->tail = NULL;
    (*list)->size = 0;
    (*list)->errors = 0;

    sem_init(&mutex, 0, 1);
    sem_init(&full, 0, N);
    sem_init(&empty, 0, 0);

    return 0;
}


int clear(List* list)
{
    if(list->head == NULL)
    {
        printf("\nERROR: CLEAR CALLED, BUT LIST IS EMPTY\n");
        list->errors += 1;
        return 1;
    }
    struct Node* node = list->head;
    struct Node* prev;
    while(node != NULL)
    {
        prev = node;
        node = node->next;
        free(prev->msg);
        free(prev);
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    sem_init(&full, 0, N);
    sem_init(&empty, 0, 0);
    return 0;
}

int insert(List* list, Message* msg)
{
    if(list->size == N)
    {
        printf("\nERROR: INSERT CALLED, BUT LIST IS FULL\n");
        list->errors += 1;
        return 1;
    }
    struct Node* node = malloc(sizeof(struct Node));
    if(node == NULL)
    {
      list->errors += 1;
      return -1;
    }
    node->msg = msg;
    node->next = NULL;
    if(msg->priority == VIP)
    {
        node->next = list->head;
        list->head = node;
        if(list->tail == NULL)
        {
            list->tail = node;
        }
    }
    else
    {
        if(list->head == NULL)
        {
            list->head = node;
            list->tail = node;
        }
        else
        {
            list->tail->next = node;
            list->tail = node;
        }
    }
    list->size += 1;
    return 0;
}

int pop(List* list, Message** msg)
{
    if(list->head == NULL)
    {
        printf("\nERROR: POP CALLED, BUT LIST IS EMPTY %d\n", list->size);
        list->errors += 1;
        return 1;
    }

    *msg = list->head->msg;
    struct Node* head = list->head;
    list->head = list->head->next;
    if(list->head == NULL)
        list->tail = NULL;
    free(head);
    list->size -= 1;
    return 0;
}

#endif
