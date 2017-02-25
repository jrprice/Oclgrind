__kernel void ptr(__global ushort *output)
{
    volatile ushort a;
    volatile char b;

    volatile ushort2 va;

	a = &b;
	va = (ushort2)(1, &b);

    output[0] = a;
    output[1] = va.x;
    output[2] = va.y;
}
