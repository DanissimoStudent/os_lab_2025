#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char** argv) {
    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    bool with_files = false;

    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = { {"seed", required_argument, 0, 0},
                                          {"array_size", required_argument, 0, 0},
                                          {"pnum", required_argument, 0, 0},
                                          {"by_files", no_argument, 0, 'f'},
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
                pnum = atoi(optarg);

                if (pnum == 0) {
                    printf("pnum parameter must be > 0!");
                    return 1;
                }

                if (pnum > array_size) {
                    printf("pnum must be <= array_size!");
                    return 1;
                }

                break;

            case 3:
                with_files = true;
                break;

            default:
                printf("Index %d is out of options\n", option_index);
            }
            break;

        case 'f':
            with_files = true;
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

    if (seed == -1 || array_size == -1 || pnum == -1) {
        printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
            argv[0]);
        return 1;
    }

    int* array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);
    int active_child_processes = 0;

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    int part = array_size / pnum;
    int remainder = array_size % pnum;

    int start;
    int end;
    struct MinMax buffer;

    // ОБЯЗАТЕЛЬНО ДО ВЫЗОВ fork() формируем каналы для связи
    // Указатель на массив из 2-х элементов
    int (*pipes)[2] = NULL;
    if (!with_files) {

        pipes = malloc(sizeof(int[2]) * pnum);
        for (int i = 0; i < pnum; i++) {
            if (pipe(pipes[i]) < 0) {
                printf("Ошибка при создании канала pipe");
                return 1;
            }
        }

    }

    for (int i = 0; i < pnum; i++) {
        pid_t child_pid = fork();
        if (child_pid >= 0) {
            // successful fork
            buffer.min = INT_MAX;
            buffer.max = INT_MIN;

            active_child_processes += 1;

            // Если сейчас работает клон, а не родитель
            if (child_pid == 0) {

                // Для текущего клона определяем его часть массива, с которой он будет работать
                if (i < remainder) {
                    start = i * (part + 1);
                    end = start + (part + 1);
                }
                else {
                    start = i * part + remainder;
                    end = start + part;
                }

                for (int j = start; j < end; j++) {
                    if (array[j] < buffer.min) buffer.min = array[j];
                    if (array[j] > buffer.max) buffer.max = array[j];
                }

                // Начало блока обмена данными
                if (with_files) {

                    // Для каждого клона свой файл
                    char filename[64];
                    sprintf(filename, "process_%d.txt", i);

                    FILE* f = fopen(filename, "wb");
                    if (f == NULL) {
                        printf("Failed to open file in clone");
                        return 1;
                    }

                    // Записываем структуру buffer целиком в файл
                    fwrite(&buffer, sizeof(struct MinMax), 1, f);
                    fclose(f);

                }
                else {
                    close(pipes[i][0]); //Сразу закрываем дескриптор чтения, т.к. в клоне он не нужен (в клоне только пишем)

                    if (write(pipes[i][1], &buffer, sizeof(struct MinMax)) < 0) {
                        printf("Ошибка при записи данных в pipe");
                        return 1;
                    }

                    close(pipes[i][1]); // Закрываем дескриптор записи
                    free(pipes);
                }

                free(array); // Очищаем память В КЛОНЕ
                return 0;
            }

            else {
                if (!with_files) close(pipes[i][1]); //Если это родитель, то закрываем дексриптор записи
            }

        }
        else {
            printf("Fork failed!\n");
            return 1;
        }
    }

    while (active_child_processes > 0) {
        wait(NULL);

        active_child_processes -= 1;
    }

    // Начало блока считывания данных

    struct MinMax min_max;
    struct MinMax pipe_buffer;
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;

    for (int i = 0; i < pnum; i++) {
        int min = INT_MAX;
        int max = INT_MIN;

        if (with_files) {

            char filename[64];
            sprintf(filename, "process_%d.txt", i);

            FILE* f = fopen(filename, "rb");
            if (f == NULL) {
                printf("Error: файл %s не открыт\n", filename);
                return 1;
            }

            size_t quan = fread(&pipe_buffer, sizeof(struct MinMax), 1, f); // В quan должно быть 1, т.к. прочитали ровно 1 структуру

            fclose(f);
            //remove(filename); 

            if (quan != 1) {
                printf("Не удалось прочитать данные из файла %s\n", filename);
                return -;
            }

            if (min_max.min > pipe_buffer.min) min_max.min = pipe_buffer.min;
            if (min_max.max < pipe_buffer.max) min_max.max = pipe_buffer.max;

        }
        else {

            int quan = read(pipes[i][0], &pipe_buffer, sizeof(struct MinMax));
            close(pipes[i][0]); // Закрываем дескриптор чтения

            if (quan < (int)sizeof(struct MinMax)) {
                printf("Ошибка при чтении данных из pipe %d\n", i);
                return 1;
            }

            if (min_max.min > pipe_buffer.min) min_max.min = pipe_buffer.min;
            if (min_max.max < pipe_buffer.max) min_max.max = pipe_buffer.max;

        }
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    if (!with_files) free(pipes);
    free(array);

    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
    printf("Elapsed time: %fms\n", elapsed_time);
    fflush(NULL);
    return 0;
}