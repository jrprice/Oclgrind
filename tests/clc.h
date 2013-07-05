typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long size_t;

// TODO: typeof?
#define event_t size_t
typedef long ptrdiff_t;
typedef unsigned long uintptr_t;
typedef unsigned long intptr_t;

typedef __attribute__((ext_vector_type(2))) float float2;
typedef __attribute__((ext_vector_type(3))) float float3;
typedef __attribute__((ext_vector_type(4))) float float4;
typedef __attribute__((ext_vector_type(8))) float float8;
typedef __attribute__((ext_vector_type(16))) float float16;

typedef __attribute__((ext_vector_type(2))) double double2;
typedef __attribute__((ext_vector_type(3))) double double3;
typedef __attribute__((ext_vector_type(4))) double double4;
typedef __attribute__((ext_vector_type(8))) double double8;
typedef __attribute__((ext_vector_type(16))) double double16;

typedef __attribute__((ext_vector_type(2))) char char2;
typedef __attribute__((ext_vector_type(3))) char char3;
typedef __attribute__((ext_vector_type(4))) char char4;
typedef __attribute__((ext_vector_type(8))) char char8;
typedef __attribute__((ext_vector_type(16))) char char16;

typedef __attribute__((ext_vector_type(2))) uchar uchar2;
typedef __attribute__((ext_vector_type(3))) uchar uchar3;
typedef __attribute__((ext_vector_type(4))) uchar uchar4;
typedef __attribute__((ext_vector_type(8))) uchar uchar8;
typedef __attribute__((ext_vector_type(16))) uchar uchar16;

typedef __attribute__((ext_vector_type(2))) short short2;
typedef __attribute__((ext_vector_type(3))) short short3;
typedef __attribute__((ext_vector_type(4))) short short4;
typedef __attribute__((ext_vector_type(8))) short short8;
typedef __attribute__((ext_vector_type(16))) short short16;

typedef __attribute__((ext_vector_type(2))) ushort ushort2;
typedef __attribute__((ext_vector_type(3))) ushort ushort3;
typedef __attribute__((ext_vector_type(4))) ushort ushort4;
typedef __attribute__((ext_vector_type(8))) ushort ushort8;
typedef __attribute__((ext_vector_type(16))) ushort ushort16;

typedef __attribute__((ext_vector_type(2))) int int2;
typedef __attribute__((ext_vector_type(3))) int int3;
typedef __attribute__((ext_vector_type(4))) int int4;
typedef __attribute__((ext_vector_type(8))) int int8;
typedef __attribute__((ext_vector_type(16))) int int16;

typedef __attribute__((ext_vector_type(2))) uint uint2;
typedef __attribute__((ext_vector_type(3))) uint uint3;
typedef __attribute__((ext_vector_type(4))) uint uint4;
typedef __attribute__((ext_vector_type(8))) uint uint8;
typedef __attribute__((ext_vector_type(16))) uint uint16;

typedef __attribute__((ext_vector_type(2))) long long2;
typedef __attribute__((ext_vector_type(3))) long long3;
typedef __attribute__((ext_vector_type(4))) long long4;
typedef __attribute__((ext_vector_type(8))) long long8;
typedef __attribute__((ext_vector_type(16))) long long16;

typedef __attribute__((ext_vector_type(2))) ulong ulong2;
typedef __attribute__((ext_vector_type(3))) ulong ulong3;
typedef __attribute__((ext_vector_type(4))) ulong ulong4;
typedef __attribute__((ext_vector_type(8))) ulong ulong8;
typedef __attribute__((ext_vector_type(16))) ulong ulong16;

#define CLK_LOCAL_MEM_FENCE 1<<0
#define CLK_GLOBAL_MEM_FENCE 1<<1
#define FLT_MAX 1E+37

#define __OVERLOAD__ __attribute__((__overloadable__))

uint get_work_dim();
size_t get_global_id(uint dim);
size_t get_global_size(uint dim);
size_t get_group_id(uint dim);
size_t get_num_groups(uint dim);
size_t get_local_id(uint dim);
size_t get_local_size(uint dim);

void barrier(uint);
float cos(float x);
float dot(float a, float b);
float fabs(float x);
int hadd(int x, int y);
int min(int a, int b);
float native_sqrt(float x);
float sin(float x);

char2 __OVERLOAD__ vload2(size_t, char*);
char2 __OVERLOAD__ vload2(size_t, __local char*);
char2 __OVERLOAD__ vload2(size_t, __global char*);
char2 __OVERLOAD__ vload2(size_t, __constant char*);
char3 __OVERLOAD__ vload3(size_t, char*);
char3 __OVERLOAD__ vload3(size_t, __local char*);
char3 __OVERLOAD__ vload3(size_t, __global char*);
char3 __OVERLOAD__ vload3(size_t, __constant char*);
char4 __OVERLOAD__ vload4(size_t, char*);
char4 __OVERLOAD__ vload4(size_t, __local char*);
char4 __OVERLOAD__ vload4(size_t, __global char*);
char4 __OVERLOAD__ vload4(size_t, __constant char*);
char8 __OVERLOAD__ vload8(size_t, char*);
char8 __OVERLOAD__ vload8(size_t, __local char*);
char8 __OVERLOAD__ vload8(size_t, __global char*);
char8 __OVERLOAD__ vload8(size_t, __constant char*);
char16 __OVERLOAD__ vload16(size_t, char*);
char16 __OVERLOAD__ vload16(size_t, __local char*);
char16 __OVERLOAD__ vload16(size_t, __global char*);
char16 __OVERLOAD__ vload16(size_t, __constant char*);
uchar2 __OVERLOAD__ vload2(size_t, uchar*);
uchar2 __OVERLOAD__ vload2(size_t, __local uchar*);
uchar2 __OVERLOAD__ vload2(size_t, __global uchar*);
uchar2 __OVERLOAD__ vload2(size_t, __constant uchar*);
uchar3 __OVERLOAD__ vload3(size_t, uchar*);
uchar3 __OVERLOAD__ vload3(size_t, __local uchar*);
uchar3 __OVERLOAD__ vload3(size_t, __global uchar*);
uchar3 __OVERLOAD__ vload3(size_t, __constant uchar*);
uchar4 __OVERLOAD__ vload4(size_t, uchar*);
uchar4 __OVERLOAD__ vload4(size_t, __local uchar*);
uchar4 __OVERLOAD__ vload4(size_t, __global uchar*);
uchar4 __OVERLOAD__ vload4(size_t, __constant uchar*);
uchar8 __OVERLOAD__ vload8(size_t, uchar*);
uchar8 __OVERLOAD__ vload8(size_t, __local uchar*);
uchar8 __OVERLOAD__ vload8(size_t, __global uchar*);
uchar8 __OVERLOAD__ vload8(size_t, __constant uchar*);
uchar16 __OVERLOAD__ vload16(size_t, uchar*);
uchar16 __OVERLOAD__ vload16(size_t, __local uchar*);
uchar16 __OVERLOAD__ vload16(size_t, __global uchar*);
uchar16 __OVERLOAD__ vload16(size_t, __constant uchar*);
short2 __OVERLOAD__ vload2(size_t, short*);
short2 __OVERLOAD__ vload2(size_t, __local short*);
short2 __OVERLOAD__ vload2(size_t, __global short*);
short2 __OVERLOAD__ vload2(size_t, __constant short*);
short3 __OVERLOAD__ vload3(size_t, short*);
short3 __OVERLOAD__ vload3(size_t, __local short*);
short3 __OVERLOAD__ vload3(size_t, __global short*);
short3 __OVERLOAD__ vload3(size_t, __constant short*);
short4 __OVERLOAD__ vload4(size_t, short*);
short4 __OVERLOAD__ vload4(size_t, __local short*);
short4 __OVERLOAD__ vload4(size_t, __global short*);
short4 __OVERLOAD__ vload4(size_t, __constant short*);
short8 __OVERLOAD__ vload8(size_t, short*);
short8 __OVERLOAD__ vload8(size_t, __local short*);
short8 __OVERLOAD__ vload8(size_t, __global short*);
short8 __OVERLOAD__ vload8(size_t, __constant short*);
short16 __OVERLOAD__ vload16(size_t, short*);
short16 __OVERLOAD__ vload16(size_t, __local short*);
short16 __OVERLOAD__ vload16(size_t, __global short*);
short16 __OVERLOAD__ vload16(size_t, __constant short*);
ushort2 __OVERLOAD__ vload2(size_t, ushort*);
ushort2 __OVERLOAD__ vload2(size_t, __local ushort*);
ushort2 __OVERLOAD__ vload2(size_t, __global ushort*);
ushort2 __OVERLOAD__ vload2(size_t, __constant ushort*);
ushort3 __OVERLOAD__ vload3(size_t, ushort*);
ushort3 __OVERLOAD__ vload3(size_t, __local ushort*);
ushort3 __OVERLOAD__ vload3(size_t, __global ushort*);
ushort3 __OVERLOAD__ vload3(size_t, __constant ushort*);
ushort4 __OVERLOAD__ vload4(size_t, ushort*);
ushort4 __OVERLOAD__ vload4(size_t, __local ushort*);
ushort4 __OVERLOAD__ vload4(size_t, __global ushort*);
ushort4 __OVERLOAD__ vload4(size_t, __constant ushort*);
ushort8 __OVERLOAD__ vload8(size_t, ushort*);
ushort8 __OVERLOAD__ vload8(size_t, __local ushort*);
ushort8 __OVERLOAD__ vload8(size_t, __global ushort*);
ushort8 __OVERLOAD__ vload8(size_t, __constant ushort*);
ushort16 __OVERLOAD__ vload16(size_t, ushort*);
ushort16 __OVERLOAD__ vload16(size_t, __local ushort*);
ushort16 __OVERLOAD__ vload16(size_t, __global ushort*);
ushort16 __OVERLOAD__ vload16(size_t, __constant ushort*);
int2 __OVERLOAD__ vload2(size_t, int*);
int2 __OVERLOAD__ vload2(size_t, __local int*);
int2 __OVERLOAD__ vload2(size_t, __global int*);
int2 __OVERLOAD__ vload2(size_t, __constant int*);
int3 __OVERLOAD__ vload3(size_t, int*);
int3 __OVERLOAD__ vload3(size_t, __local int*);
int3 __OVERLOAD__ vload3(size_t, __global int*);
int3 __OVERLOAD__ vload3(size_t, __constant int*);
int4 __OVERLOAD__ vload4(size_t, int*);
int4 __OVERLOAD__ vload4(size_t, __local int*);
int4 __OVERLOAD__ vload4(size_t, __global int*);
int4 __OVERLOAD__ vload4(size_t, __constant int*);
int8 __OVERLOAD__ vload8(size_t, int*);
int8 __OVERLOAD__ vload8(size_t, __local int*);
int8 __OVERLOAD__ vload8(size_t, __global int*);
int8 __OVERLOAD__ vload8(size_t, __constant int*);
int16 __OVERLOAD__ vload16(size_t, int*);
int16 __OVERLOAD__ vload16(size_t, __local int*);
int16 __OVERLOAD__ vload16(size_t, __global int*);
int16 __OVERLOAD__ vload16(size_t, __constant int*);
uint2 __OVERLOAD__ vload2(size_t, uint*);
uint2 __OVERLOAD__ vload2(size_t, __local uint*);
uint2 __OVERLOAD__ vload2(size_t, __global uint*);
uint2 __OVERLOAD__ vload2(size_t, __constant uint*);
uint3 __OVERLOAD__ vload3(size_t, uint*);
uint3 __OVERLOAD__ vload3(size_t, __local uint*);
uint3 __OVERLOAD__ vload3(size_t, __global uint*);
uint3 __OVERLOAD__ vload3(size_t, __constant uint*);
uint4 __OVERLOAD__ vload4(size_t, uint*);
uint4 __OVERLOAD__ vload4(size_t, __local uint*);
uint4 __OVERLOAD__ vload4(size_t, __global uint*);
uint4 __OVERLOAD__ vload4(size_t, __constant uint*);
uint8 __OVERLOAD__ vload8(size_t, uint*);
uint8 __OVERLOAD__ vload8(size_t, __local uint*);
uint8 __OVERLOAD__ vload8(size_t, __global uint*);
uint8 __OVERLOAD__ vload8(size_t, __constant uint*);
uint16 __OVERLOAD__ vload16(size_t, uint*);
uint16 __OVERLOAD__ vload16(size_t, __local uint*);
uint16 __OVERLOAD__ vload16(size_t, __global uint*);
uint16 __OVERLOAD__ vload16(size_t, __constant uint*);
long2 __OVERLOAD__ vload2(size_t, long*);
long2 __OVERLOAD__ vload2(size_t, __local long*);
long2 __OVERLOAD__ vload2(size_t, __global long*);
long2 __OVERLOAD__ vload2(size_t, __constant long*);
long3 __OVERLOAD__ vload3(size_t, long*);
long3 __OVERLOAD__ vload3(size_t, __local long*);
long3 __OVERLOAD__ vload3(size_t, __global long*);
long3 __OVERLOAD__ vload3(size_t, __constant long*);
long4 __OVERLOAD__ vload4(size_t, long*);
long4 __OVERLOAD__ vload4(size_t, __local long*);
long4 __OVERLOAD__ vload4(size_t, __global long*);
long4 __OVERLOAD__ vload4(size_t, __constant long*);
long8 __OVERLOAD__ vload8(size_t, long*);
long8 __OVERLOAD__ vload8(size_t, __local long*);
long8 __OVERLOAD__ vload8(size_t, __global long*);
long8 __OVERLOAD__ vload8(size_t, __constant long*);
long16 __OVERLOAD__ vload16(size_t, long*);
long16 __OVERLOAD__ vload16(size_t, __local long*);
long16 __OVERLOAD__ vload16(size_t, __global long*);
long16 __OVERLOAD__ vload16(size_t, __constant long*);
ulong2 __OVERLOAD__ vload2(size_t, ulong*);
ulong2 __OVERLOAD__ vload2(size_t, __local ulong*);
ulong2 __OVERLOAD__ vload2(size_t, __global ulong*);
ulong2 __OVERLOAD__ vload2(size_t, __constant ulong*);
ulong3 __OVERLOAD__ vload3(size_t, ulong*);
ulong3 __OVERLOAD__ vload3(size_t, __local ulong*);
ulong3 __OVERLOAD__ vload3(size_t, __global ulong*);
ulong3 __OVERLOAD__ vload3(size_t, __constant ulong*);
ulong4 __OVERLOAD__ vload4(size_t, ulong*);
ulong4 __OVERLOAD__ vload4(size_t, __local ulong*);
ulong4 __OVERLOAD__ vload4(size_t, __global ulong*);
ulong4 __OVERLOAD__ vload4(size_t, __constant ulong*);
ulong8 __OVERLOAD__ vload8(size_t, ulong*);
ulong8 __OVERLOAD__ vload8(size_t, __local ulong*);
ulong8 __OVERLOAD__ vload8(size_t, __global ulong*);
ulong8 __OVERLOAD__ vload8(size_t, __constant ulong*);
ulong16 __OVERLOAD__ vload16(size_t, ulong*);
ulong16 __OVERLOAD__ vload16(size_t, __local ulong*);
ulong16 __OVERLOAD__ vload16(size_t, __global ulong*);
ulong16 __OVERLOAD__ vload16(size_t, __constant ulong*);
float2 __OVERLOAD__ vload2(size_t, float*);
float2 __OVERLOAD__ vload2(size_t, __local float*);
float2 __OVERLOAD__ vload2(size_t, __global float*);
float2 __OVERLOAD__ vload2(size_t, __constant float*);
float3 __OVERLOAD__ vload3(size_t, float*);
float3 __OVERLOAD__ vload3(size_t, __local float*);
float3 __OVERLOAD__ vload3(size_t, __global float*);
float3 __OVERLOAD__ vload3(size_t, __constant float*);
float4 __OVERLOAD__ vload4(size_t, float*);
float4 __OVERLOAD__ vload4(size_t, __local float*);
float4 __OVERLOAD__ vload4(size_t, __global float*);
float4 __OVERLOAD__ vload4(size_t, __constant float*);
float8 __OVERLOAD__ vload8(size_t, float*);
float8 __OVERLOAD__ vload8(size_t, __local float*);
float8 __OVERLOAD__ vload8(size_t, __global float*);
float8 __OVERLOAD__ vload8(size_t, __constant float*);
float16 __OVERLOAD__ vload16(size_t, float*);
float16 __OVERLOAD__ vload16(size_t, __local float*);
float16 __OVERLOAD__ vload16(size_t, __global float*);
float16 __OVERLOAD__ vload16(size_t, __constant float*);

void __OVERLOAD__ vstore2(char2, size_t, char*);
void __OVERLOAD__ vstore2(char2, size_t, __local char*);
void __OVERLOAD__ vstore2(char2, size_t, __global char*);
void __OVERLOAD__ vstore2(char2, size_t, __private char*);
void __OVERLOAD__ vstore3(char3, size_t, char*);
void __OVERLOAD__ vstore3(char3, size_t, __local char*);
void __OVERLOAD__ vstore3(char3, size_t, __global char*);
void __OVERLOAD__ vstore3(char3, size_t, __private char*);
void __OVERLOAD__ vstore4(char4, size_t, char*);
void __OVERLOAD__ vstore4(char4, size_t, __local char*);
void __OVERLOAD__ vstore4(char4, size_t, __global char*);
void __OVERLOAD__ vstore4(char4, size_t, __private char*);
void __OVERLOAD__ vstore8(char8, size_t, char*);
void __OVERLOAD__ vstore8(char8, size_t, __local char*);
void __OVERLOAD__ vstore8(char8, size_t, __global char*);
void __OVERLOAD__ vstore8(char8, size_t, __private char*);
void __OVERLOAD__ vstore16(char16, size_t, char*);
void __OVERLOAD__ vstore16(char16, size_t, __local char*);
void __OVERLOAD__ vstore16(char16, size_t, __global char*);
void __OVERLOAD__ vstore16(char16, size_t, __private char*);
void __OVERLOAD__ vstore2(uchar2, size_t, uchar*);
void __OVERLOAD__ vstore2(uchar2, size_t, __local uchar*);
void __OVERLOAD__ vstore2(uchar2, size_t, __global uchar*);
void __OVERLOAD__ vstore2(uchar2, size_t, __private uchar*);
void __OVERLOAD__ vstore3(uchar3, size_t, uchar*);
void __OVERLOAD__ vstore3(uchar3, size_t, __local uchar*);
void __OVERLOAD__ vstore3(uchar3, size_t, __global uchar*);
void __OVERLOAD__ vstore3(uchar3, size_t, __private uchar*);
void __OVERLOAD__ vstore4(uchar4, size_t, uchar*);
void __OVERLOAD__ vstore4(uchar4, size_t, __local uchar*);
void __OVERLOAD__ vstore4(uchar4, size_t, __global uchar*);
void __OVERLOAD__ vstore4(uchar4, size_t, __private uchar*);
void __OVERLOAD__ vstore8(uchar8, size_t, uchar*);
void __OVERLOAD__ vstore8(uchar8, size_t, __local uchar*);
void __OVERLOAD__ vstore8(uchar8, size_t, __global uchar*);
void __OVERLOAD__ vstore8(uchar8, size_t, __private uchar*);
void __OVERLOAD__ vstore16(uchar16, size_t, uchar*);
void __OVERLOAD__ vstore16(uchar16, size_t, __local uchar*);
void __OVERLOAD__ vstore16(uchar16, size_t, __global uchar*);
void __OVERLOAD__ vstore16(uchar16, size_t, __private uchar*);
void __OVERLOAD__ vstore2(short2, size_t, short*);
void __OVERLOAD__ vstore2(short2, size_t, __local short*);
void __OVERLOAD__ vstore2(short2, size_t, __global short*);
void __OVERLOAD__ vstore2(short2, size_t, __private short*);
void __OVERLOAD__ vstore3(short3, size_t, short*);
void __OVERLOAD__ vstore3(short3, size_t, __local short*);
void __OVERLOAD__ vstore3(short3, size_t, __global short*);
void __OVERLOAD__ vstore3(short3, size_t, __private short*);
void __OVERLOAD__ vstore4(short4, size_t, short*);
void __OVERLOAD__ vstore4(short4, size_t, __local short*);
void __OVERLOAD__ vstore4(short4, size_t, __global short*);
void __OVERLOAD__ vstore4(short4, size_t, __private short*);
void __OVERLOAD__ vstore8(short8, size_t, short*);
void __OVERLOAD__ vstore8(short8, size_t, __local short*);
void __OVERLOAD__ vstore8(short8, size_t, __global short*);
void __OVERLOAD__ vstore8(short8, size_t, __private short*);
void __OVERLOAD__ vstore16(short16, size_t, short*);
void __OVERLOAD__ vstore16(short16, size_t, __local short*);
void __OVERLOAD__ vstore16(short16, size_t, __global short*);
void __OVERLOAD__ vstore16(short16, size_t, __private short*);
void __OVERLOAD__ vstore2(ushort2, size_t, ushort*);
void __OVERLOAD__ vstore2(ushort2, size_t, __local ushort*);
void __OVERLOAD__ vstore2(ushort2, size_t, __global ushort*);
void __OVERLOAD__ vstore2(ushort2, size_t, __private ushort*);
void __OVERLOAD__ vstore3(ushort3, size_t, ushort*);
void __OVERLOAD__ vstore3(ushort3, size_t, __local ushort*);
void __OVERLOAD__ vstore3(ushort3, size_t, __global ushort*);
void __OVERLOAD__ vstore3(ushort3, size_t, __private ushort*);
void __OVERLOAD__ vstore4(ushort4, size_t, ushort*);
void __OVERLOAD__ vstore4(ushort4, size_t, __local ushort*);
void __OVERLOAD__ vstore4(ushort4, size_t, __global ushort*);
void __OVERLOAD__ vstore4(ushort4, size_t, __private ushort*);
void __OVERLOAD__ vstore8(ushort8, size_t, ushort*);
void __OVERLOAD__ vstore8(ushort8, size_t, __local ushort*);
void __OVERLOAD__ vstore8(ushort8, size_t, __global ushort*);
void __OVERLOAD__ vstore8(ushort8, size_t, __private ushort*);
void __OVERLOAD__ vstore16(ushort16, size_t, ushort*);
void __OVERLOAD__ vstore16(ushort16, size_t, __local ushort*);
void __OVERLOAD__ vstore16(ushort16, size_t, __global ushort*);
void __OVERLOAD__ vstore16(ushort16, size_t, __private ushort*);
void __OVERLOAD__ vstore2(int2, size_t, int*);
void __OVERLOAD__ vstore2(int2, size_t, __local int*);
void __OVERLOAD__ vstore2(int2, size_t, __global int*);
void __OVERLOAD__ vstore2(int2, size_t, __private int*);
void __OVERLOAD__ vstore3(int3, size_t, int*);
void __OVERLOAD__ vstore3(int3, size_t, __local int*);
void __OVERLOAD__ vstore3(int3, size_t, __global int*);
void __OVERLOAD__ vstore3(int3, size_t, __private int*);
void __OVERLOAD__ vstore4(int4, size_t, int*);
void __OVERLOAD__ vstore4(int4, size_t, __local int*);
void __OVERLOAD__ vstore4(int4, size_t, __global int*);
void __OVERLOAD__ vstore4(int4, size_t, __private int*);
void __OVERLOAD__ vstore8(int8, size_t, int*);
void __OVERLOAD__ vstore8(int8, size_t, __local int*);
void __OVERLOAD__ vstore8(int8, size_t, __global int*);
void __OVERLOAD__ vstore8(int8, size_t, __private int*);
void __OVERLOAD__ vstore16(int16, size_t, int*);
void __OVERLOAD__ vstore16(int16, size_t, __local int*);
void __OVERLOAD__ vstore16(int16, size_t, __global int*);
void __OVERLOAD__ vstore16(int16, size_t, __private int*);
void __OVERLOAD__ vstore2(uint2, size_t, uint*);
void __OVERLOAD__ vstore2(uint2, size_t, __local uint*);
void __OVERLOAD__ vstore2(uint2, size_t, __global uint*);
void __OVERLOAD__ vstore2(uint2, size_t, __private uint*);
void __OVERLOAD__ vstore3(uint3, size_t, uint*);
void __OVERLOAD__ vstore3(uint3, size_t, __local uint*);
void __OVERLOAD__ vstore3(uint3, size_t, __global uint*);
void __OVERLOAD__ vstore3(uint3, size_t, __private uint*);
void __OVERLOAD__ vstore4(uint4, size_t, uint*);
void __OVERLOAD__ vstore4(uint4, size_t, __local uint*);
void __OVERLOAD__ vstore4(uint4, size_t, __global uint*);
void __OVERLOAD__ vstore4(uint4, size_t, __private uint*);
void __OVERLOAD__ vstore8(uint8, size_t, uint*);
void __OVERLOAD__ vstore8(uint8, size_t, __local uint*);
void __OVERLOAD__ vstore8(uint8, size_t, __global uint*);
void __OVERLOAD__ vstore8(uint8, size_t, __private uint*);
void __OVERLOAD__ vstore16(uint16, size_t, uint*);
void __OVERLOAD__ vstore16(uint16, size_t, __local uint*);
void __OVERLOAD__ vstore16(uint16, size_t, __global uint*);
void __OVERLOAD__ vstore16(uint16, size_t, __private uint*);
void __OVERLOAD__ vstore2(long2, size_t, long*);
void __OVERLOAD__ vstore2(long2, size_t, __local long*);
void __OVERLOAD__ vstore2(long2, size_t, __global long*);
void __OVERLOAD__ vstore2(long2, size_t, __private long*);
void __OVERLOAD__ vstore3(long3, size_t, long*);
void __OVERLOAD__ vstore3(long3, size_t, __local long*);
void __OVERLOAD__ vstore3(long3, size_t, __global long*);
void __OVERLOAD__ vstore3(long3, size_t, __private long*);
void __OVERLOAD__ vstore4(long4, size_t, long*);
void __OVERLOAD__ vstore4(long4, size_t, __local long*);
void __OVERLOAD__ vstore4(long4, size_t, __global long*);
void __OVERLOAD__ vstore4(long4, size_t, __private long*);
void __OVERLOAD__ vstore8(long8, size_t, long*);
void __OVERLOAD__ vstore8(long8, size_t, __local long*);
void __OVERLOAD__ vstore8(long8, size_t, __global long*);
void __OVERLOAD__ vstore8(long8, size_t, __private long*);
void __OVERLOAD__ vstore16(long16, size_t, long*);
void __OVERLOAD__ vstore16(long16, size_t, __local long*);
void __OVERLOAD__ vstore16(long16, size_t, __global long*);
void __OVERLOAD__ vstore16(long16, size_t, __private long*);
void __OVERLOAD__ vstore2(ulong2, size_t, ulong*);
void __OVERLOAD__ vstore2(ulong2, size_t, __local ulong*);
void __OVERLOAD__ vstore2(ulong2, size_t, __global ulong*);
void __OVERLOAD__ vstore2(ulong2, size_t, __private ulong*);
void __OVERLOAD__ vstore3(ulong3, size_t, ulong*);
void __OVERLOAD__ vstore3(ulong3, size_t, __local ulong*);
void __OVERLOAD__ vstore3(ulong3, size_t, __global ulong*);
void __OVERLOAD__ vstore3(ulong3, size_t, __private ulong*);
void __OVERLOAD__ vstore4(ulong4, size_t, ulong*);
void __OVERLOAD__ vstore4(ulong4, size_t, __local ulong*);
void __OVERLOAD__ vstore4(ulong4, size_t, __global ulong*);
void __OVERLOAD__ vstore4(ulong4, size_t, __private ulong*);
void __OVERLOAD__ vstore8(ulong8, size_t, ulong*);
void __OVERLOAD__ vstore8(ulong8, size_t, __local ulong*);
void __OVERLOAD__ vstore8(ulong8, size_t, __global ulong*);
void __OVERLOAD__ vstore8(ulong8, size_t, __private ulong*);
void __OVERLOAD__ vstore16(ulong16, size_t, ulong*);
void __OVERLOAD__ vstore16(ulong16, size_t, __local ulong*);
void __OVERLOAD__ vstore16(ulong16, size_t, __global ulong*);
void __OVERLOAD__ vstore16(ulong16, size_t, __private ulong*);
void __OVERLOAD__ vstore2(float2, size_t, float*);
void __OVERLOAD__ vstore2(float2, size_t, __local float*);
void __OVERLOAD__ vstore2(float2, size_t, __global float*);
void __OVERLOAD__ vstore2(float2, size_t, __private float*);
void __OVERLOAD__ vstore3(float3, size_t, float*);
void __OVERLOAD__ vstore3(float3, size_t, __local float*);
void __OVERLOAD__ vstore3(float3, size_t, __global float*);
void __OVERLOAD__ vstore3(float3, size_t, __private float*);
void __OVERLOAD__ vstore4(float4, size_t, float*);
void __OVERLOAD__ vstore4(float4, size_t, __local float*);
void __OVERLOAD__ vstore4(float4, size_t, __global float*);
void __OVERLOAD__ vstore4(float4, size_t, __private float*);
void __OVERLOAD__ vstore8(float8, size_t, float*);
void __OVERLOAD__ vstore8(float8, size_t, __local float*);
void __OVERLOAD__ vstore8(float8, size_t, __global float*);
void __OVERLOAD__ vstore8(float8, size_t, __private float*);
void __OVERLOAD__ vstore16(float16, size_t, float*);
void __OVERLOAD__ vstore16(float16, size_t, __local float*);
void __OVERLOAD__ vstore16(float16, size_t, __global float*);
void __OVERLOAD__ vstore16(float16, size_t, __private float*);

#define as_char( _x )  __builtin_astype( _x, char )
#define as_char2( _x )  __builtin_astype( _x, char2 )
#define as_char3( _x )  __builtin_astype( _x, char3 )
#define as_char4( _x )  __builtin_astype( _x, char4 )
#define as_char8( _x )  __builtin_astype( _x, char8 )
#define as_char16( _x )  __builtin_astype( _x, char16 )
#define as_uchar( _x )  __builtin_astype( _x, uchar )
#define as_uchar2( _x )  __builtin_astype( _x, uchar2 )
#define as_uchar3( _x )  __builtin_astype( _x, uchar3 )
#define as_uchar4( _x )  __builtin_astype( _x, uchar4 )
#define as_uchar8( _x )  __builtin_astype( _x, uchar8 )
#define as_uchar16( _x )  __builtin_astype( _x, uchar16 )
#define as_short( _x )  __builtin_astype( _x, short )
#define as_short2( _x )  __builtin_astype( _x, short2 )
#define as_short3( _x )  __builtin_astype( _x, short3 )
#define as_short4( _x )  __builtin_astype( _x, short4 )
#define as_short8( _x )  __builtin_astype( _x, short8 )
#define as_short16( _x )  __builtin_astype( _x, short16 )
#define as_ushort( _x )  __builtin_astype( _x, ushort )
#define as_ushort2( _x )  __builtin_astype( _x, ushort2 )
#define as_ushort3( _x )  __builtin_astype( _x, ushort3 )
#define as_ushort4( _x )  __builtin_astype( _x, ushort4 )
#define as_ushort8( _x )  __builtin_astype( _x, ushort8 )
#define as_ushort16( _x )  __builtin_astype( _x, ushort16 )
#define as_int( _x )  __builtin_astype( _x, int )
#define as_int2( _x )  __builtin_astype( _x, int2 )
#define as_int3( _x )  __builtin_astype( _x, int3 )
#define as_int4( _x )  __builtin_astype( _x, int4 )
#define as_int8( _x )  __builtin_astype( _x, int8 )
#define as_int16( _x )  __builtin_astype( _x, int16 )
#define as_uint( _x )  __builtin_astype( _x, uint )
#define as_uint2( _x )  __builtin_astype( _x, uint2 )
#define as_uint3( _x )  __builtin_astype( _x, uint3 )
#define as_uint4( _x )  __builtin_astype( _x, uint4 )
#define as_uint8( _x )  __builtin_astype( _x, uint8 )
#define as_uint16( _x )  __builtin_astype( _x, uint16 )
#define as_long( _x )  __builtin_astype( _x, long )
#define as_long2( _x )  __builtin_astype( _x, long2 )
#define as_long3( _x )  __builtin_astype( _x, long3 )
#define as_long4( _x )  __builtin_astype( _x, long4 )
#define as_long8( _x )  __builtin_astype( _x, long8 )
#define as_long16( _x )  __builtin_astype( _x, long16 )
#define as_ulong( _x )  __builtin_astype( _x, ulong )
#define as_ulong2( _x )  __builtin_astype( _x, ulong2 )
#define as_ulong3( _x )  __builtin_astype( _x, ulong3 )
#define as_ulong4( _x )  __builtin_astype( _x, ulong4 )
#define as_ulong8( _x )  __builtin_astype( _x, ulong8 )
#define as_ulong16( _x )  __builtin_astype( _x, ulong16 )
#define as_float( _x )  __builtin_astype( _x, float )
#define as_float2( _x )  __builtin_astype( _x, float2 )
#define as_float3( _x )  __builtin_astype( _x, float3 )
#define as_float4( _x )  __builtin_astype( _x, float4 )
#define as_float8( _x )  __builtin_astype( _x, float8 )
#define as_float16( _x )  __builtin_astype( _x, float16 )

event_t __OVERLOAD__ async_work_group_copy(__local char*, const __global char*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local char2*, const __global char2*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local char3*, const __global char3*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local char4*, const __global char4*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local char8*, const __global char8*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local char16*, const __global char16*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local uchar*, const __global uchar*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local uchar2*, const __global uchar2*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local uchar3*, const __global uchar3*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local uchar4*, const __global uchar4*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local uchar8*, const __global uchar8*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local uchar16*, const __global uchar16*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local short*, const __global short*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local short2*, const __global short2*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local short3*, const __global short3*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local short4*, const __global short4*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local short8*, const __global short8*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local short16*, const __global short16*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local ushort*, const __global ushort*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local ushort2*, const __global ushort2*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local ushort3*, const __global ushort3*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local ushort4*, const __global ushort4*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local ushort8*, const __global ushort8*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local ushort16*, const __global ushort16*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local int*, const __global int*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local int2*, const __global int2*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local int3*, const __global int3*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local int4*, const __global int4*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local int8*, const __global int8*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local int16*, const __global int16*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local uint*, const __global uint*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local uint2*, const __global uint2*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local uint3*, const __global uint3*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local uint4*, const __global uint4*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local uint8*, const __global uint8*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local uint16*, const __global uint16*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local long*, const __global long*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local long2*, const __global long2*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local long3*, const __global long3*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local long4*, const __global long4*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local long8*, const __global long8*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local long16*, const __global long16*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local ulong*, const __global ulong*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local ulong2*, const __global ulong2*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local ulong3*, const __global ulong3*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local ulong4*, const __global ulong4*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local ulong8*, const __global ulong8*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local ulong16*, const __global ulong16*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local float*, const __global float*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local float2*, const __global float2*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local float3*, const __global float3*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local float4*, const __global float4*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local float8*, const __global float8*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__local float16*, const __global float16*, size_t, event_t);

event_t __OVERLOAD__ async_work_group_copy(__global char*, const __local char*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global char2*, const __local char2*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global char3*, const __local char3*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global char4*, const __local char4*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global char8*, const __local char8*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global char16*, const __local char16*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global uchar*, const __local uchar*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global uchar2*, const __local uchar2*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global uchar3*, const __local uchar3*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global uchar4*, const __local uchar4*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global uchar8*, const __local uchar8*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global uchar16*, const __local uchar16*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global short*, const __local short*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global short2*, const __local short2*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global short3*, const __local short3*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global short4*, const __local short4*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global short8*, const __local short8*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global short16*, const __local short16*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global ushort*, const __local ushort*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global ushort2*, const __local ushort2*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global ushort3*, const __local ushort3*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global ushort4*, const __local ushort4*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global ushort8*, const __local ushort8*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global ushort16*, const __local ushort16*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global int*, const __local int*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global int2*, const __local int2*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global int3*, const __local int3*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global int4*, const __local int4*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global int8*, const __local int8*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global int16*, const __local int16*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global uint*, const __local uint*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global uint2*, const __local uint2*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global uint3*, const __local uint3*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global uint4*, const __local uint4*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global uint8*, const __local uint8*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global uint16*, const __local uint16*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global long*, const __local long*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global long2*, const __local long2*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global long3*, const __local long3*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global long4*, const __local long4*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global long8*, const __local long8*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global long16*, const __local long16*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global ulong*, const __local ulong*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global ulong2*, const __local ulong2*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global ulong3*, const __local ulong3*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global ulong4*, const __local ulong4*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global ulong8*, const __local ulong8*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global ulong16*, const __local ulong16*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global float*, const __local float*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global float2*, const __local float2*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global float3*, const __local float3*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global float4*, const __local float4*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global float8*, const __local float8*, size_t, event_t);
event_t __OVERLOAD__ async_work_group_copy(__global float16*, const __local float16*, size_t, event_t);

event_t __OVERLOAD__ async_work_group_strided_copy(__local char*, const __global char*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local char2*, const __global char2*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local char3*, const __global char3*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local char4*, const __global char4*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local char8*, const __global char8*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local char16*, const __global char16*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local uchar*, const __global uchar*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local uchar2*, const __global uchar2*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local uchar3*, const __global uchar3*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local uchar4*, const __global uchar4*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local uchar8*, const __global uchar8*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local uchar16*, const __global uchar16*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local short*, const __global short*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local short2*, const __global short2*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local short3*, const __global short3*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local short4*, const __global short4*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local short8*, const __global short8*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local short16*, const __global short16*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local ushort*, const __global ushort*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local ushort2*, const __global ushort2*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local ushort3*, const __global ushort3*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local ushort4*, const __global ushort4*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local ushort8*, const __global ushort8*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local ushort16*, const __global ushort16*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local int*, const __global int*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local int2*, const __global int2*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local int3*, const __global int3*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local int4*, const __global int4*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local int8*, const __global int8*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local int16*, const __global int16*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local uint*, const __global uint*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local uint2*, const __global uint2*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local uint3*, const __global uint3*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local uint4*, const __global uint4*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local uint8*, const __global uint8*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local uint16*, const __global uint16*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local long*, const __global long*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local long2*, const __global long2*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local long3*, const __global long3*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local long4*, const __global long4*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local long8*, const __global long8*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local long16*, const __global long16*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local ulong*, const __global ulong*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local ulong2*, const __global ulong2*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local ulong3*, const __global ulong3*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local ulong4*, const __global ulong4*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local ulong8*, const __global ulong8*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local ulong16*, const __global ulong16*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local float*, const __global float*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local float2*, const __global float2*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local float3*, const __global float3*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local float4*, const __global float4*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local float8*, const __global float8*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__local float16*, const __global float16*, size_t, size_t, event_t);

event_t __OVERLOAD__ async_work_group_strided_copy(__global char*, const __local char*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global char2*, const __local char2*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global char3*, const __local char3*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global char4*, const __local char4*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global char8*, const __local char8*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global char16*, const __local char16*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global uchar*, const __local uchar*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global uchar2*, const __local uchar2*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global uchar3*, const __local uchar3*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global uchar4*, const __local uchar4*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global uchar8*, const __local uchar8*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global uchar16*, const __local uchar16*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global short*, const __local short*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global short2*, const __local short2*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global short3*, const __local short3*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global short4*, const __local short4*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global short8*, const __local short8*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global short16*, const __local short16*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global ushort*, const __local ushort*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global ushort2*, const __local ushort2*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global ushort3*, const __local ushort3*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global ushort4*, const __local ushort4*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global ushort8*, const __local ushort8*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global ushort16*, const __local ushort16*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global int*, const __local int*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global int2*, const __local int2*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global int3*, const __local int3*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global int4*, const __local int4*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global int8*, const __local int8*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global int16*, const __local int16*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global uint*, const __local uint*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global uint2*, const __local uint2*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global uint3*, const __local uint3*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global uint4*, const __local uint4*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global uint8*, const __local uint8*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global uint16*, const __local uint16*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global long*, const __local long*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global long2*, const __local long2*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global long3*, const __local long3*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global long4*, const __local long4*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global long8*, const __local long8*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global long16*, const __local long16*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global ulong*, const __local ulong*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global ulong2*, const __local ulong2*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global ulong3*, const __local ulong3*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global ulong4*, const __local ulong4*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global ulong8*, const __local ulong8*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global ulong16*, const __local ulong16*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global float*, const __local float*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global float2*, const __local float2*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global float3*, const __local float3*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global float4*, const __local float4*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global float8*, const __local float8*, size_t, size_t, event_t);
event_t __OVERLOAD__ async_work_group_strided_copy(__global float16*, const __local float16*, size_t, size_t, event_t);

void wait_group_events(int, event_t*);

void __OVERLOAD__ prefetch(const __global char*, size_t);
void __OVERLOAD__ prefetch(const __global char2*, size_t);
void __OVERLOAD__ prefetch(const __global char3*, size_t);
void __OVERLOAD__ prefetch(const __global char4*, size_t);
void __OVERLOAD__ prefetch(const __global char8*, size_t);
void __OVERLOAD__ prefetch(const __global char16*, size_t);
void __OVERLOAD__ prefetch(const __global uchar*, size_t);
void __OVERLOAD__ prefetch(const __global uchar2*, size_t);
void __OVERLOAD__ prefetch(const __global uchar3*, size_t);
void __OVERLOAD__ prefetch(const __global uchar4*, size_t);
void __OVERLOAD__ prefetch(const __global uchar8*, size_t);
void __OVERLOAD__ prefetch(const __global uchar16*, size_t);
void __OVERLOAD__ prefetch(const __global short*, size_t);
void __OVERLOAD__ prefetch(const __global short2*, size_t);
void __OVERLOAD__ prefetch(const __global short3*, size_t);
void __OVERLOAD__ prefetch(const __global short4*, size_t);
void __OVERLOAD__ prefetch(const __global short8*, size_t);
void __OVERLOAD__ prefetch(const __global short16*, size_t);
void __OVERLOAD__ prefetch(const __global ushort*, size_t);
void __OVERLOAD__ prefetch(const __global ushort2*, size_t);
void __OVERLOAD__ prefetch(const __global ushort3*, size_t);
void __OVERLOAD__ prefetch(const __global ushort4*, size_t);
void __OVERLOAD__ prefetch(const __global ushort8*, size_t);
void __OVERLOAD__ prefetch(const __global ushort16*, size_t);
void __OVERLOAD__ prefetch(const __global int*, size_t);
void __OVERLOAD__ prefetch(const __global int2*, size_t);
void __OVERLOAD__ prefetch(const __global int3*, size_t);
void __OVERLOAD__ prefetch(const __global int4*, size_t);
void __OVERLOAD__ prefetch(const __global int8*, size_t);
void __OVERLOAD__ prefetch(const __global int16*, size_t);
void __OVERLOAD__ prefetch(const __global uint*, size_t);
void __OVERLOAD__ prefetch(const __global uint2*, size_t);
void __OVERLOAD__ prefetch(const __global uint3*, size_t);
void __OVERLOAD__ prefetch(const __global uint4*, size_t);
void __OVERLOAD__ prefetch(const __global uint8*, size_t);
void __OVERLOAD__ prefetch(const __global uint16*, size_t);
void __OVERLOAD__ prefetch(const __global long*, size_t);
void __OVERLOAD__ prefetch(const __global long2*, size_t);
void __OVERLOAD__ prefetch(const __global long3*, size_t);
void __OVERLOAD__ prefetch(const __global long4*, size_t);
void __OVERLOAD__ prefetch(const __global long8*, size_t);
void __OVERLOAD__ prefetch(const __global long16*, size_t);
void __OVERLOAD__ prefetch(const __global ulong*, size_t);
void __OVERLOAD__ prefetch(const __global ulong2*, size_t);
void __OVERLOAD__ prefetch(const __global ulong3*, size_t);
void __OVERLOAD__ prefetch(const __global ulong4*, size_t);
void __OVERLOAD__ prefetch(const __global ulong8*, size_t);
void __OVERLOAD__ prefetch(const __global ulong16*, size_t);
void __OVERLOAD__ prefetch(const __global float*, size_t);
void __OVERLOAD__ prefetch(const __global float2*, size_t);
void __OVERLOAD__ prefetch(const __global float3*, size_t);
void __OVERLOAD__ prefetch(const __global float4*, size_t);
void __OVERLOAD__ prefetch(const __global float8*, size_t);
void __OVERLOAD__ prefetch(const __global float16*, size_t);
