#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>

#include "utils.h"

#define CHAT_SLEEP 2000
#define USER_SLEEP 4000
#define VIP_SLEEP 4000
#define ADMIN_SLEEP 10000

#define BE_VERBOSE
//#define CONFIRM_SENT



void* chat(void* arg)
{
    int bufferErrors = 0;
    Message* msg;
    while(!bufferErrors)
    {
        sem_wait(&empty);
        sem_wait(&mutex);
        bufferErrors = buffer->errors;
        if(!bufferErrors)
            pop(buffer, &msg);
        sem_post(&mutex);
        if(!bufferErrors)
        {
            sem_post(&full);
            #ifdef BE_VERBOSE
                printf("Published to chat: %s\n\n", msg->text);
            #endif
        }
        usleep(CHAT_SLEEP);
    }
    //#ifdef BE_VERBOSE
  //      printf("Chat finished\n");
    //#endif
}


void* user(void* arg)
{
    char id = *((char*)&arg);
    int bufferErrors = 0;
    Message* msg;
    while(!bufferErrors)
    {
        msg = calloc(1, sizeof(struct Message));
        if(msg == NULL)
        {
            printf("Calloc error\n");
            break;
        }
        msg->priority = STANDARD;
        msg->text[0] = 'U';
        msg->text[1] = id;
        msg->text[2] = ':';
        strcpy(msg->text + 3, random_text());
        #ifdef BE_VERBOSE
            printf("Sending: %s\n", msg->text);
        #endif
        sem_wait(&full);
        sem_wait(&mutex);
        #ifdef CONFIRM_SENT
            printf("Sent: %s\n", msg->text);
        #endif
        bufferErrors = buffer->errors;
        if(!bufferErrors)
            insert(buffer, msg);
        sem_post(&mutex);
        if(!bufferErrors)
            sem_post(&empty);
        usleep(USER_SLEEP);
    }
    //#ifdef BE_VERBOSE
  //      printf("User %c finished\n", id);
    //#endif
}


void* vip(void* arg)
{
    char id = *((char*)&arg);
    int bufferErrors = 0;
    Message* msg;
    while(!bufferErrors)
    {
        msg = calloc(1, sizeof(struct Message));
        if(msg == NULL)
        {
            printf("Calloc error\n");
            break;
        }
        msg->priority = VIP;
        msg->text[0] = 'V';
        msg->text[1] = id;
        msg->text[2] = ':';
        strcpy(msg->text + 3, random_text());
        #ifdef BE_VERBOSE
            printf("Sending: %s\n", msg->text);
        #endif
        sem_wait(&full);
        sem_wait(&mutex);
        #ifdef CONFIRM_SENT
            printf("Sent: %s\n", msg->text);
        #endif
        bufferErrors = buffer->errors;
        if(!bufferErrors)
            insert(buffer, msg);
        sem_post(&mutex);
        if(!bufferErrors)
            sem_post(&empty);
        usleep(VIP_SLEEP);
    }
    //#ifdef BE_VERBOSE
  //      printf("Vip %c finished\n", id);
    //#endif
}


void* admin(void* arg)
{
    char id = *((char*)&arg);
    int bufferErrors = 0;
    while(!bufferErrors)
    {
        #ifdef BE_VERBOSE
            printf("Admin %c: clearing buffer\n", id);
        #endif
        sem_wait(&empty);
        sem_wait(&mutex);
        bufferErrors = buffer->errors;
        if(!bufferErrors)
            clear(buffer);
        sem_post(&mutex);
        usleep(ADMIN_SLEEP);
    }
}


void readFromEmpty();
void manyUsers();
void manyVips();
void manyUsersAndVip();
void publishToFull();
void publishToFullVips();
void clearEmpty();
void testAdmin();


int main()
{
    buffer = NULL;
    srand(time(NULL));

    readFromEmpty();
    manyUsers();
    manyVips();
    manyUsersAndVip();
    publishToFull();
    publishToFullVips();
    clearEmpty();
    testAdmin();

    init(&buffer);
    if(buffer->head != NULL)
        clear(buffer);
    free(buffer);
    sem_destroy(&mutex);
    sem_destroy(&full);
    sem_destroy(&empty);
    return 0;
}


void readFromEmpty()
{
    init(&buffer);
    pthread_t t1,t2;
    pthread_create(&t1,NULL,chat,NULL);
    usleep(200000);

    int errors = 0;
    sem_wait(&mutex);
    errors = buffer->errors;
    buffer->errors = 1;
    sem_post(&empty);
    sem_post(&mutex);
    pthread_join(t1,NULL);
    printf("Read from empty test done. Errors: %d\n\n", errors);
}

void manyUsers()
{
    init(&buffer);
    pthread_t t1,t2,t3,t4;
    pthread_create(&t1,NULL,chat,NULL);
    int id = '0';
    pthread_create(&t2,NULL,user,(void*)id);
    id = '1';
    pthread_create(&t3,NULL,user,(void*)id);
    id = '2';
    pthread_create(&t4,NULL,user,(void*)id);
    int sleepTime = 6*CHAT_SLEEP;
    usleep(sleepTime);

    int errors = 0;
    sem_wait(&mutex);
    errors = buffer->errors;
    buffer->errors = 1;
    sem_post(&mutex);
    sem_post(&empty);
    sem_post(&full);
    sem_post(&full);
    sem_post(&full);
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    pthread_join(t3,NULL);
    pthread_join(t4,NULL);
    printf("\nMany users test done. Errors: %d\n\n", errors);
}


void manyVips()
{
    init(&buffer);
    pthread_t t1,t2,t3,t4;
    pthread_create(&t1,NULL,chat,NULL);
    int id = '0';
    pthread_create(&t2,NULL,vip,(void*)id);
    id = '1';
    pthread_create(&t3,NULL,vip,(void*)id);
    id = '2';
    pthread_create(&t4,NULL,vip,(void*)id);
    int sleepTime = 6*CHAT_SLEEP;
    usleep(sleepTime);

    int errors = 0;
    sem_wait(&mutex);
    errors = buffer->errors;
    buffer->errors = 1;
    sem_post(&mutex);
    sem_post(&empty);
    sem_post(&full);
    sem_post(&full);
    sem_post(&full);
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    pthread_join(t3,NULL);
    pthread_join(t4,NULL);
    printf("\nMany vips test done. Errors: %d\n\n", errors);
}


void manyUsersAndVip()
{
    init(&buffer);
    pthread_t t1,t2,t3,t4;
    pthread_create(&t1,NULL,chat,NULL);
    int id = '0';
    pthread_create(&t2,NULL,user,(void*)id);
    id = '1';
    pthread_create(&t3,NULL,user,(void*)id);
    id = '0';
    pthread_create(&t4,NULL,vip,(void*)id);
    int sleepTime = 6*CHAT_SLEEP;
    usleep(sleepTime);

    int errors = 0;
    sem_wait(&mutex);
    errors = buffer->errors;
    buffer->errors = 1;
    sem_post(&mutex);
    sem_post(&empty);
    sem_post(&full);
    sem_post(&full);
    sem_post(&full);
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    pthread_join(t3,NULL);
    pthread_join(t4,NULL);
    printf("\nMany users and vip test done. Errors: %d\n\n", errors);
}


void publishToFull()
{
    init(&buffer);
    pthread_t t1,t2,t3,t4;
    int id = '0';
    pthread_create(&t2,NULL,user,(void*)id);
    id = '1';
    pthread_create(&t3,NULL,user,(void*)id);
    id = '2';
    pthread_create(&t4,NULL,user,(void*)id);

    usleep(200000);

    int errors = 0;
    sem_wait(&mutex);
    printf("Publishing to full buffer test done. Finishing threads.\n\n");
    errors = buffer->errors;
    buffer->errors = 1;
    sem_post(&mutex);
    sem_post(&full);
    sem_post(&full);
    sem_post(&full);
    pthread_join(t2,NULL);
    pthread_join(t3,NULL);
    pthread_join(t4,NULL);
    printf("\nPublishing to full buffer test done. Errors: %d\n\n", errors);
}

void publishToFullVips()
{
    init(&buffer);
    pthread_t t1,t2,t3,t4;
    int id = '0';
    pthread_create(&t2,NULL,vip,(void*)id);
    id = '1';
    pthread_create(&t3,NULL,vip,(void*)id);
    id = '2';
    pthread_create(&t4,NULL,vip,(void*)id);

    usleep(200000);

    int errors = 0;
    sem_wait(&mutex);
    printf("Publishing vip messages to full buffer test done. Finishing threads.\n\n");
    errors = buffer->errors;
    buffer->errors = 1;
    sem_post(&mutex);
    sem_post(&full);
    sem_post(&full);
    sem_post(&full);
    pthread_join(t2,NULL);
    pthread_join(t3,NULL);
    pthread_join(t4,NULL);
    printf("\nPublishing vip messages to full buffer test done. Errors: %d\n\n", errors);
}

void clearEmpty()
{
    init(&buffer);
    pthread_t t1;
    int id = '0';
    pthread_create(&t1,NULL,admin,(void*)id);
    usleep(200000);

    int errors = 0;
    sem_wait(&mutex);
    errors = buffer->errors;
    buffer->errors = 1;
    sem_post(&empty);
    sem_post(&mutex);
    pthread_join(t1,NULL);
    printf("\nClear empty test done. Errors: %d\n\n", errors);
}

void testAdmin()
{
    init(&buffer);
    pthread_t t1,t2,t3,t4;
    pthread_create(&t1,NULL,chat,NULL);
    int id = '0';
    pthread_create(&t2,NULL,user,(void*)id);
    id = '0';
    pthread_create(&t3,NULL,vip,(void*)id);
    int sleepTime = CHAT_SLEEP / 2;
    usleep(sleepTime);
    id = '0';
    pthread_create(&t4,NULL,admin,(void*)id);
    sleepTime = 6*CHAT_SLEEP;
    usleep(sleepTime);

    int errors = 0;
    sem_wait(&mutex);
    errors = buffer->errors;
    buffer->errors = 1;
    sem_post(&mutex);
    sem_post(&empty);
    sem_post(&empty);
    sem_post(&full);
    sem_post(&full);
    sem_post(&full);
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    pthread_join(t3,NULL);
    pthread_join(t4,NULL);
    printf("\nAdmin test done. Errors: %d\n\n", errors);
}
