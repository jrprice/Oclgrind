union U
{
  uint a;
  uint b;
};

uint func(union U value)
{
  uint ret = value.a;
  value.b = 777;
  return ret;
}

kernel void byval_function_argument(global uint *output)
{
  union U u = {42};
  output[0] = func(u);
  output[1] = u.b;
}
