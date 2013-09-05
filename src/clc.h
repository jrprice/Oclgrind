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

#define CLK_LOCAL_MEM_FENCE 1<<0
#define CLK_GLOBAL_MEM_FENCE 1<<1

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

#define BUILTIN_1ARG(type, name)        \
	type __OVERLOAD__ name(type a);       \
	type##2 __OVERLOAD__ name(type##2 a); \
	type##3 __OVERLOAD__ name(type##3 a); \
	type##4 __OVERLOAD__ name(type##4 a); \
	type##8 __OVERLOAD__ name(type##8 a); \
	type##16 __OVERLOAD__ name(type##16 a);
#define BUILTIN_2ARG(type, name)                   \
	type __OVERLOAD__ name(type a, type b);          \
	type##2 __OVERLOAD__ name(type##2 a, type##2 b); \
	type##3 __OVERLOAD__ name(type##3 a, type##3 b); \
	type##4 __OVERLOAD__ name(type##4 a, type##4 b); \
	type##8 __OVERLOAD__ name(type##8 a, type##8 b); \
	type##16 __OVERLOAD__ name(type##16 a, type##16 b);
#define BUILTIN_3ARG(type, name)                              \
	type __OVERLOAD__ name(type a, type b, type c);             \
	type##2 __OVERLOAD__ name(type##2 a, type##2 b, type##2 c); \
	type##3 __OVERLOAD__ name(type##3 a, type##3 b, type##3 c); \
	type##4 __OVERLOAD__ name(type##4 a, type##4 b, type##4 c); \
	type##8 __OVERLOAD__ name(type##8 a, type##8 b, type##8 c); \
	type##16 __OVERLOAD__ name(type##16 a, type##16 b, type##16 c);

#define BUILTIN_1ARG_INTEGERS(name) \
	BUILTIN_1ARG(char, name)          \
	BUILTIN_1ARG(uchar, name)         \
	BUILTIN_1ARG(short, name)         \
	BUILTIN_1ARG(ushort, name)        \
	BUILTIN_1ARG(int, name)           \
	BUILTIN_1ARG(uint, name)          \
	BUILTIN_1ARG(long, name)          \
	BUILTIN_1ARG(ulong, name);
#define BUILTIN_2ARG_INTEGERS(name) \
	BUILTIN_2ARG(char, name)          \
	BUILTIN_2ARG(uchar, name)         \
	BUILTIN_2ARG(short, name)         \
	BUILTIN_2ARG(ushort, name)        \
	BUILTIN_2ARG(int, name)           \
	BUILTIN_2ARG(uint, name)          \
	BUILTIN_2ARG(long, name)          \
	BUILTIN_2ARG(ulong, name);
#define BUILTIN_3ARG_INTEGERS(name) \
	BUILTIN_3ARG(char, name)          \
	BUILTIN_3ARG(uchar, name)         \
	BUILTIN_3ARG(short, name)         \
	BUILTIN_3ARG(ushort, name)        \
	BUILTIN_3ARG(int, name)           \
	BUILTIN_3ARG(uint, name)          \
	BUILTIN_3ARG(long, name)          \
	BUILTIN_3ARG(ulong, name);


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

BUILTIN_3ARG(float, clamp);
BUILTIN_2ARG(float, max);
BUILTIN_2ARG(float, min);



/////////////////////////
// Geometric Functions //
/////////////////////////

float __OVERLOAD__ dot(float a, float b);
float __OVERLOAD__ dot(float2 a, float2 b);
float __OVERLOAD__ dot(float3 a, float3 b);
float __OVERLOAD__ dot(float4 a, float4 b);
float __OVERLOAD__ dot(float8 a, float8 b);
float __OVERLOAD__ dot(float16 a, float16 b);


///////////////////////
// Integer Functions //
///////////////////////

BUILTIN_2ARG_INTEGERS(hadd);
BUILTIN_3ARG_INTEGERS(clamp);
BUILTIN_2ARG_INTEGERS(max);
BUILTIN_2ARG_INTEGERS(min);


////////////////////
// Math Functions //
////////////////////

BUILTIN_1ARG(float, cos);
BUILTIN_1ARG(float, fabs);
BUILTIN_1ARG(float, exp);
#define native_divide(a, b) (a/b)
#define native_exp(x) exp(x)
#define native_powr(a, b) pow(a, b)
#define native_sqrt(x) sqrt(x)
#define nextafter( _x, _y ) __builtin_nextafter( _x, _y )
BUILTIN_2ARG(float, pow);
BUILTIN_1ARG(float, sin);
float __OVERLOAD__ sincos(float x, float *cosval);
float2 __OVERLOAD__ sincos(float2 x, float2 *cosval);
float3 __OVERLOAD__ sincos(float3 x, float3 *cosval);
float4 __OVERLOAD__ sincos(float4 x, float4 *cosval);
float8 __OVERLOAD__ sincos(float8 x, float8 *cosval);
float16 __OVERLOAD__ sincos(float16 x, float16 *cosval);
BUILTIN_1ARG(float, sqrt);


//////////////////////////
// Relational Functions //
//////////////////////////

#define isinf( _x ) __builtin_isinf( _x )
#define isnan( _x ) __builtin_isnan( _x )
#define isnormal( _x ) __builtin_isnormal( _x )


///////////////////////////////
// Synchronization Functions //
///////////////////////////////

void barrier(uint);


//////////////////////////////////////////
// Vector Data Load and Store Functions //
//////////////////////////////////////////

#define VLOAD_ADDRSPACE(type, width)                             \
  type##width __OVERLOAD__ vload##width(size_t, type*);          \
  type##width __OVERLOAD__ vload##width(size_t, __local type*);  \
  type##width __OVERLOAD__ vload##width(size_t, __global type*); \
  type##width __OVERLOAD__ vload##width(size_t, __constant type*);

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
