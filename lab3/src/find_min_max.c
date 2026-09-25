#include "find_min_max.h"
#include <stdio.h>

#include <limits.h>

struct MinMax GetMinMax(int *array, unsigned int begin, unsigned int end) {
  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (unsigned int i = begin; i < end; i++) {
    int elem = array[i];

    if (elem < min_max.min) min_max.min = elem;
    if (elem > min_max.max) min_max.max = elem;
  } 

  return min_max;
}
