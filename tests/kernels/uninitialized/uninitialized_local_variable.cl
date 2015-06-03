kernel void uninitialized_local_variable(global int *output)
{
  local int x;
  *output = x;
}
