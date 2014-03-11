kernel void array(global long16 *output)
{
  long16 data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

  int i = get_global_id(0);

  long16 *foo = data;

  output[i] = foo[i];
}
