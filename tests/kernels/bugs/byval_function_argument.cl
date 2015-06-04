union U
{
  uint a;
  uint b;
};

void func(union U value)
{
  value.b = 777;
}

kernel void byval_function_argument(global uint *output)
{
  union U u = {42};
  func(u);
  *output = u.b;
}
