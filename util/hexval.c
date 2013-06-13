#include <stdio.h>

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    return 1;
  }

  int si;
  unsigned int ui;
  float f;
  double d;

  unsigned char *bytes;
  size_t size;

  switch (argv[1][0])
  {
  case 'i':
    sscanf(argv[2], "%i", &si);
    bytes = (unsigned char*)&si;
    size = 4;
    break;
  case 'u':
    sscanf(argv[2], "%u", &ui);
    bytes = (unsigned char*)&ui;
    size = 4;
    break;
  case 'f':
    sscanf(argv[2], "%f", &f);
    bytes = (unsigned char*)&f;
    size = 4;
    break;
  case 'd':
    sscanf(argv[2], "%lf", &d);
    bytes = (unsigned char*)&d;
    size = 8;
    break;
  }

  for (size_t b = 0; b < size; b++)
  {
    printf("%02X ", bytes[b]);
  }
  printf("\n");

  return 0;
}
