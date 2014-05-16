kernel void atomic_cmpxchg_false_race(global int *data, local int *scratch)
{
  int l = get_local_id(0);
  if (l == 0)
  {
    scratch[0] = 0;
  }
  barrier(CLK_LOCAL_MEM_FENCE);

  bool done = false;
  int before, old;
  int result;
  for (int i = 0; i < get_local_size(0); i++)
  {
    barrier(CLK_LOCAL_MEM_FENCE);
    before = scratch[0];
    barrier(CLK_LOCAL_MEM_FENCE);

    if (!done)
    {
      old = atomic_cmpxchg(scratch, before, before+1);
    }

    if (old == before)
    {
      done = true;
      result = scratch[0];
    }
  }

  barrier(CLK_LOCAL_MEM_FENCE);
  if (l == 0)
  {
    *data = *scratch;
  }
  data[l+1] = result;
}
