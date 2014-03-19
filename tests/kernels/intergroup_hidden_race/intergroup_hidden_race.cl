kernel void intergroup_hidden_race(global int *data, global int *output)
{
  int group = get_group_id(0);
  output[group] = data[0];
  if (group == 1)
  {
    data[0] = group;
  }
}
