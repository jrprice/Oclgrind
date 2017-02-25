kernel void wait_event_invalid(global int *data)
{
  event_t event = 0;
  wait_group_events(1, &event);
}
