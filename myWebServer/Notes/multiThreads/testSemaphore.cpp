#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

pthread_mutex_t mutex;
sem_t psem;
sem_t csem;

struct Node {
    int num; 
    struct Node *next;
};

struct Node * head = NULL;

void * producer(void* arg){
    while(1) {
        sem_wait(&psem);

        pthread_mutex_lock(&mutex);
        
        struct Node * newNode = (struct Node *)malloc(sizeof(struct Node));
        newNode->next = head;
        head = newNode;
        newNode->num = rand() % 1000;
        printf("add Node, num: %d, tid: %ld\n", newNode->num, pthread_self());
        
        pthread_mutex_unlock(&mutex);

        sem_post(&csem);
        usleep(100);
    }

    return NULL;
}

void * consumer(void* arg) {
    while(1){
        sem_wait(&csem);
        
        pthread_mutex_lock(&mutex);
        
        struct Node * tmp = head;
        head = head->next;
        printf("del node, num: %d, tid: %ld\n", tmp->num, pthread_self());
        free(tmp);
        
        pthread_mutex_unlock(&mutex);

        sem_post(&psem);
        usleep(100);
    }

    return NULL;
}

int main() {
    pthread_mutex_init(&mutex, NULL);
    sem_init(&psem, 0, 8);
    sem_init(&csem, 0, 0);

    pthread_t ptid[5], ctid[5];

    for (int i = 0; i < 5; i ++) {
        pthread_create(&ptid[i], NULL, producer, NULL);
        pthread_create(&ctid[i], NULL, consumer, NULL);
    }

    for (int i = 0; i < 5; i++) {
        pthread_detach(ptid[i]);
        pthread_detach(ctid[i]);
    }

    while(1) {
        sleep(10);
    }

    pthread_mutex_destroy(&mutex);
    pthread_exit(NULL);

    return 0;
}