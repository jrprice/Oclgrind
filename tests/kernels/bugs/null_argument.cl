ulong func_1(ulong * p_1)
{
  return 1;
}

kernel void null_argument(global ulong *output)
{
  *output = func_1((void*)0);
}
