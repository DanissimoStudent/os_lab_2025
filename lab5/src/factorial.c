#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>

uint64_t total_result = 1;
uint32_t global_mod = 1;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct{
    uint32_t start;
    uint32_t end;
} thread_data_t;

void* calculate_factorial(void *arg) {
    thread_data_t *data = (thread_data_t*)arg;
    uint64_t local_result = 1;

    // Вычисляем частичное произведение на своем отрезке [start, end]
    for (uint32_t i = data->start; i <= data->end; i++) {
        local_result = (local_result * i) % global_mod;
    }

    pthread_mutex_lock(&result_mutex);
    total_result = (total_result * local_result) % global_mod;
    pthread_mutex_unlock(&result_mutex);
}

int main(int argc, char **argv) {
  uint32_t k = 0;
  uint32_t pnum = 0;
  uint32_t mod = 0;

  while (true) {
      int current_optind = optind ? optind : 1;

      static struct option options[] = { {"k", required_argument, 0, 0},
                                         {"pnum", required_argument, 0, 0},
                                         {"mod", required_argument, 0, 0},
                                         {0, 0, 0, 0} };

      int option_index = 0;
      int c = getopt_long(argc, argv, "f", options, &option_index);

      if (c == -1) break;

      switch (c) {
      case 0:
          switch (option_index) {
          case 0:
            k = atoi(optarg);
            if (k <= 0) {
                printf("k parameter must be number and > 0!\n");
                return 1;
            }
            break;
          case 1:
              pnum = atoi(optarg);
              if (pnum <= 0) {
                  printf("pnum parameter must be > 0!\n");
                  return 1;
              }
              break;
          case 2:
              mod = atoi(optarg);
              if (mod <= 0) {
                  printf("mod parameter must be > 0!\n");
                  return 1;
              }
              break;
          default:
              printf("Index %d is out of options\n", option_index);
          }
          break;

      case '?':
          return 1;

      default:
          printf("getopt returned character code 0%o?\n", c);
      }
  }

  if (optind < argc) {
      printf("Has at least one no option argument\n");
      return 1;
  }

  if (k == 0 || mod == 0 || pnum == 0) {
      printf("Incorrect data!\n");
      return 1;
  }

  global_mod = mod;

  if (pnum > k) {
      pnum = k;
  }

  pthread_t *threads = malloc(pnum * sizeof(pthread_t));
  thread_data_t *threads_data = malloc(pnum * sizeof(thread_data_t));

  uint32_t base_range = k / pnum;
  uint32_t ostatok = k % pnum;
  uint32_t current_start = 1;

  for (uint32_t i = 0; i < pnum; i++) {
      threads_data[i].start = current_start;
      threads_data[i].end = current_start + base_range - 1;
      
      // Равномерно распределяем остаток, если k % pnum != 0
      if (ostatok > 0) {
          threads_data[i].end++;
          ostatok--;
      }

      current_start = threads_data[i].end + 1;

      if (pthread_create(&threads[i], NULL, calculate_factorial, &threads_data[i]) != 0) {
          printf("ERROR: thread not create!\n");
          return 1;
      }
  }
  for (uint32_t i = 0; i < pnum; i++) {
      if (pthread_join(threads[i], NULL) != 0) {
          printf("ERROR: thread not join");
          return 1;
      }
  }

  printf("%u! mod %u = %lu\n", k, mod, total_result);

  free(threads);
  free(threads_data);

  return 0;
}
