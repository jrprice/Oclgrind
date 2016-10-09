// clc.h (Oclgrind)
// Copyright (c) 2013-2016, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

#if defined(__SPIR32__)
  typedef uint size_t;
  typedef int ptrdiff_t;
#else
  typedef ulong size_t;
  typedef long ptrdiff_t;
#endif
typedef size_t uintptr_t;
typedef ptrdiff_t intptr_t;

#define event_t size_t

#define TYPEDEF_VECTOR(type)                                \
  typedef __attribute__((ext_vector_type(2))) type type##2; \
  typedef __attribute__((ext_vector_type(3))) type type##3; \
  typedef __attribute__((ext_vector_type(4))) type type##4; \
  typedef __attribute__((ext_vector_type(8))) type type##8; \
  typedef __attribute__((ext_vector_type(16))) type type##16;
TYPEDEF_VECTOR(char);
TYPEDEF_VECTOR(uchar);
TYPEDEF_VECTOR(short);
TYPEDEF_VECTOR(ushort);
TYPEDEF_VECTOR(int);
TYPEDEF_VECTOR(uint);
TYPEDEF_VECTOR(long);
TYPEDEF_VECTOR(ulong);
TYPEDEF_VECTOR(float);
TYPEDEF_VECTOR(double);

#define __ENDIAN_LITTLE__ 1
#define __OPENCL_VERSION__ 120
#define __OPENCL_C_VERSION__ 120
#define __IMAGE_SUPPORT__ 1
#define __kernel_exec(X, typen) __kernel                        \
  __attribute__((work_group_size_hint(X, 1, 1)))                \
  __attribute__((vec_type_hint(typen)))

#define CHAR_BIT    8
#define SCHAR_MAX 127
#define SCHAR_MIN (-128)
#define UCHAR_MAX 255
#define CHAR_MAX  SCHAR_MAX
#define CHAR_MIN  SCHAR_MIN
#define USHRT_MAX 65535
#define SHRT_MAX  32767
#define SHRT_MIN  (-32768)
#define UINT_MAX  0xffffffff
#define INT_MAX   2147483647
#define INT_MIN   (-2147483647-1)
#define ULONG_MAX 0xffffffffffffffffUL
#define LONG_MAX  ((long)0x7fffffffffffffffL)
#define LONG_MIN  ((long)(-0x7fffffffffffffffL-1))

#define FLT_DIG         6
#define FLT_MANT_DIG    24
#define FLT_MAX_10_EXP  +38
#define FLT_MAX_EXP     +128
#define FLT_MIN_10_EXP  -37
#define FLT_MIN_EXP     -125
#define FLT_RADIX       2
#define FLT_MAX         0x1.fffffep127f
#define FLT_MIN         0x1.0p-126f
#define FLT_EPSILON     0x1.0p-23f

#define DBL_DIG         15
#define DBL_MANT_DIG    53
#define DBL_MAX_10_EXP  +308
#define DBL_MAX_EXP     +1024
#define DBL_MIN_10_EXP  -307
#define DBL_MIN_EXP     -1021
#define DBL_RADIX       2
#define DBL_MAX         0x1.fffffffffffffp1023
#define DBL_MIN         0x1.0p-1022
#define DBL_EPSILON     0x1.0p-52

#define FP_ILOGB0       INT_MIN
#define FP_ILOGBNAN     INT_MIN

#define M_E_F         2.71828182845904523536028747135266250f
#define M_LOG2E_F     1.44269504088896340735992468100189214f
#define M_LOG10E_F    0.434294481903251827651128918916605082f
#define M_LN2_F       0.693147180559945309417232121458176568f
#define M_LN10_F      2.3025850929940456840179914546843642f
#define M_PI_F        3.14159265358979323846264338327950288f
#define M_PI_2_F      1.57079632679489661923132169163975144f
#define M_PI_4_F      0.785398163397448309615660845819875721f
#define M_1_PI_F      0.318309886183790671537767526745028724f
#define M_2_PI_F      0.636619772367581343075535053490057448f
#define M_2_SQRTPI_F  1.12837916709551257389615890312154517f
#define M_SQRT2_F     1.41421356237309504880168872420969808f
#define M_SQRT1_2_F   0.707106781186547524400844362104849039f

#define M_E         2.71828182845904523536028747135266250
#define M_LOG2E     1.44269504088896340735992468100189214
#define M_LOG10E    0.434294481903251827651128918916605082
#define M_LN2       0.693147180559945309417232121458176568
#define M_LN10      2.30258509299404568401799145468436421
#define M_PI        3.14159265358979323846264338327950288
#define M_PI_2      1.57079632679489661923132169163975144
#define M_PI_4      0.785398163397448309615660845819875721
#define M_1_PI      0.318309886183790671537767526745028724
#define M_2_PI      0.636619772367581343075535053490057448
#define M_2_SQRTPI  1.12837916709551257389615890312154517
#define M_SQRT2     1.41421356237309504880168872420969808
#define M_SQRT1_2   0.707106781186547524400844362104849039

#define MAXFLOAT ((float)3.40282346638528860e+38)
#define HUGE_VALF __builtin_huge_valf()
#define HUGE_VAL __builtin_huge_val()
#define INFINITY __builtin_inff()
#define NAN __builtin_nanf(0)

#define CLK_SNORM_INT8 0x10D0
#define CLK_SNORM_INT16 0x10D1
#define CLK_UNORM_INT8 0x10D2
#define CLK_UNORM_INT16 0x10D3
#define CLK_UNORM_SHORT_565 0x10D4
#define CLK_UNORM_SHORT_555 0x10D5
#define CLK_UNORM_INT_101010 0x10D6
#define CLK_SIGNED_INT8 0x10D7
#define CLK_SIGNED_INT16 0x10D8
#define CLK_SIGNED_INT32 0x10D9
#define CLK_UNSIGNED_INT8 0x10DA
#define CLK_UNSIGNED_INT16 0x10DB
#define CLK_UNSIGNED_INT32 0x10DC
#define CLK_HALF_FLOAT 0x10DD
#define CLK_FLOAT 0x10DE
#define CLK_UNORM_INT24 0x10DF

#define CLK_R 0x10B0
#define CLK_A 0x10B1
#define CLK_RG 0x10B2
#define CLK_RA 0x10B3
#define CLK_RGB 0x10B4
#define CLK_RGBA 0x10B5
#define CLK_BGRA 0x10B6
#define CLK_ARGB 0x10B7
#define CLK_INTENSITY 0x10B8
#define CLK_LUMINANCE 0x10B9
#define CLK_Rx 0x10BA
#define CLK_RGx 0x10BB
#define CLK_RGBx 0x10BC
#define CLK_DEPTH 0x10BD
#define CLK_DEPTH_STENCIL 0x10BE

#define CLK_NORMALIZED_COORDS_FALSE 0x0000
#define CLK_NORMALIZED_COORDS_TRUE 0x0001

#define CLK_ADDRESS_NONE 0x0000
#define CLK_ADDRESS_CLAMP_TO_EDGE 0x0002
#define CLK_ADDRESS_CLAMP 0x0004
#define CLK_ADDRESS_REPEAT 0x0006
#define CLK_ADDRESS_MIRRORED_REPEAT 0x0008

#define CLK_FILTER_NEAREST 0x0010
#define CLK_FILTER_LINEAR 0x0020

#define __OVERLOAD__ __attribute__((__overloadable__))

#define BUILTIN_1ARG(rtype, type0, name)  \
  rtype __OVERLOAD__ name(type0 a);       \
  rtype##2 __OVERLOAD__ name(type0##2 a); \
  rtype##3 __OVERLOAD__ name(type0##3 a); \
  rtype##4 __OVERLOAD__ name(type0##4 a); \
  rtype##8 __OVERLOAD__ name(type0##8 a); \
  rtype##16 __OVERLOAD__ name(type0##16 a);
#define BUILTIN_2ARG(rtype, type0, type1, name)       \
  rtype __OVERLOAD__ name(type0 a, type1 b);          \
  rtype##2 __OVERLOAD__ name(type0##2 a, type1##2 b); \
  rtype##3 __OVERLOAD__ name(type0##3 a, type1##3 b); \
  rtype##4 __OVERLOAD__ name(type0##4 a, type1##4 b); \
  rtype##8 __OVERLOAD__ name(type0##8 a, type1##8 b); \
  rtype##16 __OVERLOAD__ name(type0##16 a, type1##16 b);
#define BUILTIN_3ARG(rtype, type0, type1, type2, name)            \
  rtype __OVERLOAD__ name(type0 a, type1 b, type2 c);             \
  rtype##2 __OVERLOAD__ name(type0##2 a, type1##2 b, type2##2 c); \
  rtype##3 __OVERLOAD__ name(type0##3 a, type1##3 b, type2##3 c); \
  rtype##4 __OVERLOAD__ name(type0##4 a, type1##4 b, type2##4 c); \
  rtype##8 __OVERLOAD__ name(type0##8 a, type1##8 b, type2##8 c); \
  rtype##16 __OVERLOAD__ name(type0##16 a, type1##16 b, type2##16 c);

#define BUILTIN_1ARG_INTEGERS(name)  \
  BUILTIN_1ARG(char, char, name)     \
  BUILTIN_1ARG(uchar, uchar, name)   \
  BUILTIN_1ARG(short, short, name)   \
  BUILTIN_1ARG(ushort, ushort, name) \
  BUILTIN_1ARG(int, int, name)       \
  BUILTIN_1ARG(uint, uint, name)     \
  BUILTIN_1ARG(long, long, name)     \
  BUILTIN_1ARG(ulong, ulong, name);
#define BUILTIN_2ARG_INTEGERS(name)          \
  BUILTIN_2ARG(char, char, char, name)       \
  BUILTIN_2ARG(uchar, uchar, uchar, name)    \
  BUILTIN_2ARG(short, short, short, name)    \
  BUILTIN_2ARG(ushort, ushort, ushort, name) \
  BUILTIN_2ARG(int, int, int, name)          \
  BUILTIN_2ARG(uint, uint, uint, name)       \
  BUILTIN_2ARG(long, long, long, name)       \
  BUILTIN_2ARG(ulong, ulong, ulong, name);
#define BUILTIN_3ARG_INTEGERS(name)                  \
  BUILTIN_3ARG(char, char, char, char, name)         \
  BUILTIN_3ARG(uchar, uchar, uchar, uchar, name)     \
  BUILTIN_3ARG(short, short, short, short, name)     \
  BUILTIN_3ARG(ushort, ushort, ushort, ushort, name) \
  BUILTIN_3ARG(int, int, int, int, name)             \
  BUILTIN_3ARG(uint, uint, uint, uint, name)         \
  BUILTIN_3ARG(long, long, long, long, name)         \
  BUILTIN_3ARG(ulong, ulong, ulong, ulong, name);

#define BUILTIN_1ARG_FLOATS(name)  \
  BUILTIN_1ARG(float, float, name) \
  BUILTIN_1ARG(double, double, name);
#define BUILTIN_2ARG_FLOATS(name)         \
  BUILTIN_2ARG(float, float, float, name) \
  BUILTIN_2ARG(double, double, double, name);
#define BUILTIN_3ARG_FLOATS(name)                \
  BUILTIN_3ARG(float, float, float, float, name) \
  BUILTIN_3ARG(double, double, double, double, name);


///////////////////////////////////////
// Async Copy and Prefetch Functions //
///////////////////////////////////////

#define ASYNC_COPY_TYPE(type)                                                                                       \
  event_t __OVERLOAD__ async_work_group_copy(__local type*, const __global type*, size_t, event_t);                 \
  event_t __OVERLOAD__ async_work_group_copy(__global type*, const __local type*, size_t, event_t);                 \
  event_t __OVERLOAD__ async_work_group_strided_copy(__local type*, const __global type*, size_t, size_t, event_t); \
  event_t __OVERLOAD__ async_work_group_strided_copy(__global type*, const __local type*, size_t, size_t, event_t);
#define ASYNC_COPY(type)   \
  ASYNC_COPY_TYPE(type)    \
  ASYNC_COPY_TYPE(type##2) \
  ASYNC_COPY_TYPE(type##3) \
  ASYNC_COPY_TYPE(type##4) \
  ASYNC_COPY_TYPE(type##8) \
  ASYNC_COPY_TYPE(type##16);
ASYNC_COPY(char);
ASYNC_COPY(uchar);
ASYNC_COPY(short);
ASYNC_COPY(ushort);
ASYNC_COPY(int);
ASYNC_COPY(uint);
ASYNC_COPY(long);
ASYNC_COPY(ulong);
ASYNC_COPY(float);
ASYNC_COPY(double);

void wait_group_events(int, event_t*);

#define PREFETCH(type)                                         \
  void __OVERLOAD__ prefetch(const __global type*, size_t);    \
  void __OVERLOAD__ prefetch(const __global type##2*, size_t); \
  void __OVERLOAD__ prefetch(const __global type##3*, size_t); \
  void __OVERLOAD__ prefetch(const __global type##4*, size_t); \
  void __OVERLOAD__ prefetch(const __global type##8*, size_t); \
  void __OVERLOAD__ prefetch(const __global type##16*, size_t);
PREFETCH(char);
PREFETCH(uchar);
PREFETCH(short);
PREFETCH(ushort);
PREFETCH(int);
PREFETCH(uint);
PREFETCH(long);
PREFETCH(ulong);
PREFETCH(float);
PREFETCH(double);


//////////////////////
// Atomic Functions //
//////////////////////

#define ATOMIC_0ARG_DEF(name, type)                  \
  type __OVERLOAD__ name(volatile __global type *p); \
  type __OVERLOAD__ name(volatile __local type *p);
#define ATOMIC_0ARG(name)               \
  ATOMIC_0ARG_DEF(atom_##name, int);    \
  ATOMIC_0ARG_DEF(atom_##name, uint);   \
  ATOMIC_0ARG_DEF(atomic_##name, int);  \
  ATOMIC_0ARG_DEF(atomic_##name, uint);

#define ATOMIC_1ARG_DEF(name, type)                            \
  type __OVERLOAD__ name(volatile __global type *p, type val); \
  type __OVERLOAD__ name(volatile __local type *p, type val);
#define ATOMIC_1ARG(name)               \
  ATOMIC_1ARG_DEF(atom_##name, int);    \
  ATOMIC_1ARG_DEF(atom_##name, uint);   \
  ATOMIC_1ARG_DEF(atomic_##name, int);  \
  ATOMIC_1ARG_DEF(atomic_##name, uint);

ATOMIC_1ARG(add);
ATOMIC_1ARG(and);
ATOMIC_0ARG(dec);
ATOMIC_0ARG(inc);
ATOMIC_1ARG(max);
ATOMIC_1ARG(min);
ATOMIC_1ARG(or);
ATOMIC_1ARG(sub);
ATOMIC_1ARG(xchg);
ATOMIC_1ARG_DEF(atom_xchg, float);
ATOMIC_1ARG_DEF(atomic_xchg, float);
ATOMIC_1ARG(xor);

int __OVERLOAD__ atom_cmpxchg(volatile __global int *p, int cmp, int val);
int __OVERLOAD__ atom_cmpxchg(volatile __local int *p, int cmp, int val);
uint __OVERLOAD__ atom_cmpxchg(volatile __global uint *p, uint cmp, uint val);
uint __OVERLOAD__ atom_cmpxchg(volatile __local uint *p, uint cmp, uint val);
int __OVERLOAD__ atomic_cmpxchg(volatile __global int *p, int cmp, int val);
int __OVERLOAD__ atomic_cmpxchg(volatile __local int *p, int cmp, int val);
uint __OVERLOAD__ atomic_cmpxchg(volatile __global uint *p, uint cmp, uint val);
uint __OVERLOAD__ atomic_cmpxchg(volatile __local uint *p, uint cmp, uint val);


//////////////////////
// Common Functions //
//////////////////////

#define ABS(type)                 \
  u##type __OVERLOAD__ abs(type); \
  u##type __OVERLOAD__ abs(u##type);
#define ABS_DIFF(type)                       \
  u##type __OVERLOAD__ abs_diff(type, type); \
  u##type __OVERLOAD__ abs_diff(u##type, u##type);
#define ABS_BOTH(type) \
  ABS(type);           \
  ABS_DIFF(type);
#define ABS_ALL(type) \
  ABS_BOTH(type);     \
  ABS_BOTH(type##2);  \
  ABS_BOTH(type##3);  \
  ABS_BOTH(type##4);  \
  ABS_BOTH(type##8);  \
  ABS_BOTH(type##16);

ABS_ALL(char);
ABS_ALL(short);
ABS_ALL(int);
ABS_ALL(long);
BUILTIN_3ARG_FLOATS(clamp);
BUILTIN_1ARG_FLOATS(degrees);
BUILTIN_2ARG_FLOATS(max);
BUILTIN_2ARG_FLOATS(min);
BUILTIN_3ARG_FLOATS(mix);
BUILTIN_1ARG_FLOATS(radians);
BUILTIN_1ARG_FLOATS(sign);
BUILTIN_3ARG_FLOATS(smoothstep);
BUILTIN_2ARG_FLOATS(step);

#define COMMON_SCALAR(type, n)                          \
  type##n __OVERLOAD__ clamp(type##n, type, type);      \
  type##n __OVERLOAD__ max(type##n, type);              \
  type##n __OVERLOAD__ min(type##n, type);              \
  type##n __OVERLOAD__ mix(type##n, type##n, type);     \
  type##n __OVERLOAD__ smoothstep(type, type, type##n); \
  type##n __OVERLOAD__ step(type, type##n);
COMMON_SCALAR(float, 2);
COMMON_SCALAR(float, 3);
COMMON_SCALAR(float, 4);
COMMON_SCALAR(float, 8);
COMMON_SCALAR(float, 16);
COMMON_SCALAR(double, 2);
COMMON_SCALAR(double, 3);
COMMON_SCALAR(double, 4);
COMMON_SCALAR(double, 8);
COMMON_SCALAR(double, 16);


/////////////////////////
// Geometric Functions //
/////////////////////////

#define GEOM_1ARG(type, name)     \
 type __OVERLOAD__ name(type);    \
 type __OVERLOAD__ name(type##2); \
 type __OVERLOAD__ name(type##3); \
 type __OVERLOAD__ name(type##4); \
 type __OVERLOAD__ name(type##8); \
 type __OVERLOAD__ name(type##16);
#define GEOM_2ARG(type, name)              \
 type __OVERLOAD__ name(type, type);       \
 type __OVERLOAD__ name(type##2, type##2); \
 type __OVERLOAD__ name(type##3, type##3); \
 type __OVERLOAD__ name(type##4, type##4); \
 type __OVERLOAD__ name(type##8, type##8); \
 type __OVERLOAD__ name(type##16, type##16);

float4 __OVERLOAD__ cross(float4, float4);
float3 __OVERLOAD__ cross(float3, float3);
double4 __OVERLOAD__ cross(double4, double4);
double3 __OVERLOAD__ cross(double3, double3);
GEOM_2ARG(float, dot);
GEOM_2ARG(double, dot);
GEOM_2ARG(float, distance);
GEOM_2ARG(double, distance);
GEOM_1ARG(float, length);
GEOM_1ARG(double, length);
BUILTIN_1ARG_FLOATS(normalize);
GEOM_2ARG(float, fast_distance);
GEOM_2ARG(double, fast_distance);
GEOM_1ARG(float, fast_length);
GEOM_1ARG(double, fast_length);
BUILTIN_1ARG_FLOATS(fast_normalize);


/////////////////////
// Image Functions //
/////////////////////

#define IMAGE_QUERY(ret, name, type) \
  ret __OVERLOAD__ name(read_only type image); \
  ret __OVERLOAD__ name(write_only type image)

IMAGE_QUERY(size_t, get_image_array_size, image1d_array_t);
IMAGE_QUERY(size_t, get_image_array_size, image2d_array_t);

IMAGE_QUERY(int, get_image_channel_data_type, image1d_t);
IMAGE_QUERY(int, get_image_channel_data_type, image1d_buffer_t);
IMAGE_QUERY(int, get_image_channel_data_type, image1d_array_t);
IMAGE_QUERY(int, get_image_channel_data_type, image2d_t);
IMAGE_QUERY(int, get_image_channel_data_type, image2d_array_t);
IMAGE_QUERY(int, get_image_channel_data_type, image3d_t);

IMAGE_QUERY(int, get_image_channel_order, image1d_t);
IMAGE_QUERY(int, get_image_channel_order, image1d_buffer_t);
IMAGE_QUERY(int, get_image_channel_order, image1d_array_t);
IMAGE_QUERY(int, get_image_channel_order, image2d_t);
IMAGE_QUERY(int, get_image_channel_order, image2d_array_t);
IMAGE_QUERY(int, get_image_channel_order, image3d_t);

IMAGE_QUERY(int2, get_image_dim, image2d_t);
IMAGE_QUERY(int2, get_image_dim, image2d_array_t);
IMAGE_QUERY(int4, get_image_dim, image3d_t);

IMAGE_QUERY(int, get_image_depth, image3d_t);
IMAGE_QUERY(int, get_image_height, image2d_t);
IMAGE_QUERY(int, get_image_height, image2d_array_t);
IMAGE_QUERY(int, get_image_height, image3d_t);
IMAGE_QUERY(int, get_image_width, image1d_t);
IMAGE_QUERY(int, get_image_width, image1d_buffer_t);
IMAGE_QUERY(int, get_image_width, image1d_array_t);
IMAGE_QUERY(int, get_image_width, image2d_t);
IMAGE_QUERY(int, get_image_width, image2d_array_t);
IMAGE_QUERY(int, get_image_width, image3d_t);

float4 __OVERLOAD__ read_imagef(image1d_t, int);
float4 __OVERLOAD__ read_imagef(image1d_buffer_t, int);
float4 __OVERLOAD__ read_imagef(image1d_array_t, int2);
float4 __OVERLOAD__ read_imagef(image2d_t, int2);
float4 __OVERLOAD__ read_imagef(image2d_array_t, int4);
float4 __OVERLOAD__ read_imagef(image3d_t, int4);

float4 __OVERLOAD__ read_imagef(image1d_t, sampler_t, int);
float4 __OVERLOAD__ read_imagef(image1d_t, sampler_t, float);
float4 __OVERLOAD__ read_imagef(image1d_array_t, sampler_t, int2);
float4 __OVERLOAD__ read_imagef(image1d_array_t, sampler_t, float2);
float4 __OVERLOAD__ read_imagef(image2d_t, sampler_t, int2);
float4 __OVERLOAD__ read_imagef(image2d_t, sampler_t, float2);
float4 __OVERLOAD__ read_imagef(image2d_array_t, sampler_t, int4);
float4 __OVERLOAD__ read_imagef(image2d_array_t, sampler_t, float4);
float4 __OVERLOAD__ read_imagef(image3d_t, sampler_t, int4);
float4 __OVERLOAD__ read_imagef(image3d_t, sampler_t, float4);

int4 __OVERLOAD__ read_imagei(image1d_t, int);
int4 __OVERLOAD__ read_imagei(image1d_buffer_t, int);
int4 __OVERLOAD__ read_imagei(image1d_array_t, int2);
int4 __OVERLOAD__ read_imagei(image2d_t, int2);
int4 __OVERLOAD__ read_imagei(image2d_array_t, int4);
int4 __OVERLOAD__ read_imagei(image3d_t, int4);

int4 __OVERLOAD__ read_imagei(image1d_t, sampler_t, int);
int4 __OVERLOAD__ read_imagei(image1d_t, sampler_t, float);
int4 __OVERLOAD__ read_imagei(image1d_array_t, sampler_t, int2);
int4 __OVERLOAD__ read_imagei(image1d_array_t, sampler_t, float2);
int4 __OVERLOAD__ read_imagei(image2d_t, sampler_t, int2);
int4 __OVERLOAD__ read_imagei(image2d_t, sampler_t, float2);
int4 __OVERLOAD__ read_imagei(image2d_array_t, sampler_t, int4);
int4 __OVERLOAD__ read_imagei(image2d_array_t, sampler_t, float4);
int4 __OVERLOAD__ read_imagei(image3d_t, sampler_t, int4);
int4 __OVERLOAD__ read_imagei(image3d_t, sampler_t, float4);

uint4 __OVERLOAD__ read_imageui(image1d_t, int);
uint4 __OVERLOAD__ read_imageui(image1d_buffer_t, int);
uint4 __OVERLOAD__ read_imageui(image1d_array_t, int2);
uint4 __OVERLOAD__ read_imageui(image2d_t, int2);
uint4 __OVERLOAD__ read_imageui(image2d_array_t, int4);
uint4 __OVERLOAD__ read_imageui(image3d_t, int4);

uint4 __OVERLOAD__ read_imageui(image1d_t, sampler_t, int);
uint4 __OVERLOAD__ read_imageui(image1d_t, sampler_t, float);
uint4 __OVERLOAD__ read_imageui(image1d_array_t, sampler_t, int2);
uint4 __OVERLOAD__ read_imageui(image1d_array_t, sampler_t, float2);
uint4 __OVERLOAD__ read_imageui(image2d_t, sampler_t, int2);
uint4 __OVERLOAD__ read_imageui(image2d_t, sampler_t, float2);
uint4 __OVERLOAD__ read_imageui(image2d_array_t, sampler_t, int4);
uint4 __OVERLOAD__ read_imageui(image2d_array_t, sampler_t, float4);
uint4 __OVERLOAD__ read_imageui(image3d_t, sampler_t, int4);
uint4 __OVERLOAD__ read_imageui(image3d_t, sampler_t, float4);

void __OVERLOAD__ write_imagef(write_only image1d_t, int, float4);
void __OVERLOAD__ write_imagef(write_only image1d_array_t, int2, float4);
void __OVERLOAD__ write_imagef(write_only image2d_t, int2, float4);
void __OVERLOAD__ write_imagef(write_only image2d_array_t, int4, float4);
void __OVERLOAD__ write_imagef(write_only image3d_t, int4, float4);
void __OVERLOAD__ write_imagei(write_only image1d_t, int, int4);
void __OVERLOAD__ write_imagei(write_only image1d_array_t, int2, int4);
void __OVERLOAD__ write_imagei(write_only image2d_t, int2, int4);
void __OVERLOAD__ write_imagei(write_only image2d_array_t, int4, int4);
void __OVERLOAD__ write_imagei(write_only image3d_t, int4, int4);
void __OVERLOAD__ write_imageui(write_only image1d_t, int, uint4);
void __OVERLOAD__ write_imageui(write_only image1d_array_t, int2, uint4);
void __OVERLOAD__ write_imageui(write_only image2d_t, int2, uint4);
void __OVERLOAD__ write_imageui(write_only image2d_array_t, int4, uint4);
void __OVERLOAD__ write_imageui(write_only image3d_t, int4, uint4);


///////////////////////
// Integer Functions //
///////////////////////

BUILTIN_2ARG_INTEGERS(add_sat);
BUILTIN_3ARG_INTEGERS(clamp);
BUILTIN_1ARG_INTEGERS(clz);
BUILTIN_2ARG_INTEGERS(hadd);
BUILTIN_3ARG(int, int, int, int, mad24);
BUILTIN_3ARG(uint, uint, uint, uint, mad24);
BUILTIN_3ARG_INTEGERS(mad_hi);
BUILTIN_3ARG_INTEGERS(mad_sat);
BUILTIN_2ARG_INTEGERS(max);
BUILTIN_2ARG_INTEGERS(min);
BUILTIN_2ARG(int, int, int, mul24);
BUILTIN_2ARG(uint, uint, uint, mul24);
BUILTIN_2ARG_INTEGERS(mul_hi);
BUILTIN_1ARG_INTEGERS(popcount);
BUILTIN_2ARG_INTEGERS(rhadd);
BUILTIN_2ARG_INTEGERS(rotate);
BUILTIN_2ARG_INTEGERS(sub_sat);
#define UPSAMPLE_SIZES(out, in1, in2)            \
  out     __OVERLOAD__ upsample(in1, in2);       \
  out##2  __OVERLOAD__ upsample(in1##2, in2##2); \
  out##3  __OVERLOAD__ upsample(in1##3, in2##3); \
  out##4  __OVERLOAD__ upsample(in1##4, in2##4); \
  out##8  __OVERLOAD__ upsample(in1##8, in2##8); \
  out##16 __OVERLOAD__ upsample(in1##16, in2##16);
#define UPSAMPLE(out, in)      \
  UPSAMPLE_SIZES(out, in, u##in); \
  UPSAMPLE_SIZES(u##out, u##in, u##in);
UPSAMPLE(short, char);
UPSAMPLE(int, short);
UPSAMPLE(long, int);


////////////////////
// Math Functions //
////////////////////

#define BUILTIN_2TYPE_PTR(type1, type2, name)     \
 type1 __OVERLOAD__ name(type1, __global type2*); \
 type1 __OVERLOAD__ name(type1, __local type2*);  \
 type1 __OVERLOAD__ name(type1, __private type2*);
#define BUILTIN_PTR_ARG(type1, type2, name)  \
 BUILTIN_2TYPE_PTR(type1, type2, name)       \
 BUILTIN_2TYPE_PTR(type1##2, type2##2, name) \
 BUILTIN_2TYPE_PTR(type1##3, type2##3, name) \
 BUILTIN_2TYPE_PTR(type1##4, type2##4, name) \
 BUILTIN_2TYPE_PTR(type1##8, type2##8, name) \
 BUILTIN_2TYPE_PTR(type1##16, type2##16, name);
#define REMQUO(type, addrspace)                                   \
  type __OVERLOAD__ remquo(type, type, addrspace int*);           \
  type##2 __OVERLOAD__ remquo(type##2, type##2, addrspace int2*); \
  type##3 __OVERLOAD__ remquo(type##3, type##3, addrspace int3*); \
  type##4 __OVERLOAD__ remquo(type##4, type##4, addrspace int4*); \
  type##8 __OVERLOAD__ remquo(type##8, type##8, addrspace int8*); \
  type##16 __OVERLOAD__ remquo(type##16, type##16, addrspace int16*);

BUILTIN_1ARG_FLOATS(acos);
BUILTIN_1ARG_FLOATS(acosh);
BUILTIN_1ARG_FLOATS(acospi);
BUILTIN_1ARG_FLOATS(asin);
BUILTIN_1ARG_FLOATS(asinh);
BUILTIN_1ARG_FLOATS(asinpi);
BUILTIN_1ARG_FLOATS(atan);
BUILTIN_2ARG_FLOATS(atan2);
BUILTIN_1ARG_FLOATS(atanh);
BUILTIN_1ARG_FLOATS(atanpi);
BUILTIN_2ARG_FLOATS(atan2pi);
BUILTIN_1ARG_FLOATS(cbrt);
BUILTIN_1ARG_FLOATS(ceil);
BUILTIN_2ARG_FLOATS(copysign);
BUILTIN_1ARG_FLOATS(cos);
BUILTIN_1ARG_FLOATS(cosh);
BUILTIN_1ARG_FLOATS(cospi);
BUILTIN_1ARG_FLOATS(erfc);
BUILTIN_1ARG_FLOATS(erf);
BUILTIN_1ARG_FLOATS(exp);
BUILTIN_1ARG_FLOATS(exp2);
BUILTIN_1ARG_FLOATS(exp10);
BUILTIN_1ARG_FLOATS(expm1);
BUILTIN_1ARG_FLOATS(fabs);
BUILTIN_2ARG_FLOATS(fdim);
BUILTIN_1ARG_FLOATS(floor);
BUILTIN_3ARG_FLOATS(fma);
BUILTIN_2ARG_FLOATS(fmax);
BUILTIN_2ARG_FLOATS(fmin);
BUILTIN_2ARG_FLOATS(fmod);
BUILTIN_PTR_ARG(float, float, fract);
BUILTIN_PTR_ARG(double, double, fract);
BUILTIN_PTR_ARG(float, int, frexp);
BUILTIN_PTR_ARG(double, int, frexp);
BUILTIN_2ARG_FLOATS(hypot);
BUILTIN_1ARG(int, float, ilogb);
BUILTIN_1ARG(int, double, ilogb);
BUILTIN_2ARG(float, float, int, ldexp);
BUILTIN_2ARG(double, double, int, ldexp);
BUILTIN_1ARG_FLOATS(lgamma);
BUILTIN_PTR_ARG(float, int, lgamma_r);
BUILTIN_PTR_ARG(double, int, lgamma_r);
BUILTIN_1ARG_FLOATS(log);
BUILTIN_1ARG_FLOATS(log2);
BUILTIN_1ARG_FLOATS(log10);
BUILTIN_1ARG_FLOATS(log1p);
BUILTIN_1ARG_FLOATS(logb);
BUILTIN_3ARG_FLOATS(mad);
BUILTIN_2ARG_FLOATS(maxmag);
BUILTIN_2ARG_FLOATS(minmag);
BUILTIN_PTR_ARG(float, float, modf);
BUILTIN_PTR_ARG(double, double, modf);
BUILTIN_1ARG(float, uint, nan);
BUILTIN_1ARG(double, ulong, nan);
BUILTIN_2ARG_FLOATS(nextafter);
BUILTIN_2ARG_FLOATS(pow);
BUILTIN_2ARG(float, float, int, pown);
BUILTIN_2ARG(double, double, int, pown);
BUILTIN_2ARG_FLOATS(powr);
BUILTIN_2ARG_FLOATS(remainder);
REMQUO(float, global);
REMQUO(float, local);
REMQUO(float, private);
REMQUO(double, global);
REMQUO(double, local);
REMQUO(double, private);
BUILTIN_1ARG_FLOATS(rint);
BUILTIN_2ARG(float, float, int, rootn);
BUILTIN_2ARG(double, double, int, rootn);
BUILTIN_1ARG_FLOATS(round);
BUILTIN_1ARG_FLOATS(rsqrt);
BUILTIN_1ARG_FLOATS(sin);
BUILTIN_1ARG_FLOATS(sinpi);
BUILTIN_1ARG_FLOATS(sinh);
BUILTIN_PTR_ARG(float, float, sincos);
BUILTIN_PTR_ARG(double, double, sincos);
BUILTIN_1ARG_FLOATS(sqrt);
BUILTIN_1ARG_FLOATS(tan);
BUILTIN_1ARG_FLOATS(tanh);
BUILTIN_1ARG_FLOATS(tanpi);
BUILTIN_1ARG_FLOATS(tgamma);
BUILTIN_1ARG_FLOATS(trunc);

// Native math functions
BUILTIN_1ARG_FLOATS(half_cos);
BUILTIN_1ARG_FLOATS(native_cos);
BUILTIN_2ARG_FLOATS(half_divide);
BUILTIN_2ARG_FLOATS(native_divide);
BUILTIN_1ARG_FLOATS(half_exp);
BUILTIN_1ARG_FLOATS(native_exp);
BUILTIN_1ARG_FLOATS(half_exp2);
BUILTIN_1ARG_FLOATS(native_exp2);
BUILTIN_1ARG_FLOATS(half_exp10);
BUILTIN_1ARG_FLOATS(native_exp10);
BUILTIN_1ARG_FLOATS(half_log);
BUILTIN_1ARG_FLOATS(native_log);
BUILTIN_1ARG_FLOATS(half_log2);
BUILTIN_1ARG_FLOATS(native_log2);
BUILTIN_1ARG_FLOATS(half_log10);
BUILTIN_1ARG_FLOATS(native_log10);
BUILTIN_2ARG_FLOATS(half_powr);
BUILTIN_2ARG_FLOATS(native_powr);
BUILTIN_1ARG_FLOATS(half_recip);
BUILTIN_1ARG_FLOATS(native_recip);
BUILTIN_1ARG_FLOATS(half_rsqrt);
BUILTIN_1ARG_FLOATS(native_rsqrt);
BUILTIN_1ARG_FLOATS(half_sin);
BUILTIN_1ARG_FLOATS(native_sin);
BUILTIN_1ARG_FLOATS(half_sqrt);
BUILTIN_1ARG_FLOATS(native_sqrt);
BUILTIN_1ARG_FLOATS(half_tan);
BUILTIN_1ARG_FLOATS(native_tan);



////////////////////////////
// Misc. Vector Functions //
////////////////////////////

#define SHUFFLE_TYPE(ret, type, mask)         \
  ret __OVERLOAD__ shuffle(type, mask);       \
  ret##2 __OVERLOAD__ shuffle(type, mask##2); \
  ret##3 __OVERLOAD__ shuffle(type, mask##3); \
  ret##4 __OVERLOAD__ shuffle(type, mask##4); \
  ret##8 __OVERLOAD__ shuffle(type, mask##8); \
  ret##16 __OVERLOAD__ shuffle(type, mask##16);
#define SHUFFLE(type, mask)          \
  SHUFFLE_TYPE(type, type, mask);    \
  SHUFFLE_TYPE(type, type##2, mask); \
  SHUFFLE_TYPE(type, type##3, mask); \
  SHUFFLE_TYPE(type, type##4, mask); \
  SHUFFLE_TYPE(type, type##8, mask); \
  SHUFFLE_TYPE(type, type##16, mask);
SHUFFLE(char, uchar);
SHUFFLE(uchar, uchar);
SHUFFLE(short, ushort);
SHUFFLE(ushort, ushort);
SHUFFLE(int, uint);
SHUFFLE(uint, uint);
SHUFFLE(long, ulong);
SHUFFLE(ulong, ulong);
SHUFFLE(float, uint);
SHUFFLE(double, ulong);

#define SHUFFLE2_TYPE(ret, type, mask)               \
  ret __OVERLOAD__ shuffle2(type, type, mask);       \
  ret##2 __OVERLOAD__ shuffle2(type, type, mask##2); \
  ret##3 __OVERLOAD__ shuffle2(type, type, mask##3); \
  ret##4 __OVERLOAD__ shuffle2(type, type, mask##4); \
  ret##8 __OVERLOAD__ shuffle2(type, type, mask##8); \
  ret##16 __OVERLOAD__ shuffle2(type, type, mask##16);
#define SHUFFLE2(type, mask)          \
  SHUFFLE2_TYPE(type, type, mask);    \
  SHUFFLE2_TYPE(type, type##2, mask); \
  SHUFFLE2_TYPE(type, type##3, mask); \
  SHUFFLE2_TYPE(type, type##4, mask); \
  SHUFFLE2_TYPE(type, type##8, mask); \
  SHUFFLE2_TYPE(type, type##16, mask);
SHUFFLE2(char, uchar);
SHUFFLE2(uchar, uchar);
SHUFFLE2(short, ushort);
SHUFFLE2(ushort, ushort);
SHUFFLE2(int, uint);
SHUFFLE2(uint, uint);
SHUFFLE2(long, ulong);
SHUFFLE2(ulong, ulong);
SHUFFLE2(float, uint);
SHUFFLE2(double, ulong);


//////////////////////////
// Relational Functions //
//////////////////////////

#define BUILTIN_ANYALL(name, type) \
  int __OVERLOAD__ name(type);     \
  int __OVERLOAD__ name(type##2);  \
  int __OVERLOAD__ name(type##3);  \
  int __OVERLOAD__ name(type##4);  \
  int __OVERLOAD__ name(type##8);  \
  int __OVERLOAD__ name(type##16);
#define REL_1ARG(name)            \
  BUILTIN_1ARG(int, float, name); \
  BUILTIN_1ARG(long, double, name);
#define REL_2ARG(name)                   \
  BUILTIN_2ARG(int, float, float, name); \
  BUILTIN_2ARG(long, double, double, name);
BUILTIN_ANYALL(all, char);
BUILTIN_ANYALL(all, short);
BUILTIN_ANYALL(all, int);
BUILTIN_ANYALL(all, long);
BUILTIN_ANYALL(any, char);
BUILTIN_ANYALL(any, short);
BUILTIN_ANYALL(any, int);
BUILTIN_ANYALL(any, long);
BUILTIN_3ARG_FLOATS(bitselect);
BUILTIN_3ARG_INTEGERS(bitselect);
REL_2ARG(isequal);
REL_2ARG(isnotequal);
REL_2ARG(isgreater);
REL_2ARG(isgreaterequal);
REL_2ARG(isless);
REL_2ARG(islessequal);
REL_2ARG(islessgreater);
REL_1ARG(isfinite);
REL_1ARG(isinf);
REL_1ARG(isnan);
REL_1ARG(isnormal);
REL_2ARG(isordered);
REL_2ARG(isunordered);
REL_1ARG(signbit);

#define SELECT_TYPE(type, ctype)               \
  type __OVERLOAD__ select(type, type, ctype); \
  type __OVERLOAD__ select(type, type, u##ctype);
#define SELECT(type, ctype)      \
  SELECT_TYPE(type, ctype)       \
  SELECT_TYPE(type##2, ctype##2) \
  SELECT_TYPE(type##3, ctype##3) \
  SELECT_TYPE(type##4, ctype##4) \
  SELECT_TYPE(type##8, ctype##8) \
  SELECT_TYPE(type##16, ctype##16);
SELECT(char, char);
SELECT(uchar, char);
SELECT(short, short);
SELECT(ushort, short);
SELECT(int, int);
SELECT(uint, int);
SELECT(long, long);
SELECT(ulong, long);
SELECT(float, int);
SELECT(double, long);


///////////////////////////////
// Synchronization Functions //
///////////////////////////////

typedef uint cl_mem_fence_flags;
#define CLK_LOCAL_MEM_FENCE  (1<<0)
#define CLK_GLOBAL_MEM_FENCE (1<<1)

void barrier(cl_mem_fence_flags);
void mem_fence(cl_mem_fence_flags);
void read_mem_fence(cl_mem_fence_flags);
void write_mem_fence(cl_mem_fence_flags);


//////////////////////////////////////////
// Vector Data Load and Store Functions //
//////////////////////////////////////////

#define VLOAD_ADDRSPACE(type, width)                                    \
  type##width __OVERLOAD__ vload##width(size_t, const __private type*); \
  type##width __OVERLOAD__ vload##width(size_t, const __local type*);   \
  type##width __OVERLOAD__ vload##width(size_t, const __global type*);  \
  type##width __OVERLOAD__ vload##width(size_t, const __constant type*);

#define VSTORE_ADDRSPACE(type, width)                                   \
  void __OVERLOAD__ vstore##width(type##width, size_t, __local type*);  \
  void __OVERLOAD__ vstore##width(type##width, size_t, __global type*); \
  void __OVERLOAD__ vstore##width(type##width, size_t, __private type*);

#define V_ADDRSPACE(macro, type) \
  macro(type, 2)                 \
  macro(type, 3)                 \
  macro(type, 4)                 \
  macro(type, 8)                 \
  macro(type, 16);

#define VLOADSTORE(type)              \
  V_ADDRSPACE(VLOAD_ADDRSPACE, type); \
  V_ADDRSPACE(VSTORE_ADDRSPACE, type);

VLOADSTORE(char);
VLOADSTORE(uchar);
VLOADSTORE(short);
VLOADSTORE(ushort);
VLOADSTORE(int);
VLOADSTORE(uint);
VLOADSTORE(long);
VLOADSTORE(ulong);
VLOADSTORE(float);
VLOADSTORE(double);

#define VLOAD_HALF_WIDTH(n)                                            \
  float##n __OVERLOAD__ vload_half##n(size_t, const __private half*);  \
  float##n __OVERLOAD__ vloada_half##n(size_t, const __private half*); \
  float##n __OVERLOAD__ vload_half##n(size_t, const __local half*);    \
  float##n __OVERLOAD__ vloada_half##n(size_t, const __local half*);   \
  float##n __OVERLOAD__ vload_half##n(size_t, const __global half*);   \
  float##n __OVERLOAD__ vloada_half##n(size_t, const __global half*);  \
  float##n __OVERLOAD__ vload_half##n(size_t, const __constant half*); \
  float##n __OVERLOAD__ vloada_half##n(size_t, const __constant half*);
#define VSTORE_HALF_ADDRSPACE(func, type)                      \
  void __OVERLOAD__ func(type, size_t, const __private half*); \
  void __OVERLOAD__ func(type, size_t, const __local half*);   \
  void __OVERLOAD__ func(type, size_t, const __global half*);  \
  void __OVERLOAD__ func(type, size_t, const __constant half*);
#define VSTORE_HALF_ROUND(func, type)      \
  VSTORE_HALF_ADDRSPACE(func, type);       \
  VSTORE_HALF_ADDRSPACE(func##_rte, type); \
  VSTORE_HALF_ADDRSPACE(func##_rtz, type); \
  VSTORE_HALF_ADDRSPACE(func##_rtp, type); \
  VSTORE_HALF_ADDRSPACE(func##_rtn, type);
#define VSTORE_HALF_WIDTH(n)                    \
  VSTORE_HALF_ROUND(vstore_half##n, float##n);  \
  VSTORE_HALF_ROUND(vstorea_half##n, float##n); \
  VSTORE_HALF_ROUND(vstore_half##n, double##n); \
  VSTORE_HALF_ROUND(vstorea_half##n, double##n);
#define VLOADSTORE_HALF_WIDTH(n) \
  VLOAD_HALF_WIDTH(n);           \
  VSTORE_HALF_WIDTH(n);
VLOADSTORE_HALF_WIDTH();
VLOADSTORE_HALF_WIDTH(2);
VLOADSTORE_HALF_WIDTH(3);
VLOADSTORE_HALF_WIDTH(4);
VLOADSTORE_HALF_WIDTH(8);
VLOADSTORE_HALF_WIDTH(16);


/////////////////////////
// Work-Item Functions //
/////////////////////////

size_t get_global_id(uint dim);
size_t get_global_size(uint dim);
size_t get_global_offset(uint dim);
size_t get_group_id(uint dim);
size_t get_local_id(uint dim);
size_t get_local_size(uint dim);
size_t get_num_groups(uint dim);
uint get_work_dim(void);



/////////////////////
// Other Functions //
/////////////////////

int printf(__constant char * restrict, ...);


/////////////////
// Conversions //
/////////////////

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
#define as_double( _x )  __builtin_astype( _x, double )
#define as_double2( _x )  __builtin_astype( _x, double2 )
#define as_double3( _x )  __builtin_astype( _x, double3 )
#define as_double4( _x )  __builtin_astype( _x, double4 )
#define as_double8( _x )  __builtin_astype( _x, double8 )
#define as_double16( _x )  __builtin_astype( _x, double16 )
#define as_size_t( _x ) __builtin_astype( _x, size_t )
#define as_ptrdiff_t( _x ) __builtin_astype( _x, ptrdiff_t )
#define as_uintptr_t( _x ) __builtin_astype( _x, uintptr_t )
#define as_intptr_t( _x ) __builtin_astype( _x, intptr_t )

#define CONVERT_TYPE_SIZE(out, in)              \
  out __OVERLOAD__ convert_##out(in);           \
  out __OVERLOAD__ convert_##out##_rte(in);     \
  out __OVERLOAD__ convert_##out##_rtz(in);     \
  out __OVERLOAD__ convert_##out##_rtp(in);     \
  out __OVERLOAD__ convert_##out##_rtn(in);     \
  out __OVERLOAD__ convert_##out##_sat(in);     \
  out __OVERLOAD__ convert_##out##_sat_rte(in); \
  out __OVERLOAD__ convert_##out##_sat_rtz(in); \
  out __OVERLOAD__ convert_##out##_sat_rtp(in); \
  out __OVERLOAD__ convert_##out##_sat_rtn(in);
#define CONVERT_TYPE(out, in)             \
  CONVERT_TYPE_SIZE(out, in);             \
  CONVERT_TYPE_SIZE(out##2, in##2);       \
  CONVERT_TYPE_SIZE(out##3, in##3);       \
  CONVERT_TYPE_SIZE(out##4, in##4);       \
  CONVERT_TYPE_SIZE(out##8, in##8);       \
  CONVERT_TYPE_SIZE(out##16, in##16);
#define CONVERT(out)         \
  CONVERT_TYPE(out, char);   \
  CONVERT_TYPE(out, uchar);  \
  CONVERT_TYPE(out, short);  \
  CONVERT_TYPE(out, ushort); \
  CONVERT_TYPE(out, int);    \
  CONVERT_TYPE(out, uint);   \
  CONVERT_TYPE(out, long);   \
  CONVERT_TYPE(out, ulong);  \
  CONVERT_TYPE(out, float);  \
  CONVERT_TYPE(out, double);

CONVERT(char);
CONVERT(uchar);
CONVERT(short);
CONVERT(ushort);
CONVERT(int);
CONVERT(uint);
CONVERT(long);
CONVERT(ulong);
CONVERT(float);
CONVERT(double);
