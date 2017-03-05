#pragma OPENCL EXTENSION cl_khr_int64_base_atomics : enable

kernel void _atom_add(global ulong *data)
{
  atom_add(data, (ulong)UINT_MAX);
}
