kernel void reduce(uint n,
                   global uint *data,
                   global uint *result,
                   local uint *localData)
{
  uint gid = get_global_id(0);
  uint lid = get_local_id(0);
  uint gsz = get_global_size(0);
  uint lsz = get_local_size(0);
  uint grp = get_group_id(0);

  uint sum = 0;
  for (uint i = gid; i < n; i+=gsz)
  {
    sum += data[i];
  }

  localData[lid] = sum;
  for (uint offset = lsz/2; offset > 0; offset/=2)
  {
    barrier(CLK_LOCAL_MEM_FENCE);
    if (lid < offset)
    {
      localData[lid] += localData[lid + offset];
    }
  }

  if (lid == 0)
  {
    result[grp] = localData[lid];
  }
}
