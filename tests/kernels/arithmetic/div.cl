__kernel void div(__global uint *output)
{
    volatile int a = INT_MIN;
    volatile int b = 2;
    volatile int c = 0;
    volatile int d = -1;
    volatile uint e = 0;

    volatile int4 vb = {2, 0, -1, 1};
    volatile uint2 vc = {0, 1};

    output[0] = a / b;
    output[1] = a / c;
    output[2] = a / d;
    output[3] = a / e;
    int4 t = a / vb;
    output[4] = t.x;
    output[5] = t.y;
    output[6] = t.z;
    output[7] = t.w;
    uint2 u = a / vc;
    output[8] = u.x;
    output[9] = u.y;
}
