#include <time.h>

clock_t start, end;
double cpu_time_used;

#define TIME_FUNCTION(FUNCTION) \
  start = clock();              \
  FUNCTION;                     \
  end = clock();                \
  cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC; \
  printf("Time taken by %s: %f\n", #FUNCTION, cpu_time_used);

