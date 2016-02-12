__kernel void partially_uninitialized_fract(__global float4 *output)
{
    float4 f;
    f.xzw = 4.2;
    *(output + 1) = fract(f, output);
}
