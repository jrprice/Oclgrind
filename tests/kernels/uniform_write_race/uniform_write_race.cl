kernel void uniform_write_race(global int *data)
{
  *data = 0;
}
