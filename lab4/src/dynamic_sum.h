#ifndef DYNAMIC_SUM_H
#define DYNAMIC_SUM_H

struct SumArgs {
  int *array;
  int begin;
  int end;
};

int Sum(const struct SumArgs *args);

void *ThreadSum(void *args);

int SumArray(int *array, int array_size, int threads_num);

#endif