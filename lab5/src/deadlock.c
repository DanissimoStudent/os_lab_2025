#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t mutexA = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexB = PTHREAD_MUTEX_INITIALIZER;

void* thread_one_func(void* arg) {
    pthread_mutex_lock(&mutexA);
    sleep(1);
    pthread_mutex_lock(&mutexB);
    
    printf("First critical section\n");
    
    pthread_mutex_unlock(&mutexB);
    pthread_mutex_unlock(&mutexA);
}

void* thread_two_func(void* arg) {
    pthread_mutex_lock(&mutexB);
    sleep(1);
    pthread_mutex_lock(&mutexA);
    
        printf("Second critical section\n");
    
    pthread_mutex_unlock(&mutexA);
    pthread_mutex_unlock(&mutexB);
}

int main() {
  pthread_t thread1, thread2;

  if (pthread_create(&thread1, NULL, (void *)thread_one_func, NULL) != 0) {
    perror("pthread_create");
    exit(1);
  }

  if (pthread_create(&thread2, NULL, (void *)thread_two_func, NULL) != 0) {
    perror("pthread_create");
    exit(1);
  }

  if (pthread_join(thread1, NULL) != 0) {
    perror("pthread_join");
    exit(1);
  }

  if (pthread_join(thread2, NULL) != 0) {
    perror("pthread_join");
    exit(1);
  }

  printf("Finish!\n");

  return 0;
}