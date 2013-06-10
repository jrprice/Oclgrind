typedef __attribute__((ext_vector_type(2))) float float2;
typedef __attribute__((ext_vector_type(3))) float float3;
typedef __attribute__((ext_vector_type(4))) float float4;

typedef unsigned int uint;
typedef unsigned long size_t;

#define CLK_LOCAL_MEM_FENCE 1<<0
#define CLK_GLOBAL_MEM_FENCE 1<<1
#define FLT_MAX 1E+37


size_t get_global_id(uint dim);
size_t get_global_size(uint dim);
size_t get_group_id(uint dim);
size_t get_local_id(uint dim);
size_t get_local_size(uint dim);

void barrier(uint);
float cos(float x);
float fabs(float x);
int min(int a, int b);
float native_sqrt(float x);
float sin(float x);
