#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

#include "dynamic_sum.h"

int Sum(const struct SumArgs *args) {
  long long sum = 0;
  
  for (int i = args->begin; i < args->end; i++) {
    sum += args->array[i];
  }

  return sum;
}

void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  return (void *)(size_t)Sum(sum_args);
}

int SumArray(int *array, int array_size, int threads_num) {
  pthread_t threads[threads_num];

  struct SumArgs args[threads_num];

  int parts = array_size/threads_num;
  int ostatok = array_size % threads_num;
  int old_begin = 0;

  for (uint32_t i = 0; i < threads_num; i++) {

    if (ostatok > 0) {
      args[i].begin = old_begin;
      args[i].end = i*parts + parts + 1;
    }
    else {
      args[i].begin = old_begin;
      args[i].end = i*parts + parts;
    }
    old_begin = args[i].end;
    ostatok -= 1;
    args[i].array = array;

    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
      printf("Error: pthread_create failed!\n");
      return 0;
    }
  }

  long long total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    long long sum = 0;
    pthread_join(threads[i], (void **)&sum);
    total_sum += sum;
  }

  return total_sum;
}