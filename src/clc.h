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

#define TYPEDEF_VECTOR(type)                                \
  typedef __attribute__((ext_vector_type(2))) type type##2; \
  typedef __attribute__((ext_vector_type(3))) type type##3; \
  typedef __attribute__((ext_vector_type(4))) type type##4; \
  typedef __attribute__((ext_vector_type(8))) type type##8; \
  typedef __attribute__((ext_vector_type(16))) type type##16;
TYPEDEF_VECTOR(char);
TYPEDEF_VECTOR(uchar);
TYPEDEF_VECTOR(short)
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
#define __kernel_exec(X, typen) __kernel                        \
  __attribute__((work_group_size_hint(X, 1, 1)))                \
  __attribute__((vec_type_hint(typen)))

#define CHAR_BIT    8
#define	SCHAR_MAX	127		/* min value for a signed char */
#define	SCHAR_MIN	(-128)		/* max value for a signed char */
#define	UCHAR_MAX	255		/* max value for an unsigned char */
#define	CHAR_MAX	SCHAR_MAX		/* max value for a char */
#define	CHAR_MIN	SCHAR_MIN		/* min value for a char */
#define	USHRT_MAX	65535		/* max value for an unsigned short */
#define	SHRT_MAX	32767		/* max value for a short */
#define	SHRT_MIN	(-32768)	/* min value for a short */
#define	UINT_MAX	0xffffffff	/* max value for an unsigned int */
#define	INT_MAX		2147483647	/* max value for an int */
#define	INT_MIN		(-2147483647-1)	/* min value for an int */
#define	ULONG_MAX	0xffffffffffffffffUL	/* max unsigned long */
#define	LONG_MAX	((long)0x7fffffffffffffffL)	/* max signed long */
#define	LONG_MIN	((long)(-0x7fffffffffffffffL-1)) /* min signed long */

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

#define FP_ILOGB0       INT_MIN
#define FP_ILOGBNAN     INT_MIN

#define M_E_F         2.71828182845904523536028747135266250f   /* e */
#define M_LOG2E_F     1.44269504088896340735992468100189214f   /* log 2e */
#define M_LOG10E_F    0.434294481903251827651128918916605082f  /* log 10e */
#define M_LN2_F       0.693147180559945309417232121458176568f  /* log e2 */
#define M_LN10_F      2.3025850929940456840179914546843642f    /* log e10 */
#define M_PI_F        3.14159265358979323846264338327950288f   /* pi */
#define M_PI_2_F      1.57079632679489661923132169163975144f   /* pi/2 */
#define M_PI_4_F      0.785398163397448309615660845819875721f  /* pi/4 */
#define M_1_PI_F      0.318309886183790671537767526745028724f  /* 1/pi */
#define M_2_PI_F      0.636619772367581343075535053490057448f  /* 2/pi */
#define M_2_SQRTPI_F  1.12837916709551257389615890312154517f   /* 2/sqrt(pi) */
#define M_SQRT2_F     1.41421356237309504880168872420969808f   /* sqrt(2) */
#define M_SQRT1_2_F   0.707106781186547524400844362104849039f  /* 1/sqrt(2) */

#define MAXFLOAT ((float)3.40282346638528860e+38)
#define HUGE_VALF __builtin_huge_valf()
#define INFINITY __builtin_inff()
#define NAN __builtin_nanf((const char*)"")

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


///////////////////////////////////////
// Async Copy and Prefetch Functions //
///////////////////////////////////////

#define ASYNC_COPY_TYPE(type) \
	event_t __OVERLOAD__ async_work_group_copy(__local type*, const __global type*, size_t, event_t);  \
	event_t __OVERLOAD__ async_work_group_copy(__global type*, const __local type*, size_t, event_t);  \
	event_t __OVERLOAD__ async_work_group_strided_copy(__local type*, const __global type*, size_t, size_t, event_t);  \
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
BUILTIN_3ARG(float, float, float, float, clamp);
BUILTIN_1ARG(float, float, degrees);
BUILTIN_2ARG(float, float, float, max);
BUILTIN_2ARG(float, float, float, min);
BUILTIN_3ARG(float, float, float, float, mix);
BUILTIN_1ARG(float, float, radians);
BUILTIN_1ARG(float, float, sign);
BUILTIN_3ARG(float, float, float, float, smoothstep);
BUILTIN_2ARG(float, float, float, step);


/////////////////////////
// Geometric Functions //
/////////////////////////

#define GEOM_1ARG(name) \
 float __OVERLOAD__ name(float);  \
 float __OVERLOAD__ name(float2); \
 float __OVERLOAD__ name(float3); \
 float __OVERLOAD__ name(float4); \
 float __OVERLOAD__ name(float8); \
 float __OVERLOAD__ name(float16);
#define GEOM_2ARG(name) \
 float __OVERLOAD__ name(float, float);   \
 float __OVERLOAD__ name(float2, float2); \
 float __OVERLOAD__ name(float3, float3); \
 float __OVERLOAD__ name(float4, float4); \
 float __OVERLOAD__ name(float8, float8); \
 float __OVERLOAD__ name(float16, float16);

float4 __OVERLOAD__ cross(float4, float4);
float3 __OVERLOAD__ cross(float3, float3);
GEOM_2ARG(dot);
GEOM_2ARG(distance);
GEOM_1ARG(length);
BUILTIN_1ARG(float, float, normalize);
GEOM_2ARG(fast_distance);
GEOM_1ARG(fast_length);
BUILTIN_1ARG(float, float, fast_normalize);


///////////////////////
// Integer Functions //
///////////////////////

BUILTIN_2ARG_INTEGERS(add_sat);
BUILTIN_3ARG_INTEGERS(clamp);
BUILTIN_1ARG_INTEGERS(clz);
BUILTIN_2ARG_INTEGERS(hadd);
BUILTIN_3ARG(int, int, int, int, mad24);
BUILTIN_3ARG(uint, uint, uint, uint, mad24);
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


////////////////////
// Math Functions //
////////////////////

#define BUILTIN_2TYPE_PTR(type1, type2, name) \
 type1 __OVERLOAD__ name(type1, __global type2*); \
 type1 __OVERLOAD__ name(type1, __local type2*); \
 type1 __OVERLOAD__ name(type1, __private type2*);
#define BUILTIN_PTR_ARG(type1, type2, name) \
 BUILTIN_2TYPE_PTR(type1, type2, name) \
 BUILTIN_2TYPE_PTR(type1##2, type2##2, name) \
 BUILTIN_2TYPE_PTR(type1##3, type2##3, name) \
 BUILTIN_2TYPE_PTR(type1##4, type2##4, name) \
 BUILTIN_2TYPE_PTR(type1##8, type2##8, name) \
 BUILTIN_2TYPE_PTR(type1##16, type2##16, name);

BUILTIN_1ARG(float, float, acos);
BUILTIN_1ARG(float, float, acosh);
BUILTIN_1ARG(float, float, acospi);
BUILTIN_1ARG(float, float, asin);
BUILTIN_1ARG(float, float, asinh);
BUILTIN_1ARG(float, float, asinpi);
BUILTIN_1ARG(float, float, atan);
BUILTIN_2ARG(float, float, float, atan2);
BUILTIN_1ARG(float, float, atanh);
BUILTIN_1ARG(float, float, atanpi);
BUILTIN_2ARG(float, float, float, atan2pi);
BUILTIN_1ARG(float, float, cbrt);
BUILTIN_1ARG(float, float, ceil);
BUILTIN_2ARG(float, float, float, copysign);
BUILTIN_1ARG(float, float, cos);
BUILTIN_1ARG(float, float, cosh);
BUILTIN_1ARG(float, float, cospi);
BUILTIN_1ARG(float, float, erfc);
BUILTIN_1ARG(float, float, erf);
BUILTIN_1ARG(float, float, exp);
BUILTIN_1ARG(float, float, exp2);
BUILTIN_1ARG(float, float, exp10);
BUILTIN_1ARG(float, float, expm1);
BUILTIN_1ARG(float, float, fabs);
BUILTIN_2ARG(float, float, float, fdim);
BUILTIN_1ARG(float, float, floor);
BUILTIN_3ARG(float, float, float, float, fma);
BUILTIN_2ARG(float, float, float, fmax);
BUILTIN_2ARG(float, float, float, fmin);
BUILTIN_2ARG(float, float, float, fmod);
BUILTIN_PTR_ARG(float, float, fract);
BUILTIN_PTR_ARG(float, int, frexp);
BUILTIN_2ARG(float, float, float, hypot);
BUILTIN_1ARG(int, float, ilogb);
BUILTIN_2ARG(float, float, int, ldexp);
BUILTIN_1ARG(float, float, lgamma);
BUILTIN_PTR_ARG(float, int, lgamma_r);
BUILTIN_1ARG(float, float, log);
BUILTIN_1ARG(float, float, log2);
BUILTIN_1ARG(float, float, log10);
BUILTIN_1ARG(float, float, log1p);
BUILTIN_1ARG(float, float, logb);
BUILTIN_3ARG(float, float, float, float, mad);
BUILTIN_2ARG(float, float, float, maxmag);
BUILTIN_2ARG(float, float, float, minmag);
BUILTIN_PTR_ARG(float, float, modf);
BUILTIN_1ARG(float, uint, nan);
BUILTIN_2ARG(float, float, float, nextafter);
BUILTIN_2ARG(float, float, float, pow);
BUILTIN_2ARG(float, float, int, pown);
BUILTIN_2ARG(float, float, float, powr);
BUILTIN_2ARG(float, float, float, remainder);
float __OVERLOAD__ remquo(float, float, __global int*);
float2 __OVERLOAD__ remquo(float2, float2, __global int2*);
float3 __OVERLOAD__ remquo(float3, float3, __global int3*);
float4 __OVERLOAD__ remquo(float4, float4, __global int4*);
float8 __OVERLOAD__ remquo(float8, float8, __global int8*);
float16 __OVERLOAD__ remquo(float16, float16, __global int16*);
float __OVERLOAD__ remquo(float, float, __local int*);
float2 __OVERLOAD__ remquo(float2, float2, __local int2*);
float3 __OVERLOAD__ remquo(float3, float3, __local int3*);
float4 __OVERLOAD__ remquo(float4, float4, __local int4*);
float8 __OVERLOAD__ remquo(float8, float8, __local int8*);
float16 __OVERLOAD__ remquo(float16, float16, __local int16*);
float __OVERLOAD__ remquo(float, float, __private int*);
float2 __OVERLOAD__ remquo(float2, float2, __private int2*);
float3 __OVERLOAD__ remquo(float3, float3, __private int3*);
float4 __OVERLOAD__ remquo(float4, float4, __private int4*);
float8 __OVERLOAD__ remquo(float8, float8, __private int8*);
float16 __OVERLOAD__ remquo(float16, float16, __private int16*);
BUILTIN_1ARG(float, float, rint);
BUILTIN_2ARG(float, float, int, rootn);
BUILTIN_1ARG(float, float, round);
BUILTIN_1ARG(float, float, rsqrt);
BUILTIN_1ARG(float, float, sin);
BUILTIN_1ARG(float, float, sinpi);
BUILTIN_1ARG(float, float, sinh);
BUILTIN_PTR_ARG(float, float, sincos);
BUILTIN_1ARG(float, float, sqrt);
BUILTIN_1ARG(float, float, tan);
BUILTIN_1ARG(float, float, tanh);
BUILTIN_1ARG(float, float, tanpi);
BUILTIN_1ARG(float, float, tgamma);
BUILTIN_1ARG(float, float, trunc);

// Native math functions
BUILTIN_1ARG(float, float, half_cos);
BUILTIN_1ARG(float, float, native_cos);
BUILTIN_2ARG(float, float, float, half_divide);
BUILTIN_2ARG(float, float, float, native_divide);
BUILTIN_1ARG(float, float, half_exp);
BUILTIN_1ARG(float, float, native_exp);
BUILTIN_1ARG(float, float, half_exp2);
BUILTIN_1ARG(float, float, native_exp2);
BUILTIN_1ARG(float, float, half_exp10);
BUILTIN_1ARG(float, float, native_exp10);
BUILTIN_1ARG(float, float, half_log);
BUILTIN_1ARG(float, float, native_log);
BUILTIN_1ARG(float, float, half_log2);
BUILTIN_1ARG(float, float, native_log2);
BUILTIN_1ARG(float, float, half_log10);
BUILTIN_1ARG(float, float, native_log10);
BUILTIN_2ARG(float, float, float, half_powr);
BUILTIN_2ARG(float, float, float, native_powr);
BUILTIN_1ARG(float, float, half_recip);
BUILTIN_1ARG(float, float, native_recip);
BUILTIN_1ARG(float, float, half_rsqrt);
BUILTIN_1ARG(float, float, native_rsqrt);
BUILTIN_1ARG(float, float, half_sin);
BUILTIN_1ARG(float, float, native_sin);
BUILTIN_1ARG(float, float, half_sqrt);
BUILTIN_1ARG(float, float, native_sqrt);
BUILTIN_1ARG(float, float, half_tan);
BUILTIN_1ARG(float, float, native_tan);



//////////////////////////
// Relational Functions //
//////////////////////////

#define BUILTIN_ANYALL(name, type) \
	int __OVERLOAD__ name(type);  \
	int __OVERLOAD__ name(type##2); \
	int __OVERLOAD__ name(type##3); \
	int __OVERLOAD__ name(type##4); \
	int __OVERLOAD__ name(type##8); \
	int __OVERLOAD__ name(type##16);
BUILTIN_ANYALL(all, char);
BUILTIN_ANYALL(all, short);
BUILTIN_ANYALL(all, int);
BUILTIN_ANYALL(all, long);
BUILTIN_ANYALL(any, char);
BUILTIN_ANYALL(any, short);
BUILTIN_ANYALL(any, int);
BUILTIN_ANYALL(any, long);
BUILTIN_3ARG(float, float, float, float, bitselect);
BUILTIN_3ARG_INTEGERS(bitselect);
BUILTIN_2ARG(int, float, float, isequal);
BUILTIN_2ARG(int, float, float, isnotequal);
BUILTIN_2ARG(int, float, float, isgreater);
BUILTIN_2ARG(int, float, float, isgreaterequal);
BUILTIN_2ARG(int, float, float, isless);
BUILTIN_2ARG(int, float, float, islessequal);
BUILTIN_2ARG(int, float, float, islessgreater);
BUILTIN_1ARG(int, float, isfinite);
BUILTIN_1ARG(int, float, isinf);
BUILTIN_1ARG(int, float, isnan);
BUILTIN_1ARG(int, float, isnormal);
BUILTIN_2ARG(int, float, float, isordered);
BUILTIN_2ARG(int, float, float, isunordered);
BUILTIN_1ARG(int, float, signbit);

#define SELECT_TYPE(type, ctype) \
	type __OVERLOAD__ select(type, type, ctype); \
	type __OVERLOAD__ select(type, type, u##ctype);
#define SELECT(type, ctype) \
	SELECT_TYPE(type, ctype) \
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


///////////////////////////////
// Synchronization Functions //
///////////////////////////////

typedef enum {
  CLK_LOCAL_MEM_FENCE  = 1U << 0,
  CLK_GLOBAL_MEM_FENCE = 1U << 1,
  __unused_except_to_make_sure_the_enum_has_the_right_size = 1U << 31
} cl_mem_fence_flags;

void barrier(cl_mem_fence_flags);
void mem_fence(cl_mem_fence_flags);
void read_mem_fence(cl_mem_fence_flags);
void write_mem_fence(cl_mem_fence_flags);


//////////////////////////////////////////
// Vector Data Load and Store Functions //
//////////////////////////////////////////

#define VLOAD_ADDRSPACE(type, width)                             \
  type##width __OVERLOAD__ vload##width(size_t, const __private type*); \
  type##width __OVERLOAD__ vload##width(size_t, const __local type*);   \
  type##width __OVERLOAD__ vload##width(size_t, const __global type*);  \
  type##width __OVERLOAD__ vload##width(size_t, const __constant type*);

#define VSTORE_ADDRSPACE(type, width)                                   \
  void __OVERLOAD__ vstore##width(type##width, size_t, __local type*);  \
  void __OVERLOAD__ vstore##width(type##width, size_t, __global type*); \
  void __OVERLOAD__ vstore##width(type##width, size_t, __private type*);

#define V_ADDRSPACE(macro, type) \
  macro(type, 2) \
  macro(type, 3) \
  macro(type, 4) \
  macro(type, 8) \
  macro(type, 16);

#define VLOADSTORE(type) \
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
uint get_work_dim();



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
