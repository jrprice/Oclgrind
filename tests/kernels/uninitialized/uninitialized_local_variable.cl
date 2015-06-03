kernel void uninitialized_local_variable(global int *output)
{
  local int x;
  if (*output > 0)
    x = *output;
  *output = x;
}
