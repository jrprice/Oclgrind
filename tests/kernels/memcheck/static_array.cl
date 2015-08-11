struct S
{
  int a;
  char b[2];
};

kernel void static_array(global char *output)
{
  volatile struct S s = {-1, {42, 7}};
  int i = get_global_id(0);
  s.b[i] = i;
  output[i] = s.b[i];
}
