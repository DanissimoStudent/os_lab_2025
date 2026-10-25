#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>

#include <getopt.h>

#include <pthread.h>
#include "utils.h"

struct SumArgs {
  int *array;
  int begin;
  int end;
};

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
      _exit(1);
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

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;

  while (true) {
      int current_optind = optind ? optind : 1;

      static struct option options[] = { {"seed", required_argument, 0, 0},
                                          {"array_size", required_argument, 0, 0},
                                          {"threads_num", required_argument, 0, 0},
                                          {0, 0, 0, 0} };

      int option_index = 0;
      int c = getopt_long(argc, argv, "f", options, &option_index);

      if (c == -1) break;

      switch (c) {
      case 0:
          switch (option_index) {
          case 0:
              seed = atoi(optarg);

              if (seed == 0) {
                  printf("seed parameter must be number and > 0!");
                  return 1;
              }

              break;
          case 1:
              array_size = atoi(optarg);

              if (array_size == 0) {
                  printf("array_size parameter must be > 0!");
                  return 1;
              }

              break;

          case 2:
              threads_num = atoi(optarg);

              if (threads_num == 0) {
                  printf("threads_num parameter must be > 0!");
                  return 1;
              }

              break;

          default:
              printf("Index %d is out of options\n", option_index);
          }
          break;

      case '?':
          break;

      default:
          printf("getopt returned character code 0%o?\n", c);
      }
  }

  if (optind < argc) {
      printf("Has at least one no option argument\n");
      return 1;
  }

  if (seed == 0 || array_size == 0 || threads_num == 0) {
      printf("Usage: %s --seed \"num\" --array_size \"num\" --threads_num \"num\" \n",
          argv[0]);
      return 1;
  }

  int *array = malloc(sizeof(int) * array_size);

  GenerateArray(array, array_size, seed);
  
  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  int total_sum = SumArray(array, array_size, threads_num);

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  
  printf("Total: %lld\n", total_sum);
  printf("Elapsed time: %fms\n", elapsed_time);
  
  return 0;
}