#include <stdio.h>

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    return 1;
  }

  int i;
  sscanf(argv[1], "%X", &i);

  float f = *((float*)&i);
  printf("%f\n", f);

  return 0;
}
