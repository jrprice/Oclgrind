#include "common.h"
#include <algorithm>

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Type.h"

#include "Memory.h"
#include "WorkGroup.h"
#include "WorkItem.h"

using namespace spirsim;
using namespace std;

#define DEFINE_BUILTIN(name)      void WorkItem::name(const llvm::CallInst *callInst, \
																											std::string fnName,             \
																											std::string overload,           \
																											TypedValue& result)
#define ARG(i) (callInst->getArgOperand(i))
#define UARG(i) getUnsignedInt(callInst->getArgOperand(i))
#define SARG(i) getSignedInt(callInst->getArgOperand(i))
#define FARG(i) getFloatValue(callInst->getArgOperand(i))
#define UARGV(i,v) getUnsignedInt(callInst->getArgOperand(i), v)
#define SARGV(i,v) getSignedInt(callInst->getArgOperand(i), v)
#define FARGV(i,v) getFloatValue(callInst->getArgOperand(i), v)

void WorkItem::builtin_f1arg(const llvm::CallInst *callInst,
                             TypedValue& result,
                             double (*func)(double))
{
  for (int i = 0; i < result.num; i++)
  {
    setFloatResult(result, func(FARGV(0, i)), i);
  }
}

void WorkItem::builtin_f2arg(const llvm::CallInst *callInst,
                             TypedValue& result,
                             double (*func)(double, double))
{
  for (int i = 0; i < result.num; i++)
  {
    setFloatResult(result, func(FARGV(0, i), FARGV(1, i)), i);
  }
}

void WorkItem::builtin_f3arg(const llvm::CallInst *callInst,
                             TypedValue& result,
                             double (*func)(double, double, double))
{
  for (int i = 0; i < result.num; i++)
  {
    setFloatResult(result, func(FARGV(0, i), FARGV(1, i), FARGV(2, i)), i);
  }
}

void WorkItem::builtin_u1arg(const llvm::CallInst *callInst,
                             TypedValue& result,
                             uint64_t (*func)(uint64_t))
{
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, func(UARGV(0, i)), i);
  }
}

void WorkItem::builtin_u2arg(const llvm::CallInst *callInst,
                             TypedValue& result,
                             uint64_t (*func)(uint64_t, uint64_t))
{
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, func(UARGV(0, i), UARGV(1, i)), i);
  }
}

void WorkItem::builtin_u3arg(const llvm::CallInst *callInst,
                             TypedValue& result,
                             uint64_t (*func)(uint64_t, uint64_t, uint64_t))
{
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, func(UARGV(0, i), UARGV(1, i), UARGV(2, i)), i);
  }
}

void WorkItem::builtin_s1arg(const llvm::CallInst *callInst,
                             TypedValue& result,
                             int64_t (*func)(int64_t))
{
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, func(SARGV(0, i)), i);
  }
}

void WorkItem::builtin_s2arg(const llvm::CallInst *callInst,
                             TypedValue& result,
                             int64_t (*func)(int64_t, int64_t))
{
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, func(SARGV(0, i), SARGV(1, i)), i);
  }
}

void WorkItem::builtin_s3arg(const llvm::CallInst *callInst,
                             TypedValue& result,
                             int64_t (*func)(int64_t, int64_t, int64_t))
{
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, func(SARGV(0, i), SARGV(1, i), SARGV(2, i)), i);
  }
}

///////////////////////////////////////
// Async Copy and Prefetch Functions //
///////////////////////////////////////

DEFINE_BUILTIN(async_work_group_copy)
{
  int arg = 0;

  // Get src/dest addresses
  const llvm::Value *destOp = ARG(arg++);
  const llvm::Value *srcOp = ARG(arg++);
  size_t dest = *(size_t*)(m_privateMemory[destOp].data);
  size_t src = *(size_t*)(m_privateMemory[srcOp].data);

  // Get size of copy
  size_t elemSize = getTypeSize(destOp->getType()->getPointerElementType());
  uint64_t num = UARG(arg++);

  // Get stride
  uint64_t stride = 1;
  size_t srcStride = 1;
  size_t destStride = 1;
  if (fnName == "async_work_group_strided_copy")
  {
    stride = UARG(arg++);
  }

  uint64_t event = UARG(arg++);

  // Get type of copy
  WorkGroup::AsyncCopyType type;
  if (destOp->getType()->getPointerAddressSpace() == AddrSpaceLocal)
  {
    type = WorkGroup::GLOBAL_TO_LOCAL;
    srcStride = stride;
  }
  else
  {
    type = WorkGroup::LOCAL_TO_GLOBAL;
    destStride = stride;
  }

  // Register copy
  WorkGroup::AsyncCopy copy = {
    callInst,
    type,
    dest,
    src,
    elemSize,
    num,
    srcStride,
    destStride
  };
  event = m_workGroup.async_copy(copy, event);
  setIntResult(result, event);
}

DEFINE_BUILTIN(wait_group_events)
{
  uint64_t num = UARG(0);
  const llvm::Value *ptrOp = ARG(1);
  size_t address = *(size_t*)(m_privateMemory[ptrOp].data);
  for (int i = 0; i < num; i++)
  {
    // TODO: Can we safely assume this is private/stack data?
    uint64_t event;
    if (!m_stack->load((unsigned char*)&event, address, sizeof(uint64_t)))
    {
      outputMemoryError(*callInst, "Invalid read", AddrSpacePrivate,
                        address, sizeof(uint64_t));
      return;
    }
    m_workGroup.wait_event(event);
    address += sizeof(uint64_t);
  }
  m_state = WAIT_EVENT;
}

DEFINE_BUILTIN(prefetch)
{
  // Do nothing.
}


//////////////////////
// Common Functions //
//////////////////////

template<typename T> T _max(T a, T b){return a > b ? a : b;}
template<typename T> T _min(T a, T b){return a < b ? a : b;}
template<typename T> T _clamp(T x, T min, T max){return _min(x, _max(x, min));}
DEFINE_BUILTIN(clamp)
{
  char type = overload[0];
  if (type == 'D')
  {
    type = overload[4];
  }

  switch (type)
  {
    case 'f':
    case 'd':
      builtin_f3arg(callInst, result, _clamp);
      break;
    case 'h':
    case 't':
    case 'j':
    case 'm':
      builtin_u3arg(callInst, result, _clamp);
      break;
    case 'c':
    case 's':
    case 'i':
    case 'l':
      builtin_s3arg(callInst, result, _clamp);
      break;
    default:
      assert(false);
    }
}

DEFINE_BUILTIN(max)
{
  char type = overload[0];
  if (type == 'D')
  {
    type = overload[4];
  }

  switch (type)
  {
    case 'f':
    case 'd':
      builtin_f2arg(callInst, result, fmax);
      break;
    case 'h':
    case 't':
    case 'j':
    case 'm':
      builtin_u2arg(callInst, result, _max);
      break;
    case 'c':
    case 's':
    case 'i':
    case 'l':
      builtin_s2arg(callInst, result, _max);
      break;
    default:
      assert(false);
    }
}

DEFINE_BUILTIN(min)
{
  char type = overload[0];
  if (type == 'D')
  {
    type = overload[4];
  }

  switch (type)
  {
    case 'f':
    case 'd':
      builtin_f2arg(callInst, result, fmin);
      break;
    case 'h':
    case 't':
    case 'j':
    case 'm':
      builtin_u2arg(callInst, result, _min);
      break;
    case 'c':
    case 's':
    case 'i':
    case 'l':
      builtin_s2arg(callInst, result, _min);
      break;
    default:
      assert(false);
    }
}

/////////////////////////
// Geometric Functions //
/////////////////////////

DEFINE_BUILTIN(dot)
{
  int num = 1;
  if (callInst->getType()->isVectorTy())
  {
    ARG(0)->getType()->getVectorNumElements();
  }

  double r = 0.f;
  for (int i = 0; i < num; i++)
  {
    double a = FARGV(0, i);
    double b = FARGV(1, i);
    r += a * b;
  }
  setFloatResult(result, r);
}


///////////////////////
// Integer Functions //
///////////////////////

DEFINE_BUILTIN(hadd)
{
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, (UARGV(0, i) + UARGV(1, i)) >> 1, i);
  }
}


////////////////////
// Math Functions //
////////////////////

double acospi(double x){ return (acos(x) / M_PI); }
double asinpi(double x){ return (asin(x) / M_PI); }
double atanpi(double x){ return (atan(x) / M_PI); }
double atan2pi(double x, double y){ return (atan2(x, y) / M_PI); }
double cospi(double x){ return (cos(x * M_PI)); }
double exp10(double x){ return pow(10, x); }
double rsqrt(double x){ return 1.0 / sqrt(x); }
double sinpi(double x){ return (sin(x * M_PI)); }
double tanpi(double x){ return (tan(x * M_PI)); }

double maxmag(double x, double y)
{
  double _x = fabs(x);
  double _y = fabs(y);
  if (_x > _y)
  {
    return x;
  }
  else if (_y > _x)
  {
    return y;
  }
  else
  {
    return fmax(x, y);
  }
}
double minmag(double x, double y)
{
  double _x = fabs(x);
  double _y = fabs(y);
  if (_x < _y)
  {
    return x;
  }
  else if (_y < _x)
  {
    return y;
  }
  else
  {
    return fmin(x, y);
  }
}

DEFINE_BUILTIN(fract)
{
  size_t iptr = UARG(1);
  for (int i = 0; i < result.num; i++)
  {
    double x = FARGV(0, i);
    double fl = floor(x);
    double r = fmin(x - fl, 0x1.fffffep-1f);

    size_t offset = i*result.size;
    setFloatResult(result, fl, i);
    switch (ARG(1)->getType()->getPointerAddressSpace())
    {
      case AddrSpacePrivate:
        m_stack->store(result.data + offset, iptr + offset, result.size);
        break;
      case AddrSpaceGlobal:
        m_globalMemory.store(result.data + offset, iptr + offset, result.size);
        break;
      case AddrSpaceLocal:
        m_workGroup.getLocalMemory()->store(result.data + offset, iptr + offset, result.size);
        break;
    }

    setFloatResult(result, r, i);
  }
}

DEFINE_BUILTIN(frexp_builtin)
{
  size_t iptr = UARG(1);
  for (int i = 0; i < result.num; i++)
  {
    int32_t e;
    double r = frexp(FARGV(0, i), &e);
    switch (ARG(1)->getType()->getPointerAddressSpace())
    {
      case AddrSpacePrivate:
        m_stack->store((const unsigned char*)&e, iptr + i*4, 4);
        break;
      case AddrSpaceGlobal:
        m_globalMemory.store((const unsigned char*)&e, iptr + i*4, 4);
        break;
      case AddrSpaceLocal:
        m_workGroup.getLocalMemory()->store((const unsigned char*)&e, iptr + i*4, 4);
        break;
    }

    setFloatResult(result, r, i);
  }
}

DEFINE_BUILTIN(ilogb_builtin)
{
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, (int64_t)ilogb(FARGV(0, i)), i);
  }
}

DEFINE_BUILTIN(ldexp_builtin)
{
  for (int i = 0; i < result.num; i++)
  {
    setFloatResult(result, ldexp(FARGV(0, i), SARGV(1, i)), i);
  }
}

DEFINE_BUILTIN(lgamma_r)
{
  size_t signp = UARG(1);
  for (int i = 0; i < result.num; i++)
  {
    double r = lgamma(FARGV(0, i));
    int32_t s = (tgamma(FARGV(0, i)) < 0 ? -1 : 1);
    switch (ARG(1)->getType()->getPointerAddressSpace())
    {
      case AddrSpacePrivate:
        m_stack->store((const unsigned char*)&s, signp + i*4, 4);
        break;
      case AddrSpaceGlobal:
        m_globalMemory.store((const unsigned char*)&s, signp + i*4, 4);
        break;
      case AddrSpaceLocal:
        m_workGroup.getLocalMemory()->store((const unsigned char*)&s, signp + i*4, 4);
        break;
    }

    setFloatResult(result, r, i);
  }
}

DEFINE_BUILTIN(modf_builtin)
{
  size_t iptr = UARG(1);
  for (int i = 0; i < result.num; i++)
  {
    double x = FARGV(0, i);
    double integral = trunc(x);
    double fractional = copysign(isinf(x) ? 0.0 : x - integral, x);

    size_t offset = i*result.size;
    setFloatResult(result, integral, i);
    switch (ARG(1)->getType()->getPointerAddressSpace())
    {
      case AddrSpacePrivate:
        m_stack->store(result.data + offset, iptr + offset, result.size);
        break;
      case AddrSpaceGlobal:
        m_globalMemory.store(result.data + offset, iptr + offset, result.size);
        break;
      case AddrSpaceLocal:
        m_workGroup.getLocalMemory()->store(result.data + offset,
                                            iptr + offset, result.size);
        break;
    }

    setFloatResult(result, fractional, i);
  }
}

DEFINE_BUILTIN(nan_builtin)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t nancode = UARGV(0, i);
    setFloatResult(result, nan(""), i);
  }
}

DEFINE_BUILTIN(pown)
{
  for (int i = 0; i < result.num; i++)
  {
    double x = FARGV(0, i);
    int32_t y = SARGV(1, i);
    setFloatResult(result, pow(x, y), i);
  }
}

DEFINE_BUILTIN(remquo_builtin)
{
  size_t quop = UARG(2);
  for (int i = 0; i < result.num; i++)
  {
    double x = FARGV(0, i);
    double y = FARGV(1, i);

    int32_t quo;
    double rem = remquo(x, y, &quo);

    switch (ARG(2)->getType()->getPointerAddressSpace())
    {
      case AddrSpacePrivate:
        m_stack->store((const unsigned char*)&quo, quop + i*4, 4);
        break;
      case AddrSpaceGlobal:
        m_globalMemory.store((const unsigned char*)&quo, quop + i*4, 4);
        break;
      case AddrSpaceLocal:
        m_workGroup.getLocalMemory()->store((const unsigned char*)&quo, quop + i*4, 4);
        break;
    }

    setFloatResult(result, rem, i);
  }
}

DEFINE_BUILTIN(rootn)
{
  for (int i = 0; i < result.num; i++)
  {
    double x = FARGV(0, i);
    int y = SARGV(1, i);
    setFloatResult(result, pow(x, (double)(1.0/y)), i);
  }
}

DEFINE_BUILTIN(sincos)
{
  size_t cv = UARG(1);
  for (int i = 0; i < result.num; i++)
  {
    double x = FARGV(0, i);

    size_t offset = i*result.size;
    setFloatResult(result, cos(x), i);
    switch (ARG(1)->getType()->getPointerAddressSpace())
    {
      case AddrSpacePrivate:
        m_stack->store(result.data + offset, cv + offset, result.size);
        break;
      case AddrSpaceGlobal:
        m_globalMemory.store(result.data + offset, cv + offset, result.size);
        break;
      case AddrSpaceLocal:
        m_workGroup.getLocalMemory()->store(result.data + offset,
                                            cv + offset, result.size);
        break;
    }

    setFloatResult(result, sin(x), i);
  }
}


///////////////////////////////
// Synchronization Functions //
///////////////////////////////

DEFINE_BUILTIN(barrier)
{
	m_state = BARRIER;
}


//////////////////////////////////////////
// Vector Data Load and Store Functions //
//////////////////////////////////////////

DEFINE_BUILTIN(vload)
{
  const llvm::Value *ptrOp = ARG(1);
  size_t base = *(size_t*)(m_privateMemory[ptrOp].data);
  uint64_t offset = UARG(0);

  unsigned addressSpace = atoi(overload.substr(overload.length()-2).c_str());

  Memory *memory = NULL;
  switch (addressSpace)
  {
  case AddrSpacePrivate:
    memory = m_stack;
    break;
  case AddrSpaceGlobal:
  case AddrSpaceConstant:
    memory = &m_globalMemory;
    break;
  case AddrSpaceLocal:
    memory = m_workGroup.getLocalMemory();
    break;
  default:
    cerr << "Unhandled address space '" << addressSpace << "'" << endl;
    break;
  }

  if (!memory->load(result.data,
                    base + offset*result.size*result.num,
                    result.size*result.num))
  {
    outputMemoryError(*callInst, "Invalid read",
                      addressSpace, base + offset*result.size*result.num,
                      result.size*result.num);
  }
}

DEFINE_BUILTIN(vstore)
{
  const llvm::Value *value = ARG(0);
  size_t size = getTypeSize(value->getType());
  unsigned char *data = new unsigned char[size];
  if (isConstantOperand(value))
  {
    getConstantData(data, (const llvm::Constant*)value);
  }
  else
  {
    memcpy(data, m_privateMemory[value].data, size);
  }
  uint64_t offset = UARG(1);

  const llvm::Value *ptrOp = ARG(2);
  size_t base = *(size_t*)(m_privateMemory[ptrOp].data);

  unsigned addressSpace = atoi(overload.substr(overload.length()-2).c_str());

  Memory *memory = NULL;
  switch (addressSpace)
  {
  case AddrSpacePrivate:
    memory = m_stack;
    break;
  case AddrSpaceGlobal:
  case AddrSpaceConstant:
    memory = &m_globalMemory;
    break;
  case AddrSpaceLocal:
    memory = m_workGroup.getLocalMemory();
    break;
  default:
    cerr << "Unhandled address space '" << addressSpace << "'" << endl;
    break;
  }

  if (!memory->store(data, base + offset*size, size))
  {
    outputMemoryError(*callInst, "Invalid write",
                      addressSpace, base + offset*size, size);
  }
  delete[] data;
}


/////////////////////////
// Work-Item Functions //
/////////////////////////

DEFINE_BUILTIN(get_global_id)
{
	uint64_t dim = UARG(0);
  assert(dim < 3);
  *((size_t*)result.data) = m_globalID[dim];
}

DEFINE_BUILTIN(get_global_size)
{
  uint64_t dim = UARG(0);
  assert(dim < 3);
  *((size_t*)result.data) = m_workGroup.getGlobalSize()[dim];
}

DEFINE_BUILTIN(get_global_offset)
{
  uint64_t dim = UARG(0);
  assert(dim < 3);
  *((size_t*)result.data) = m_workGroup.getGlobalOffset()[dim];
}

DEFINE_BUILTIN(get_group_id)
{
  uint64_t dim = UARG(0);
  assert(dim < 3);
  *((size_t*)result.data) = m_workGroup.getGroupID()[dim];
}

DEFINE_BUILTIN(get_local_id)
{
  uint64_t dim = UARG(0);
  assert(dim < 3);
  *((size_t*)result.data) = m_localID[dim];
}

DEFINE_BUILTIN(get_local_size)
{
  uint64_t dim = UARG(0);
  assert(dim < 3);
  *((size_t*)result.data) = m_workGroup.getGroupSize()[dim];
}

DEFINE_BUILTIN(get_num_groups)
{
  uint64_t dim = UARG(0);
  assert(dim < 3);
  size_t num = m_workGroup.getGlobalSize()[dim] / m_workGroup.getGroupSize()[dim];
  *((size_t*)result.data) = num;
}

DEFINE_BUILTIN(get_work_dim)
{
  *((uint*)result.data) = m_workGroup.getWorkDim();
}


/////////////////////
// LLVM Intrinsics //
/////////////////////

DEFINE_BUILTIN(llvm_dbg_declare)
{
  // TODO: Implement?
}

DEFINE_BUILTIN(llvm_dbg_value)
{
  updateVariable((const llvm::DbgValueInst*)callInst);
}

DEFINE_BUILTIN(llvm_lifetime_start)
{
  // TODO: Implement?
}

DEFINE_BUILTIN(llvm_lifetime_end)
{
  // TODO: Implement?
}

DEFINE_BUILTIN(llvm_memcpy)
{
  const llvm::MemCpyInst *memcpyInst = (const llvm::MemCpyInst*)callInst;
  size_t dest = *(size_t*)(m_privateMemory[memcpyInst->getDest()].data);
  size_t src = *(size_t*)(m_privateMemory[memcpyInst->getSource()].data);
  size_t size = getUnsignedInt(memcpyInst->getLength());
  unsigned destAddrSpace = memcpyInst->getDestAddressSpace();
  unsigned srcAddrSpace = memcpyInst->getSourceAddressSpace();

  Memory *destMemory = NULL;
  switch (destAddrSpace)
  {
  case AddrSpacePrivate:
    destMemory = m_stack;
    break;
  case AddrSpaceGlobal:
  case AddrSpaceConstant:
    destMemory = &m_globalMemory;
    break;
  case AddrSpaceLocal:
    destMemory = m_workGroup.getLocalMemory();
    break;
  default:
    cerr << "Unhandled address space '" << destAddrSpace << "'" << endl;
    break;
  }

  Memory *srcMemory = NULL;
  switch (srcAddrSpace)
  {
  case AddrSpacePrivate:
    srcMemory = m_stack;
    break;
  case AddrSpaceGlobal:
  case AddrSpaceConstant:
    srcMemory = &m_globalMemory;
    break;
  case AddrSpaceLocal:
    srcMemory = m_workGroup.getLocalMemory();
    break;
  default:
    cerr << "Unhandled address space '" << srcAddrSpace << "'" << endl;
    break;
  }

  unsigned char *buffer = new unsigned char[size];
  if (!srcMemory->load(buffer, src, size))
  {
    outputMemoryError(*callInst, "Invalid read",
                      srcAddrSpace, src, size);
  }
  else if (!destMemory->store(buffer, dest, size))
  {
    outputMemoryError(*callInst, "Invalid write",
                      destAddrSpace, dest, size);
  }
  delete[] buffer;
}

DEFINE_BUILTIN(llvm_memset)
{
  const llvm::MemSetInst *memsetInst = (const llvm::MemSetInst*)callInst;
  size_t dest = *(size_t*)(m_privateMemory[memsetInst->getDest()].data);
  size_t size = getUnsignedInt(memsetInst->getLength());
  unsigned addressSpace = memsetInst->getDestAddressSpace();

  Memory *mem = NULL;
  switch (addressSpace)
  {
  case AddrSpacePrivate:
    mem = m_stack;
    break;
  case AddrSpaceGlobal:
  case AddrSpaceConstant:
    mem = &m_globalMemory;
    break;
  case AddrSpaceLocal:
    mem = m_workGroup.getLocalMemory();
    break;
  default:
    cerr << "Unhandled address space '" << addressSpace << "'" << endl;
    break;
  }

  unsigned char *buffer = new unsigned char[size];
  unsigned char value = getUnsignedInt(memsetInst->getArgOperand(1));
  memset(buffer, value, size);
  if (!mem->store(buffer, dest, size))
  {
    outputMemoryError(*callInst, "Invalid write",
                      addressSpace, dest, size);
  }
  delete[] buffer;
}

DEFINE_BUILTIN(llvm_trap)
{
  trap();
}
