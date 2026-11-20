#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "pthread.h"

uint64_t total_result = 1;
uint64_t global_mod = 1;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

struct Server {
  char ip[255];
  int port;
};

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
  uint64_t result = 0;
  a = a % mod;
  while (b > 0) {
    if (b % 2 == 1)
      result = (result + a) % mod;
    a = (a * 2) % mod;
    b /= 2;
  }

  return result % mod;
}

bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0)
    return false;

  *val = i;
  return true;
}

int readServersData(const char* filename, struct Server **to) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
      printf("Cannot open servers file");
      return -1;
  }

  int counter = -1;
  int idx = 0;
  char line[255];

  while (fgets(line, sizeof(line), fp) != NULL) {
    line[strcspn(line, "\r\n")] = '\0';

    if (counter == -1) {
      counter = atoi(line);
      if (counter <= 0) {
          fclose(fp);
          printf("servers counter must be > 0!");
          return -1;
      }

      *to = malloc(sizeof(struct Server) * counter);
    }
    else if (idx < counter) {
      char *colon = strchr(line, ':');
      if (colon != NULL) {
          *colon = '\0';
          strncpy((*to)[idx].ip, line, sizeof((*to)[idx].ip) - 1);
          
          (*to)[idx].ip[sizeof((*to)[idx].ip) - 1] = '\0';
          (*to)[idx].port = atoi(colon + 1);
          
          idx++;
      }
    }
  }

  fclose(fp);
  return counter;
}

typedef struct{
    uint64_t start;
    uint64_t end;
    int socket;

} thread_data_t;

void* calculate_factorial(void *arg) {
    thread_data_t *data = (thread_data_t*)arg;

    int sck = data->socket;
    uint64_t begin = data->start;
    uint64_t end = data->end;

    char task[sizeof(uint64_t) * 3];
    memcpy(task, &begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &global_mod, sizeof(uint64_t));

    if (send(sck, task, sizeof(task), 0) < 0) {
      fprintf(stderr, "Send failed\n");
      exit(1);
    }

    char response[sizeof(uint64_t)];
    if (recv(sck, response, sizeof(response), 0) < 0) {
      fprintf(stderr, "Recieve failed\n");
      exit(1);
    }

    uint64_t answer = 0;
    memcpy(&answer, response, sizeof(uint64_t));

    close(sck);

    pthread_mutex_lock(&result_mutex);
    total_result = MultModulo(total_result, answer, global_mod);
    pthread_mutex_unlock(&result_mutex);

    return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers[255] = {'\0'}; //255, т.к. в большинстве систем макисальная длина пути к файлу не может превышать 255 символов

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        
        if (k <= 0) {
          printf("k parameter must be > 0!\n");
          return 1;
        }

        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        
        if (mod <= 0) {
          printf("mod parameter must be > 0!\n");
          return 1;
        }

        break;
      case 2:

        if (strlen(optarg) == 0) {
          printf("servers parameter must not be empty!\n");
          return 1;
        }

        memcpy(servers, optarg, strlen(optarg));
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 40 --mod 5 --servers ./servers_data.txt\n",
            argv[0]);
    return 1;
  }
  global_mod = mod;
  struct Server *to = NULL;
  unsigned int servers_num = readServersData(servers, &to);

  pthread_t *threads = malloc(servers_num * sizeof(pthread_t));
  thread_data_t *threads_data = malloc(servers_num * sizeof(thread_data_t));

  uint64_t base_range = k / servers_num;
  uint64_t ostatok = k % servers_num;
  uint64_t current_start = 1;

  // Т.к. gethostbyname() непотокобезопасный, то выносим заполнение этой структуры в обычный последовательный цикл
  for (uint64_t i = 0; i < servers_num; i++) {
      struct hostent *hostname = gethostbyname(to[i].ip);
      if (hostname == NULL) {
      fprintf(stderr, "gethostbyname failed with %s\n", to[i].ip);
      exit(1);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(to[i].port);
    server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
      fprintf(stderr, "Socket creation failed!\n");
      exit(1);
    }

    if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
      fprintf(stderr, "Connection failed\n");
      exit(1);
    }

    threads_data[i].start = current_start;
    threads_data[i].end = current_start + base_range - 1;
    threads_data[i].socket = sck;
    
    // Равномерно распределяем остаток, если k % servers_num != 0
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
  for (uint32_t i = 0; i < servers_num; i++) {
      if (pthread_join(threads[i], NULL) != 0) {
          printf("ERROR: thread not join");
          return 1;
      }
  }
  
  printf("Факториал числа %lu по модулю %lu равен %lu\n", k, mod, total_result);

  free(to);
  free(threads);
  free(threads_data);

  return 0;
}