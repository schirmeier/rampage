#include <stdio.h>
#include <stdlib.h>

#define SIZE (1024*1024)

int main(int argc, char *argv[])
{
  int k;
  int mbytes;
  int sleeptime;

  if (argc <= 2) {
    fprintf(stderr, "usage: %s mbytes sleeptime\n", argv[0]);
	return 1;
  }
  mbytes = atoi(argv[1]);
  sleeptime = atoi(argv[2]);

  for (k=0;k<mbytes;k++) {
    int *p = malloc(SIZE), i, j;
    if (!p) {
      printf("malloc failed\n");
      break;
    }

    for (j = 0; j < SIZE / sizeof(*p); ++j) {
      p[j] = 42;
    }
  }

  sleep(sleeptime);

  return 0;
}
