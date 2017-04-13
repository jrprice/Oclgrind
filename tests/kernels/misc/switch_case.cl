kernel void switch_case(global int *input, global int *output)
{
  int i = get_global_id(0);
  int in = input[i];
  int out;
  switch (in)
  {
  case 0:
    out = -7;
    break;
  case 1:
    out = i;
    break;
  case 2:
  case 3:
  case 4:
    out = in + i;
    break;
  default:
    out = 42;
    break;
  }

  output[i] = out;
}
