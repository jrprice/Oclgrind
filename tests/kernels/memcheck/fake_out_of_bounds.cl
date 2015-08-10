struct S0 {
    uchar f[1];
    ulong g[4];
};

__kernel void entry(__global ulong *result) {
    struct S0 s = {{1}, {2,3,4,5}};
    struct S0 t = s;

    volatile int i = 0;
    *result = t.g[i];
}
