global int g_arr[] = {7, 42};
constant int c_arr[] = {-3, 56};
global int *p_g_int = &g_arr[1];
constant int *p_c_int = &c_arr[1];

kernel void global_variables(global int *output)
{
  output[0] = g_arr[0];
  output[1] = g_arr[1];
  output[2] = c_arr[0];
  output[3] = c_arr[1];
  output[4] = *p_g_int;
  output[5] = *p_c_int;
}
