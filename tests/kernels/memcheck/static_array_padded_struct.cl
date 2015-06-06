struct S
{
  int a;
  char b[2];
};

kernel void static_array_padded_struct(global char *output)
{
  struct S s = {-1, {42, 7}};
  int i = get_global_id(0);
  output[i] = s.b[i];
}
