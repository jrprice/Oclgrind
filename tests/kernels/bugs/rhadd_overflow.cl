kernel void rhadd_overflow(global ulong *output)
{
  output[0] = rhadd(0UL, 0xFFFFFFFFFFFFFFFFUL);
}
