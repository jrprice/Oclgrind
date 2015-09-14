__kernel void add(__global uint *output)
{
    volatile int a = 3;
    volatile int b = 5;

    volatile int c = 3;
    volatile int d = INT_MAX;

    volatile int2 va = {3, 3};
    volatile int2 vb = {5, INT_MAX};

    output[0] = a + b;
    output[1] = c + d;
    int2 t = va + vb;
    output[2] = t.x;
    output[3] = t.y;
}
