kernel void reduce(unsigned int n,
                   global unsigned int *data,
                   global unsigned int *result,
                   local unsigned int *localData)
{
  unsigned int lid = get_local_id(0);
  unsigned int lsz = get_local_size(0);
  unsigned int sum = 0;
  for (unsigned int i = lid; i < n; i+=lsz)
  {
    sum += data[i];
  }

  localData[lid] = sum;
  for (unsigned int offset = lsz/2; offset > 0; offset/=2)
  {
    barrier(CLK_LOCAL_MEM_FENCE);
    if (lid < offset)
    {
      localData[lid] += localData[lid + offset];
    }
  }

  if (lid == 0)
  {
    *result = localData[lid];
  }
}
