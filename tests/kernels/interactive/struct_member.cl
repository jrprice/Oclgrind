struct S
{
  int a;
  float b;
};

kernel void struct_member(global int *i, global float *f, global struct S *out)
{
  struct S s;
  local struct S t;
  s.a = *i;
  s.b = *f;
  t = s;
  t.a += 1;
  t.b += 0.1f;
  *out = t;
  out->a += 2;
  out->b += 0.2f;
}
