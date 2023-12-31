#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t mutex;
pthread_cond_t cond;

struct Node {
    int num; 
    struct Node *next;
};

struct Node * head = NULL;

void * producer(void* arg){
    while(1) {
        pthread_mutex_lock(&mutex);
        struct Node * newNode = (struct Node *)malloc(sizeof(struct Node));
        newNode->next = head;
        head = newNode;
        newNode->num = rand() % 1000;
        printf("add Node, num: %d, tid: %ld\n", newNode->num, pthread_self());
        
        // 生产一个，就通知消费者
        pthread_cond_signal(&cond);

        pthread_mutex_unlock(&mutex);
        usleep(100);
    }

    return NULL;
}

void * consumer(void* arg) {
    while(1){
        pthread_mutex_lock(&mutex);
        
        while(head == NULL) {
            pthread_cond_wait(&cond, &mutex);
        }
        struct Node * tmp = head;
        head = head->next;
        printf("del node, num: %d, tid: %ld\n", tmp->num, pthread_self());
        free(tmp);



        // struct Node * tmp = head;
        // if (head != NULL) {
        //     head = head->next;
        //     printf("del node, num: %d, tid: %ld\n", tmp->num, pthread_self());
        //     free(tmp);
        // }else {
        //     // 等待
        //     pthread_cond_wait(&cond, &mutex);
        // }
        
        pthread_mutex_unlock(&mutex);
        usleep(100);
    }

    return NULL;
}

int main() {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

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

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
    pthread_exit(NULL);

    return 0;
}