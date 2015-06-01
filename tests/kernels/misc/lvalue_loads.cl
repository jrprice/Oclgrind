typedef struct
{
  char  a;
  int   b;
  int   c;
  char  d;
} S;

void va(global float4 *input, global float4 *output)
{
  int i = get_global_id(0);
  output[i].z = 42.f;
}

void vb(global float4 *input, global float4 *output)
{
  int i = get_global_id(0);
  output[i].z = 7.f;
  output[i].y = 42.f;
}

void vc(global float4 *input, global float4 *output)
{
  int i = get_global_id(0);
  output[i].zy = (float2)(7.f,42.f);
}

void vd(global float4 *input, global float4 *output)
{
  int i = get_global_id(0);
  output[i].y = output[i].z;
}

void ve(global float4 *input, global float4 *output)
{
  int i = get_global_id(0);
  output[i].wzyx = output[i];
}

void vf(global float4 *input, global float4 *output)
{
  int i = get_global_id(0);
  output[i].zy = output[i].yz;
}

void vg(global float4 *input, global float4 *output)
{
  int i = get_global_id(0);
  output[i].wzyx = input[i];
}

void vh(global float4 *input, global float4 *output)
{
  int i = get_global_id(0);
  output[i].zy = input[i].yz;
}

void vi(global float4 *input, global float4 *output)
{
  int i = get_global_id(0);

  float4 x = output[i];
  x.z = 42.f;
  output[i] = x;
  output[i+1] = x;
}

void sa(global S *input, global S *output)
{
  int i = get_global_id(0);
  output[i].c = 42;
}

void sb(global S *input, global S *output)
{
  int i = get_global_id(0);
  output[i].c = output[i].b;
}

void sc(global S *input, global S *output)
{
  int i = get_global_id(0);
  output[i].c = input[i].b;
}

kernel void lvalue_loads(
  global float4 *vIn,
  global float4 *vA,
  global float4 *vB,
  global float4 *vC,
  global float4 *vD,
  global float4 *vE,
  global float4 *vF,
  global float4 *vG,
  global float4 *vH,
  global float4 *vI,

  global S      *sIn,
  global S      *sA,
  global S      *sB,
  global S      *sC,

  global float  *nop
  )
{
  va(vIn, vA);
  vb(vIn, vB);
  vc(vIn, vC);
  vd(vIn, vD);
  ve(vIn, vE);
  vf(vIn, vF);
  vg(vIn, vG);
  vh(vIn, vH);
  vi(vIn, vI);

  sa(sIn, sA);
  sb(sIn, sB);
  sc(sIn, sC);
}
