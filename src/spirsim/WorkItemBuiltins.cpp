// WorkItemBuiltins.cpp (oclgrind)
// Copyright (C) 2013 James Price
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.

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

#define DEFINE_BUILTIN(name)                                              \
  void WorkItem::name(WorkItem *workItem, const llvm::CallInst *callInst, \
                      std::string fnName, std::string overload,           \
                      TypedValue& result, void *)
#define ARG(i) (callInst->getArgOperand(i))
#define UARG(i) workItem->getUnsignedInt(callInst->getArgOperand(i))
#define SARG(i) workItem->getSignedInt(callInst->getArgOperand(i))
#define FARG(i) workItem->getFloatValue(callInst->getArgOperand(i))
#define UARGV(i,v) workItem->getUnsignedInt(callInst->getArgOperand(i), v)
#define SARGV(i,v) workItem->getSignedInt(callInst->getArgOperand(i), v)
#define FARGV(i,v) workItem->getFloatValue(callInst->getArgOperand(i), v)

void WorkItem::builtin_f1arg(WorkItem *workItem,
                             const llvm::CallInst *callInst,
                             string name,
                             string overload,
                             TypedValue& result,
                             double (*func)(double))
{
  for (int i = 0; i < result.num; i++)
  {
    setFloatResult(result, func(FARGV(0, i)), i);
  }
}

void WorkItem::builtin_f2arg(WorkItem *workItem,
                             const llvm::CallInst *callInst,
                             string name,
                             string overload,
                             TypedValue& result,
                             double (*func)(double, double))
{
  for (int i = 0; i < result.num; i++)
  {
    setFloatResult(result, func(FARGV(0, i), FARGV(1, i)), i);
  }
}

void WorkItem::builtin_f3arg(WorkItem *workItem,
                             const llvm::CallInst *callInst,
                             string name,
                             string overload,
                             TypedValue& result,
                             double (*func)(double, double, double))
{
  for (int i = 0; i < result.num; i++)
  {
    setFloatResult(result, func(FARGV(0, i), FARGV(1, i), FARGV(2, i)), i);
  }
}

void WorkItem::builtin_u1arg(WorkItem *workItem,
                             const llvm::CallInst *callInst,
                             string name,
                             string overload,
                             TypedValue& result,
                             uint64_t (*func)(uint64_t))
{
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, func(UARGV(0, i)), i);
  }
}

void WorkItem::builtin_u2arg(WorkItem *workItem,
                             const llvm::CallInst *callInst,
                             string name,
                             string overload,
                             TypedValue& result,
                             uint64_t (*func)(uint64_t, uint64_t))
{
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, func(UARGV(0, i), UARGV(1, i)), i);
  }
}

void WorkItem::builtin_u3arg(WorkItem *workItem,
                             const llvm::CallInst *callInst,
                             string name,
                             string overload,
                             TypedValue& result,
                             uint64_t (*func)(uint64_t, uint64_t, uint64_t))
{
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, func(UARGV(0, i), UARGV(1, i), UARGV(2, i)), i);
  }
}

void WorkItem::builtin_s1arg(WorkItem *workItem,
                             const llvm::CallInst *callInst,
                             string name,
                             string overload,
                             TypedValue& result,
                             int64_t (*func)(int64_t))
{
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, func(SARGV(0, i)), i);
  }
}

void WorkItem::builtin_s2arg(WorkItem *workItem,
                             const llvm::CallInst *callInst,
                             string name,
                             string overload,
                             TypedValue& result,
                             int64_t (*func)(int64_t, int64_t))
{
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, func(SARGV(0, i), SARGV(1, i)), i);
  }
}

void WorkItem::builtin_s3arg(WorkItem *workItem,
                             const llvm::CallInst *callInst,
                             string name,
                             string overload,
                             TypedValue& result,
                             int64_t (*func)(int64_t, int64_t, int64_t))
{
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, func(SARGV(0, i), SARGV(1, i), SARGV(2, i)), i);
  }
}

void WorkItem::builtin_rel1arg(WorkItem *workItem,
                               const llvm::CallInst *callInst,
                               string name,
                               string overload,
                               TypedValue& result,
                               int (*func)(double))
{
  int t = result.num > 1 ? -1 : 1;
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, (int64_t)func(FARGV(0, i))*t, i);
  }
}

void WorkItem::builtin_rel2arg(WorkItem *workItem,
                               const llvm::CallInst *callInst,
                               string name,
                               string overload,
                               TypedValue& result,
                               int (*func)(double, double))
{
  int t = result.num > 1 ? -1 : 1;
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, (int64_t)func(FARGV(0, i), FARGV(1, i))*t, i);
  }
}

char getOverloadArgType(string overload)
{
  char type = overload[0];
  if (type == 'D')
  {
    char *typestr;
    strtol(overload.c_str() + 2, &typestr, 10);
    type = typestr[1];
  }
  return type;
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
  size_t dest = *(size_t*)(workItem->m_privateMemory[destOp].data);
  size_t src = *(size_t*)(workItem->m_privateMemory[srcOp].data);

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
  event = workItem->m_workGroup.async_copy(copy, event);
  setIntResult(result, event);
}

DEFINE_BUILTIN(wait_group_events)
{
  uint64_t num = UARG(0);
  const llvm::Value *ptrOp = ARG(1);
  size_t address = *(size_t*)(workItem->m_privateMemory[ptrOp].data);
  for (int i = 0; i < num; i++)
  {
    // TODO: Can we safely assume this is private/stack data?
    uint64_t event;
    if (!workItem->m_stack->load((unsigned char*)&event,
                                 address, sizeof(uint64_t)))
    {
      workItem->outputMemoryError(*callInst, "Invalid read", AddrSpacePrivate,
                            address, sizeof(uint64_t));
      return;
    }
    workItem->m_workGroup.wait_event(event);
    address += sizeof(uint64_t);
  }
  workItem->m_state = WAIT_EVENT;
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
template<typename T> T _clamp(T x, T min, T max){return _min(_max(x, min), max);}

double degrees(double x)
{
  return x * (180 / M_PI);
}

double mix(double x, double y, double a)
{
  return x + (y - x) * a;
}

double radians(double x)
{
  return x * (M_PI / 180);
}

double sign(double x)
{
  if (isnan(x))  return  0.0;
  if (x  >  0.0) return  1.0;
  if (x == -0.0) return -0.0;
  if (x ==  0.0) return  0.0;
  if (x  <  0.0) return -1.0;
  return 0.0;
}

double smoothstep(double edge0, double edge1, double x)
{
  double t = _clamp<double>((x - edge0) / (edge1 - edge0), 0, 1);
  return t * t * (3 - 2*t);
}

double step_builtin(double edge, double x)
{
  return (x < edge) ? 0.0 : 1.0;
}

DEFINE_BUILTIN(clamp)
{
  switch (getOverloadArgType(overload))
  {
    case 'f':
    case 'd':
      builtin_f3arg(workItem, callInst, fnName, overload, result, _clamp);
      break;
    case 'h':
    case 't':
    case 'j':
    case 'm':
      builtin_u3arg(workItem, callInst, fnName, overload, result, _clamp);
      break;
    case 'c':
    case 's':
    case 'i':
    case 'l':
      builtin_s3arg(workItem, callInst, fnName, overload, result, _clamp);
      break;
    default:
      assert(false);
    }
}

DEFINE_BUILTIN(max)
{
  switch (getOverloadArgType(overload))
  {
    case 'f':
    case 'd':
      builtin_f2arg(workItem, callInst, fnName, overload, result, fmax);
      break;
    case 'h':
    case 't':
    case 'j':
    case 'm':
      builtin_u2arg(workItem, callInst, fnName, overload, result, _max);
      break;
    case 'c':
    case 's':
    case 'i':
    case 'l':
      builtin_s2arg(workItem, callInst, fnName, overload, result, _max);
      break;
    default:
      assert(false);
    }
}

DEFINE_BUILTIN(min)
{
  switch (getOverloadArgType(overload))
  {
    case 'f':
    case 'd':
      builtin_f2arg(workItem, callInst, fnName, overload, result, fmin);
      break;
    case 'h':
    case 't':
    case 'j':
    case 'm':
      builtin_u2arg(workItem, callInst, fnName, overload, result, _min);
      break;
    case 'c':
    case 's':
    case 'i':
    case 'l':
      builtin_s2arg(workItem, callInst, fnName, overload, result, _min);
      break;
    default:
      assert(false);
    }
}


/////////////////////////
// Geometric Functions //
/////////////////////////

DEFINE_BUILTIN(cross)
{
  double u1 = FARGV(0, 0);
  double u2 = FARGV(0, 1);
  double u3 = FARGV(0, 2);
  double v1 = FARGV(1, 0);
  double v2 = FARGV(1, 1);
  double v3 = FARGV(1, 2);
  setFloatResult(result, u2*v3 - u3*v2, 0);
  setFloatResult(result, u3*v1 - u1*v3, 1);
  setFloatResult(result, u1*v2 - u2*v1, 2);
  setFloatResult(result, 0, 3);
}

DEFINE_BUILTIN(dot)
{
  int num = 1;
  if (ARG(0)->getType()->isVectorTy())
  {
    num = ARG(0)->getType()->getVectorNumElements();
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

DEFINE_BUILTIN(distance)
{
  int num = 1;
  if (ARG(0)->getType()->isVectorTy())
  {
    num = ARG(0)->getType()->getVectorNumElements();
  }

  double distSq = 0.0;
  for (int i = 0; i < num; i++)
  {
    double diff = FARGV(0,i) - FARGV(1,i);
    distSq += diff*diff;
  }
  setFloatResult(result, sqrt(distSq));
}

DEFINE_BUILTIN(length)
{
  int num = 1;
  if (ARG(0)->getType()->isVectorTy())
  {
    num = ARG(0)->getType()->getVectorNumElements();
  }

  double lengthSq = 0.0;
  for (int i = 0; i < num; i++)
  {
    lengthSq += FARGV(0, i) * FARGV(0, i);
  }
  setFloatResult(result, sqrt(lengthSq));
}

DEFINE_BUILTIN(normalize)
{
  double lengthSq = 0.0;
  for (int i = 0; i < result.num; i++)
  {
    lengthSq += FARGV(0, i) * FARGV(0, i);
  }
  double length = sqrt(lengthSq);

  for (int i = 0; i < result.num; i++)
  {
    setFloatResult(result, FARGV(0, i)/length, i);
  }
}


///////////////////////
// Integer Functions //
///////////////////////

DEFINE_BUILTIN(abs_builtin)
{
  for (int i = 0; i < result.num; i++)
  {
    switch (getOverloadArgType(overload))
    {
    case 'h':
    case 't':
    case 'j':
    case 'm':
    {
      setIntResult(result, UARGV(0,i), i);
      break;
    }
    case 'c':
    case 's':
    case 'i':
    case 'l':
    {
      setIntResult(result, abs(SARGV(0,i)), i);
      break;
    }
    default:
      assert(false);
    }
  }
}

DEFINE_BUILTIN(abs_diff)
{
  for (int i = 0; i < result.num; i++)
  {
    switch (getOverloadArgType(overload))
    {
    case 'h':
    case 't':
    case 'j':
    case 'm':
    {
      uint64_t a = UARGV(0, i);
      uint64_t b = UARGV(1, i);
      setIntResult(result, _max(a,b) - _min(a,b), i);
      break;
    }
    case 'c':
    case 's':
    case 'i':
    case 'l':
    {
      int64_t a = SARGV(0, i);
      int64_t b = SARGV(1, i);
      setIntResult(result, _max(a,b) - _min(a,b), i);
      break;
    }
    default:
      assert(false);
    }
  }
}

DEFINE_BUILTIN(add_sat)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t uresult = UARGV(0,i) + UARGV(1,i);
    int64_t  sresult = SARGV(0,i) + SARGV(1,i);
    switch (getOverloadArgType(overload))
    {
    case 'h':
      setIntResult(result, _min<uint64_t>(uresult, UINT8_MAX), i);
      break;
    case 't':
      setIntResult(result, _min<uint64_t>(uresult, UINT16_MAX), i);
      break;
    case 'j':
      setIntResult(result, _min<uint64_t>(uresult, UINT32_MAX), i);
      break;
    case 'm':
    {
      if (UARGV(1, i) > uresult)
      {
        uresult = UINT64_MAX;
      }
      setIntResult(result, uresult, i);
      break;
    }
    case 'c':
      setIntResult(result, _clamp<int64_t>(sresult, INT8_MIN, INT8_MAX), i);
      break;
    case 's':
      setIntResult(result, _clamp<int64_t>(sresult, INT16_MIN, INT16_MAX), i);
      break;
    case 'i':
      setIntResult(result, _clamp<int64_t>(sresult, INT32_MIN, INT32_MAX), i);
      break;
    case 'l':
    {
      if ((SARGV(0,i)>0) == (SARGV(1,i)>0) && (SARGV(0,i)>0) != (sresult>0))
      {
        sresult = (SARGV(0,i)>0) ? INT64_MAX : INT64_MIN;
      }
      setIntResult(result, sresult, i);
      break;
    }
    default:
      assert(false);
    }
  }
}

DEFINE_BUILTIN(clz)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t x = UARGV(0, i);
    int nz = 0;
    while (x)
    {
      x >>= 1;
      nz++;
    }

    setIntResult(result, (uint64_t)((result.size<<3) - nz), i);
  }
}

DEFINE_BUILTIN(hadd)
{
  for (int i = 0; i < result.num; i++)
  {
    switch (getOverloadArgType(overload))
    {
    case 'h':
    case 't':
    case 'j':
    case 'm':
    {
      uint64_t a = UARGV(0, i);
      uint64_t b = UARGV(1, i);
      uint64_t c = (a > UINT64_MAX-b) ? (1L<<63) : 0;
      setIntResult(result, ((a + b) >> 1) | c, i);
      break;
    }
    case 'c':
    case 's':
    case 'i':
    case 'l':
    {
      int64_t a = SARGV(0, i);
      int64_t b = SARGV(1, i);
      int64_t c = (a & b) & 1;
      setIntResult(result, (a>>1) + (b>>1) + c, i);
      break;
    }
    default:
      assert(false);
    }
  }
}

uint64_t mad(uint64_t a, uint64_t b, uint64_t c)
{
  return a*b + c;
}

uint64_t umul_hi(uint64_t x, uint64_t y, uint64_t bits)
{
  if (bits == 64)
  {
    uint64_t xl = x & UINT32_MAX;
    uint64_t xh = x >> 32;
    uint64_t yl = y & UINT32_MAX;
    uint64_t yh = y >> 32;

    uint64_t xlyl = xl*yl;
    uint64_t xlyh = xl*yh;
    uint64_t xhyl = xh*yl;
    uint64_t xhyh = xh*yh;

    uint64_t  a = xhyl + ((xlyl)>>32);
    uint64_t al = a & UINT32_MAX;
    uint64_t ah = a >> 32;
    uint64_t  b = ((al + xlyh)>>32) + ah;

    return xh*yh + b;
  }
  else
  {
    return (x*y) >> bits;
  }
}

int64_t smul_hi(int64_t x, int64_t y, int64_t bits)
{
  if (bits == 64)
  {
    // TODO: Sometimes 1 out
    int64_t xl = x & UINT32_MAX;
    int64_t xh = x >> 32;
    int64_t yl = y & UINT32_MAX;
    int64_t yh = y >> 32;

    int64_t xlyl = xl*yl;
    int64_t xlyh = xl*yh;
    int64_t xhyl = xh*yl;
    int64_t xhyh = xh*yh;

    int64_t  a = xhyl + ((xlyl)>>32);
    int64_t al = a & UINT32_MAX;
    int64_t ah = a >> 32;
    int64_t  b = ((al + xlyh)>>32) + ah;

    return xh*yh + b;
  }
  else
  {
    return (x*y) >> bits;
  }
}

DEFINE_BUILTIN(mad_hi)
{
  for (int i = 0; i < result.num; i++)
  {
    switch (getOverloadArgType(overload))
    {
    case 'h':
    case 't':
    case 'j':
    case 'm':
      setIntResult(result,
                   umul_hi(UARGV(0, i), UARGV(1, i),
                           result.size<<3) + UARGV(2, i),
                   i);
      break;
    case 'c':
    case 's':
    case 'i':
    case 'l':
      setIntResult(result,
                   smul_hi(SARGV(0, i), SARGV(1, i),
                           result.size<<3) + SARGV(2, i),
                   i);
      break;
    default:
      assert(false);
    }
  }
}

DEFINE_BUILTIN(mad_sat)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t uresult = UARGV(0,i)*UARGV(1,i) + UARGV(2,i);
    int64_t  sresult = SARGV(0,i)*SARGV(1,i) + SARGV(2,i);
    switch (getOverloadArgType(overload))
    {
    case 'h':
      setIntResult(result, _min<uint64_t>(uresult, UINT8_MAX), i);
      break;
    case 't':
      setIntResult(result, _min<uint64_t>(uresult, UINT16_MAX), i);
      break;
    case 'j':
      setIntResult(result, _min<uint64_t>(uresult, UINT32_MAX), i);
      break;
    case 'm':
    {
      uint64_t hi = umul_hi(UARGV(0, i), UARGV(1, i), 64);
      if (hi || UARGV(2, i) > uresult)
      {
        uresult = UINT64_MAX;
      }
      setIntResult(result, uresult, i);
      break;
    }
    case 'c':
      setIntResult(result, _clamp<int64_t>(sresult, INT8_MIN, INT8_MAX), i);
      break;
    case 's':
      setIntResult(result, _clamp<int64_t>(sresult, INT16_MIN, INT16_MAX), i);
      break;
    case 'i':
      setIntResult(result, _clamp<int64_t>(sresult, INT32_MIN, INT32_MAX), i);
      break;
    case 'l':
    {
      // Check for overflow in multiplication
      if (smul_hi(SARGV(0, i), SARGV(1, i), 64))
      {
        sresult = (SARGV(0,i)>0) ^ (SARGV(1,i)>0) ? INT64_MIN : INT64_MAX;
      }
      else
      {
        // Check for overflow in addition
        int64_t m = SARGV(0, i) * SARGV(1, i);
        if ((m>0) == (SARGV(2,i)>0) && (m>0) != (sresult>0))
        {
          sresult = (m>0) ? INT64_MAX : INT64_MIN;
        }
      }
      setIntResult(result, sresult, i);
      break;
    }
    default:
      assert(false);
    }
  }
}

uint64_t mul_builtin(uint64_t a, uint64_t b)
{
  return a*b;
}

DEFINE_BUILTIN(mul_hi)
{
  for (int i = 0; i < result.num; i++)
  {
    switch (getOverloadArgType(overload))
    {
    case 'h':
    case 't':
    case 'j':
    case 'm':
      setIntResult(result, umul_hi(UARGV(0, i), UARGV(1, i), result.size<<3), i);
      break;
    case 'c':
    case 's':
    case 'i':
    case 'l':
      setIntResult(result, smul_hi(SARGV(0, i), SARGV(1, i), result.size<<3), i);
      break;
    default:
      assert(false);
    }
  }
}

uint64_t popcount(uint64_t x)
{
  int i = 0;
  while (x)
  {
    i += (x & 0x1);
    x >>= 1;
  }
  return i;
}

DEFINE_BUILTIN(rhadd)
{
  for (int i = 0; i < result.num; i++)
  {
    switch (getOverloadArgType(overload))
    {
    case 'h':
    case 't':
    case 'j':
    case 'm':
    {
      uint64_t a = UARGV(0, i);
      uint64_t b = UARGV(1, i);
      uint64_t c = (a > UINT64_MAX-(b+1)) ? (1L<<63) : 0;
      setIntResult(result, ((a + b + 1) >> 1) | c, i);
      break;
    }
    case 'c':
    case 's':
    case 'i':
    case 'l':
    {
      int64_t a = SARGV(0, i);
      int64_t b = SARGV(1, i);
      int64_t c = (a | b) & 1;
      setIntResult(result, (a>>1) + (b>>1) + c, i);
      break;
    }
    default:
      assert(false);
    }
  }
}

DEFINE_BUILTIN(rotate)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t width = (result.size << 3);
    uint64_t v  = UARGV(0, i);
    uint64_t ls = UARGV(1, i) % width;
    uint64_t rs = width - ls;
    setIntResult(result, (v << ls) | (v >> rs), i);
  }
}

DEFINE_BUILTIN(sub_sat)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t uresult = UARGV(0,i) - UARGV(1,i);
    int64_t  sresult = SARGV(0,i) - SARGV(1,i);
    switch (getOverloadArgType(overload))
    {
    case 'h':
      setIntResult(result, uresult > UINT8_MAX ? 0 : uresult, i);
      break;
    case 't':
      setIntResult(result, uresult > UINT16_MAX ? 0 : uresult, i);
      break;
    case 'j':
      setIntResult(result, uresult > UINT32_MAX ? 0 : uresult, i);
      break;
    case 'm':
    {
      if (UARGV(1, i) > UARGV(0, i))
      {
        uresult = 0;
      }
      setIntResult(result, uresult, i);
      break;
    }
    case 'c':
      setIntResult(result, _clamp<int64_t>(sresult, INT8_MIN, INT8_MAX), i);
      break;
    case 's':
      setIntResult(result, _clamp<int64_t>(sresult, INT16_MIN, INT16_MAX), i);
      break;
    case 'i':
      setIntResult(result, _clamp<int64_t>(sresult, INT32_MIN, INT32_MAX), i);
      break;
    case 'l':
    {
      if ((SARGV(0,i)>0) != (SARGV(1,i)>0) && (SARGV(0,i)>0) != (sresult>0))
      {
        sresult = (SARGV(0,i)>0) ? INT64_MAX : INT64_MIN;
      }
      setIntResult(result, sresult, i);
      break;
    }
    default:
      assert(false);
    }
  }
}

DEFINE_BUILTIN(upsample)
{
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, (UARGV(0,i)<<(result.size<<2)) | UARGV(1, i), i);
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
double fdivide(double x, double y){ return x/y; }
double frecip(double x){ return 1.0/x; }
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
        workItem->m_stack->store(result.data + offset,
                                 iptr + offset, result.size);
        break;
      case AddrSpaceGlobal:
        workItem->m_globalMemory.store(result.data + offset,
                                       iptr + offset, result.size);
        break;
      case AddrSpaceLocal:
        workItem->m_workGroup.getLocalMemory()->store(
          result.data + offset,iptr + offset, result.size);
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
        workItem->m_stack->store((const unsigned char*)&e, iptr + i*4, 4);
        break;
      case AddrSpaceGlobal:
        workItem->m_globalMemory.store((const unsigned char*)&e, iptr + i*4, 4);
        break;
      case AddrSpaceLocal:
        workItem->m_workGroup.getLocalMemory()->store(
          (const unsigned char*)&e, iptr + i*4, 4);
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
        workItem->m_stack->store((const unsigned char*)&s, signp + i*4, 4);
        break;
      case AddrSpaceGlobal:
        workItem->m_globalMemory.store((const unsigned char*)&s, signp + i*4, 4);
        break;
      case AddrSpaceLocal:
        workItem->m_workGroup.getLocalMemory()->store(
          (const unsigned char*)&s, signp + i*4, 4);
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
        workItem->m_stack->store(result.data + offset,
                                 iptr + offset, result.size);
        break;
      case AddrSpaceGlobal:
        workItem->m_globalMemory.store(result.data + offset,
                                       iptr + offset, result.size);
        break;
      case AddrSpaceLocal:
        workItem->m_workGroup.getLocalMemory()->store(
          result.data + offset, iptr + offset, result.size);
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
        workItem->m_stack->store((const unsigned char*)&quo, quop + i*4, 4);
        break;
      case AddrSpaceGlobal:
        workItem->m_globalMemory.store((const unsigned char*)&quo,
                                       quop + i*4, 4);
        break;
      case AddrSpaceLocal:
        workItem->m_workGroup.getLocalMemory()->store(
          (const unsigned char*)&quo, quop + i*4, 4);
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
        workItem->m_stack->store(result.data + offset,
                                 cv + offset, result.size);
        break;
      case AddrSpaceGlobal:
        workItem->m_globalMemory.store(result.data + offset,
                                       cv + offset, result.size);
        break;
      case AddrSpaceLocal:
        workItem->m_workGroup.getLocalMemory()->store(
          result.data + offset, cv + offset, result.size);
        break;
    }

    setFloatResult(result, sin(x), i);
  }
}


////////////////////////////
// Misc. Vector Functions //
////////////////////////////

DEFINE_BUILTIN(shuffle_builtin)
{
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, UARGV(0, UARGV(1, i)), i);
  }
}

DEFINE_BUILTIN(shuffle2_builtin)
{
  for (int i = 0; i < result.num; i++)
  {
    uint64_t m = 1;
    if (ARG(0)->getType()->isVectorTy())
    {
      m = ARG(0)->getType()->getVectorNumElements();
    }

    uint64_t src = 0;
    uint64_t index = UARGV(2, i);
    if (index >= m)
    {
      index -= m;
      src = 1;
    }
    setIntResult(result, UARGV(src, index), i);
  }
}


///////////////////////////////
// Relational Functions //
///////////////////////////////

int isequal_builtin(double x, double y){ return x == y; }
int isnotequal_builtin(double x, double y){ return x != y; }
int isgreater_builtin(double x, double y){ return isgreater(x, y); }
int isgreaterequal_builtin(double x, double y){ return isgreaterequal(x, y); }
int isless_builtin(double x, double y){ return isless(x, y); }
int islessequal_builtin(double x, double y){ return islessequal(x, y); }
int islessgreater_builtin(double x, double y){ return islessgreater(x, y); }
int isfinite_builtin(double x){ return isfinite(x); }
int isinf_builtin(double x){ return isinf(x); }
int isnan_builtin(double x){ return isnan(x); }
int isnormal_builtin(double x){ return isnormal(x); }
int isordered_builtin(double x, double y){ return !isunordered(x, y); }
int isunordered_builtin(double x, double y){ return isunordered(x, y); }
int signbit_builtin(double x){ return signbit(x); }

DEFINE_BUILTIN(all)
{
  int num = 1;
  if (ARG(0)->getType()->isVectorTy())
  {
    num = ARG(0)->getType()->getVectorNumElements();
  }

  for (int i = 0; i < num; i++)
  {
    if (!signbit(SARGV(0, i)))
    {
      setIntResult(result, (int64_t)0);
      return;
    }
  }
  setIntResult(result, (int64_t)1);
}

DEFINE_BUILTIN(any)
{
  int num = 1;
  if (ARG(0)->getType()->isVectorTy())
  {
    num = ARG(0)->getType()->getVectorNumElements();
  }

  for (int i = 0; i < num; i++)
  {
    if (signbit(SARGV(0, i)))
    {
      setIntResult(result, (int64_t)1);
      return;
    }
  }
  setIntResult(result, (int64_t)0);
}

uint64_t ibitselect(uint64_t a, uint64_t b, uint64_t c)
{
  return ((a & ~c) | (b & c));
}

double fbitselect(double a, double b, double c)
{
  uint64_t _a = *(uint64_t*)&a;
  uint64_t _b = *(uint64_t*)&b;
  uint64_t _c = *(uint64_t*)&c;
  uint64_t _r = ibitselect(_a, _b, _c);
  return *(double*)&_r;
}

DEFINE_BUILTIN(bitselect)
{
  switch (getOverloadArgType(overload))
  {
    case 'f':
    case 'd':
      builtin_f3arg(workItem, callInst, fnName, overload, result, fbitselect);
      break;
    case 'h':
    case 't':
    case 'j':
    case 'm':
    case 'c':
    case 's':
    case 'i':
    case 'l':
      builtin_u3arg(workItem, callInst, fnName, overload, result, ibitselect);
      break;
    default:
      assert(false);
    }
}

DEFINE_BUILTIN(select_builtin)
{
  char type = getOverloadArgType(overload);
  for (int i = 0; i < result.num; i++)
  {
    int64_t c = SARGV(2, i);
    bool _c = (result.num > 1) ? signbit(c) : c;
    switch (type)
    {
      case 'f':
      case 'd':
        setFloatResult(result, _c ? FARGV(1, i) : FARGV(0, i), i);
        break;
      case 'h':
      case 't':
      case 'j':
      case 'm':
      case 'c':
      case 's':
      case 'i':
      case 'l':
       setIntResult(result, _c ? SARGV(1, i) : SARGV(0, i), i);
      break;
    default:
      assert(false);
    }
  }
}


///////////////////////////////
// Synchronization Functions //
///////////////////////////////

DEFINE_BUILTIN(barrier)
{
  workItem->m_state = BARRIER;
}

DEFINE_BUILTIN(mem_fence)
{
  // TODO: Implement?
}


//////////////////////////////////////////
// Vector Data Load and Store Functions //
//////////////////////////////////////////

DEFINE_BUILTIN(vload)
{
  const llvm::Value *ptrOp = ARG(1);
  size_t base = *(size_t*)(workItem->m_privateMemory[ptrOp].data);
  uint64_t offset = UARG(0);

  unsigned addressSpace = atoi(overload.substr(overload.length()-2).c_str());

  Memory *memory = NULL;
  switch (addressSpace)
  {
  case AddrSpacePrivate:
    memory = workItem->m_stack;
    break;
  case AddrSpaceGlobal:
  case AddrSpaceConstant:
    memory = &workItem->m_globalMemory;
    break;
  case AddrSpaceLocal:
    memory = workItem->m_workGroup.getLocalMemory();
    break;
  default:
    cerr << "Unhandled address space '" << addressSpace << "'" << endl;
    break;
  }

  if (!memory->load(result.data,
                    base + offset*result.size*result.num,
                    result.size*result.num))
  {
    workItem->outputMemoryError(*callInst, "Invalid read",
                          addressSpace, base + offset*result.size*result.num,
                          result.size*result.num);
  }
}

DEFINE_BUILTIN(vstore)
{
  const llvm::Value *value = ARG(0);
  size_t size = getTypeSize(value->getType());
  unsigned char *data = new unsigned char[size];
  if (isVector3(value))
  {
    // 3-element vectors are same size as 4-element vectors,
    // but vstore address offset shouldn't use this.
    size = (size/4) * 3;
  }

  if (isConstantOperand(value))
  {
    getConstantData(data, (const llvm::Constant*)value);
  }
  else
  {
    memcpy(data, workItem->m_privateMemory[value].data, size);
  }
  uint64_t offset = UARG(1);

  const llvm::Value *ptrOp = ARG(2);
  size_t base = *(size_t*)(workItem->m_privateMemory[ptrOp].data);

  unsigned addressSpace = atoi(overload.substr(overload.length()-2).c_str());

  Memory *memory = NULL;
  switch (addressSpace)
  {
  case AddrSpacePrivate:
    memory = workItem->m_stack;
    break;
  case AddrSpaceGlobal:
  case AddrSpaceConstant:
    memory = &workItem->m_globalMemory;
    break;
  case AddrSpaceLocal:
    memory = workItem->m_workGroup.getLocalMemory();
    break;
  default:
    cerr << "Unhandled address space '" << addressSpace << "'" << endl;
    break;
  }

  if (!memory->store(data, base + offset*size, size))
  {
    workItem->outputMemoryError(*callInst, "Invalid write",
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
  *((size_t*)result.data) = workItem->m_globalID[dim];
}

DEFINE_BUILTIN(get_global_size)
{
  uint64_t dim = UARG(0);
  assert(dim < 3);
  *((size_t*)result.data) = workItem->m_workGroup.getGlobalSize()[dim];
}

DEFINE_BUILTIN(get_global_offset)
{
  uint64_t dim = UARG(0);
  assert(dim < 3);
  *((size_t*)result.data) = workItem->m_workGroup.getGlobalOffset()[dim];
}

DEFINE_BUILTIN(get_group_id)
{
  uint64_t dim = UARG(0);
  assert(dim < 3);
  *((size_t*)result.data) = workItem->m_workGroup.getGroupID()[dim];
}

DEFINE_BUILTIN(get_local_id)
{
  uint64_t dim = UARG(0);
  assert(dim < 3);
  *((size_t*)result.data) = workItem->m_localID[dim];
}

DEFINE_BUILTIN(get_local_size)
{
  uint64_t dim = UARG(0);
  assert(dim < 3);
  *((size_t*)result.data) = workItem->m_workGroup.getGroupSize()[dim];
}

DEFINE_BUILTIN(get_num_groups)
{
  uint64_t dim = UARG(0);
  assert(dim < 3);
  size_t num = workItem->m_workGroup.getGlobalSize()[dim] /
               workItem->m_workGroup.getGroupSize()[dim];
  *((size_t*)result.data) = num;
}

DEFINE_BUILTIN(get_work_dim)
{
  *((uint*)result.data) = workItem->m_workGroup.getWorkDim();
}


/////////////////////
// Other Functions //
/////////////////////

DEFINE_BUILTIN(convert_float)
{
  for (int i = 0; i < result.num; i++)
  {
    switch (getOverloadArgType(overload))
    {
    case 'h':
    case 't':
    case 'j':
    case 'm':
      setFloatResult(result, (float)UARGV(0, i), i);
      break;
    case 'c':
    case 's':
    case 'i':
    case 'l':
      setFloatResult(result, (float)SARGV(0, i), i);
      break;
    case 'f':
    case 'd':
      setFloatResult(result, FARGV(0, i), i);
      break;
    default:
      assert(false);
    }
  }
}

DEFINE_BUILTIN(convert_uint)
{
  for (int i = 0; i < result.num; i++)
  {
    switch (getOverloadArgType(overload))
    {
    case 'h':
    case 't':
    case 'j':
    case 'm':
      setIntResult(result, UARGV(0, i), i);
      break;
    case 'c':
    case 's':
    case 'i':
    case 'l':
      setIntResult(result, (uint64_t)SARGV(0, i), i);
      break;
    case 'f':
    case 'd':
      setIntResult(result, (uint64_t)FARGV(0, i), i);
      break;
    default:
      assert(false);
    }
  }
}

DEFINE_BUILTIN(convert_sint)
{
  for (int i = 0; i < result.num; i++)
  {
    switch (getOverloadArgType(overload))
    {
    case 'h':
    case 't':
    case 'j':
    case 'm':
      setIntResult(result, (int64_t)UARGV(0, i), i);
      break;
    case 'c':
    case 's':
    case 'i':
    case 'l':
      setIntResult(result, SARGV(0, i), i);
      break;
    case 'f':
    case 'd':
      setIntResult(result, (int64_t)FARGV(0, i), i);
      break;
    default:
      assert(false);
    }
  }
}

DEFINE_BUILTIN(printf_builtin)
{
  const llvm::ConstantExpr *formatExpr = (llvm::ConstantExpr*)ARG(0);
  TypedValue formatPtrData = workItem->resolveConstExpr(formatExpr);
  size_t formatPtr = *(size_t*)formatPtrData.data;

  int arg = 1;
  while (true)
  {
    char c;
    workItem->m_globalMemory.load((unsigned char*)&c, formatPtr++);
    if (c == '\0')
    {
      break;
    }

    if (c == '%')
    {
      string format = "%";
      while (true)
      {
        workItem->m_globalMemory.load((unsigned char*)&c, formatPtr++);
        if (c == '\0')
        {
          cout << format;
          break;
        }

        format += c;
        bool done = false;
        switch (c)
        {
          case 'd':
          case 'i':
            printf(format.c_str(), SARG(arg++));
            done = true;
            break;
          case 'o':
          case 'u':
          case 'x':
          case 'X':
            printf(format.c_str(), UARG(arg++));
            done = true;
            break;
          case 'f':
          case 'F':
          case 'e':
          case 'E':
          case 'g':
          case 'G':
          case 'a':
          case 'A':
            printf(format.c_str(), FARG(arg++));
            done = true;
            break;
          case '%':
            printf("%%");
            done = true;
            break;
        }
        if (done)
        {
          break;
        }
      }
      if (c == '\0')
      {
        break;
      }
    }
    else
    {
      cout << c;
    }
  }
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
  workItem->updateVariable((const llvm::DbgValueInst*)callInst);
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
  const llvm::MemCpyInst *cpyInst = (const llvm::MemCpyInst*)callInst;
  size_t dest = *(size_t*)(workItem->m_privateMemory[cpyInst->getDest()].data);
  size_t src = *(size_t*)(workItem->m_privateMemory[cpyInst->getSource()].data);
  size_t size = workItem->getUnsignedInt(cpyInst->getLength());
  unsigned destAddrSpace = cpyInst->getDestAddressSpace();
  unsigned srcAddrSpace = cpyInst->getSourceAddressSpace();

  Memory *destMemory = NULL;
  switch (destAddrSpace)
  {
  case AddrSpacePrivate:
    destMemory = workItem->m_stack;
    break;
  case AddrSpaceGlobal:
  case AddrSpaceConstant:
    destMemory = &workItem->m_globalMemory;
    break;
  case AddrSpaceLocal:
    destMemory = workItem->m_workGroup.getLocalMemory();
    break;
  default:
    cerr << "Unhandled address space '" << destAddrSpace << "'" << endl;
    break;
  }

  Memory *srcMemory = NULL;
  switch (srcAddrSpace)
  {
  case AddrSpacePrivate:
    srcMemory = workItem->m_stack;
    break;
  case AddrSpaceGlobal:
  case AddrSpaceConstant:
    srcMemory = &workItem->m_globalMemory;
    break;
  case AddrSpaceLocal:
    srcMemory = workItem->m_workGroup.getLocalMemory();
    break;
  default:
    cerr << "Unhandled address space '" << srcAddrSpace << "'" << endl;
    break;
  }

  unsigned char *buffer = new unsigned char[size];
  if (!srcMemory->load(buffer, src, size))
  {
    workItem->outputMemoryError(*callInst, "Invalid read",
                          srcAddrSpace, src, size);
  }
  else if (!destMemory->store(buffer, dest, size))
  {
    workItem->outputMemoryError(*callInst, "Invalid write",
                          destAddrSpace, dest, size);
  }
  delete[] buffer;
}

DEFINE_BUILTIN(llvm_memset)
{
  const llvm::MemSetInst *setInst = (const llvm::MemSetInst*)callInst;
  size_t dest = *(size_t*)(workItem->m_privateMemory[setInst->getDest()].data);
  size_t size = workItem->getUnsignedInt(setInst->getLength());
  unsigned addressSpace = setInst->getDestAddressSpace();

  Memory *mem = NULL;
  switch (addressSpace)
  {
  case AddrSpacePrivate:
    mem = workItem->m_stack;
    break;
  case AddrSpaceGlobal:
  case AddrSpaceConstant:
    mem = &workItem->m_globalMemory;
    break;
  case AddrSpaceLocal:
    mem = workItem->m_workGroup.getLocalMemory();
    break;
  default:
    cerr << "Unhandled address space '" << addressSpace << "'" << endl;
    break;
  }

  unsigned char *buffer = new unsigned char[size];
  unsigned char value = workItem->getUnsignedInt(setInst->getArgOperand(1));
  memset(buffer, value, size);
  if (!mem->store(buffer, dest, size))
  {
    workItem->outputMemoryError(*callInst, "Invalid write",
                          addressSpace, dest, size);
  }
  delete[] buffer;
}

DEFINE_BUILTIN(llvm_trap)
{
  workItem->trap();
}

// Generate builtin function map
#define BUILTIN_CAST                        \
  void(*)(WorkItem*, const llvm::CallInst*, \
  std::string, std::string, TypedValue& result, void*)
#define F1ARG(name) (double(*)(double))name
#define F2ARG(name) (double(*)(double,double))name
#define ADD_BUILTIN(name, func, op)         \
  builtins[name] = BuiltinFunction((BUILTIN_CAST)func, (void*)op);
#define ADD_PREFIX_BUILTIN(name, func, op)  \
  prefixBuiltins.push_back(                 \
    make_pair(name, BuiltinFunction((BUILTIN_CAST)func, (void*)op)));
list< pair<string, WorkItem::BuiltinFunction> > WorkItem::prefixBuiltins;
map<string, WorkItem::BuiltinFunction> WorkItem::builtins = initBuiltins();
map<string, WorkItem::BuiltinFunction> WorkItem::initBuiltins()
{
  map<string, BuiltinFunction> builtins;

  // Async Copy and Prefetch Functions
  ADD_BUILTIN("async_work_group_copy", async_work_group_copy, NULL);
  ADD_BUILTIN("async_work_group_strided_copy", async_work_group_copy, NULL);
  ADD_BUILTIN("wait_group_events", wait_group_events, NULL);
  ADD_BUILTIN("prefetch", prefetch, NULL);

  // Common Functions
  ADD_BUILTIN("clamp", clamp, NULL);
  ADD_BUILTIN("degrees", builtin_f1arg, degrees);
  ADD_BUILTIN("max", max, NULL);
  ADD_BUILTIN("min", min, NULL);
  ADD_BUILTIN("mix", builtin_f3arg, mix);
  ADD_BUILTIN("radians", builtin_f1arg, radians);
  ADD_BUILTIN("sign", builtin_f1arg, sign);
  ADD_BUILTIN("smoothstep", builtin_f3arg, smoothstep);
  ADD_BUILTIN("step", builtin_f2arg, step_builtin);

  // Geometric Functions
  ADD_BUILTIN("cross", cross, NULL);
  ADD_BUILTIN("dot", dot, NULL);
  ADD_BUILTIN("distance", distance, NULL);
  ADD_BUILTIN("length", length, NULL);
  ADD_BUILTIN("normalize", normalize, NULL);
  ADD_BUILTIN("fast_distance", distance, NULL);
  ADD_BUILTIN("fast_length", length, NULL);
  ADD_BUILTIN("fast_normalize", normalize, NULL);

  // Integer Functions
  ADD_BUILTIN("abs", abs_builtin, NULL);
  ADD_BUILTIN("abs_diff", abs_diff, NULL);
  ADD_BUILTIN("add_sat", add_sat, NULL);
  ADD_BUILTIN("clz", clz, NULL);
  ADD_BUILTIN("hadd", hadd, NULL);
  ADD_BUILTIN("mad24", builtin_u3arg, mad);
  ADD_BUILTIN("mad_hi", mad_hi, NULL);
  ADD_BUILTIN("mad_sat", mad_sat, NULL);
  ADD_BUILTIN("mul24", builtin_u2arg, mul_builtin);
  ADD_BUILTIN("mul_hi", mul_hi, NULL);
  ADD_BUILTIN("popcount", builtin_u1arg, popcount);
  ADD_BUILTIN("rhadd", rhadd, NULL);
  ADD_BUILTIN("rotate", rotate, NULL);
  ADD_BUILTIN("sub_sat", sub_sat, NULL);
  ADD_BUILTIN("upsample", upsample, NULL);

  // Math Functions
  ADD_BUILTIN("acos", builtin_f1arg, F1ARG(acos));
  ADD_BUILTIN("acosh", builtin_f1arg, acosh);
  ADD_BUILTIN("acospi", builtin_f1arg, acospi);
  ADD_BUILTIN("asin", builtin_f1arg, F1ARG(asin));
  ADD_BUILTIN("asinh", builtin_f1arg, asinh);
  ADD_BUILTIN("asinpi", builtin_f1arg, asinpi);
  ADD_BUILTIN("atan", builtin_f1arg, F1ARG(atan));
  ADD_BUILTIN("atan2", builtin_f2arg, F2ARG(atan2));
  ADD_BUILTIN("atanh", builtin_f1arg, atanh);
  ADD_BUILTIN("atanpi", builtin_f1arg, atanpi);
  ADD_BUILTIN("atan2pi", builtin_f2arg, atan2pi);
  ADD_BUILTIN("cbrt", builtin_f1arg, cbrt);
  ADD_BUILTIN("ceil", builtin_f1arg, F1ARG(ceil));
  ADD_BUILTIN("copysign", builtin_f2arg, copysign);
  ADD_BUILTIN("cos", builtin_f1arg, F1ARG(cos));
  ADD_BUILTIN("cosh", builtin_f1arg, F1ARG(cosh));
  ADD_BUILTIN("cospi", builtin_f1arg, cospi);
  ADD_BUILTIN("erfc", builtin_f1arg, erfc);
  ADD_BUILTIN("erf", builtin_f1arg, erf);
  ADD_BUILTIN("exp", builtin_f1arg, F1ARG(exp));
  ADD_BUILTIN("exp2", builtin_f1arg, exp2);
  ADD_BUILTIN("exp10", builtin_f1arg, exp10);
  ADD_BUILTIN("expm1", builtin_f1arg, expm1);
  ADD_BUILTIN("fabs", builtin_f1arg, F1ARG(fabs));
  ADD_BUILTIN("fdim", builtin_f2arg, fdim);
  ADD_BUILTIN("floor", builtin_f1arg, F1ARG(floor));
  ADD_BUILTIN("fma", builtin_f3arg, fma);
  ADD_BUILTIN("fmax", builtin_f2arg, fmax);
  ADD_BUILTIN("fmin", builtin_f2arg, fmin);
  ADD_BUILTIN("fmod", builtin_f2arg, F2ARG(fmod));
  ADD_BUILTIN("fract", fract, NULL);
  ADD_BUILTIN("frexp", frexp_builtin, NULL);
  ADD_BUILTIN("hypot", builtin_f2arg, hypot);
  ADD_BUILTIN("ilogb", ilogb_builtin, NULL);
  ADD_BUILTIN("ldexp", ldexp_builtin, NULL);
  ADD_BUILTIN("lgamma", builtin_f1arg, lgamma);
  ADD_BUILTIN("lgamma_r", lgamma_r, NULL);
  ADD_BUILTIN("log", builtin_f1arg, F1ARG(log));
  ADD_BUILTIN("log2", builtin_f1arg, F1ARG(log2));
  ADD_BUILTIN("log10", builtin_f1arg, F1ARG(log10));
  ADD_BUILTIN("log1p", builtin_f1arg, log1p);
  ADD_BUILTIN("logb", builtin_f1arg, logb);
  ADD_BUILTIN("mad", builtin_f3arg, fma);
  ADD_BUILTIN("maxmag", builtin_f2arg, maxmag);
  ADD_BUILTIN("minmag", builtin_f2arg, minmag);
  ADD_BUILTIN("modf", modf_builtin, NULL);
  ADD_BUILTIN("nan", nan_builtin, NULL);
  ADD_BUILTIN("nextafter", builtin_f2arg, nextafter);
  ADD_BUILTIN("pow", builtin_f2arg, F2ARG(pow));
  ADD_BUILTIN("pown", pown, NULL);
  ADD_BUILTIN("powr", builtin_f2arg, F2ARG(pow));
  ADD_BUILTIN("remainder", builtin_f2arg, remainder);
  ADD_BUILTIN("remquo", remquo_builtin, NULL);
  ADD_BUILTIN("rint", builtin_f1arg, rint);
  ADD_BUILTIN("rootn", rootn, NULL);
  ADD_BUILTIN("round", builtin_f1arg, round);
  ADD_BUILTIN("rsqrt", builtin_f1arg, rsqrt);
  ADD_BUILTIN("sin", builtin_f1arg, F1ARG(sin));
  ADD_BUILTIN("sinh", builtin_f1arg, F1ARG(sinh));
  ADD_BUILTIN("sinpi", builtin_f1arg, sinpi);
  ADD_BUILTIN("sincos", sincos, NULL);
  ADD_BUILTIN("sqrt", builtin_f1arg, F1ARG(sqrt));
  ADD_BUILTIN("tan", builtin_f1arg, F1ARG(tan));
  ADD_BUILTIN("tanh", builtin_f1arg, F1ARG(tanh));
  ADD_BUILTIN("tanpi", builtin_f1arg, tanpi);
  ADD_BUILTIN("tgamma", builtin_f1arg, tgamma);
  ADD_BUILTIN("trunc", builtin_f1arg, trunc);

  // Native Math Functions
  ADD_BUILTIN("half_cos", builtin_f1arg, F1ARG(cos));
  ADD_BUILTIN("native_cos", builtin_f1arg, F1ARG(cos));
  ADD_BUILTIN("half_divide", builtin_f2arg, fdivide);
  ADD_BUILTIN("native_divide", builtin_f2arg, fdivide);
  ADD_BUILTIN("half_exp", builtin_f1arg, F1ARG(exp));
  ADD_BUILTIN("native_exp", builtin_f1arg, F1ARG(exp));
  ADD_BUILTIN("half_exp2", builtin_f1arg, exp2);
  ADD_BUILTIN("native_exp2", builtin_f1arg, exp2);
  ADD_BUILTIN("half_exp10", builtin_f1arg, exp10);
  ADD_BUILTIN("native_exp10", builtin_f1arg, exp10);
  ADD_BUILTIN("half_log", builtin_f1arg, F1ARG(log));
  ADD_BUILTIN("native_log", builtin_f1arg, F1ARG(log));
  ADD_BUILTIN("half_log2", builtin_f1arg, log2);
  ADD_BUILTIN("native_log2", builtin_f1arg, log2);
  ADD_BUILTIN("half_log10", builtin_f1arg, F1ARG(log10));
  ADD_BUILTIN("native_log10", builtin_f1arg, F1ARG(log10));
  ADD_BUILTIN("half_powr", builtin_f2arg, F2ARG(pow));
  ADD_BUILTIN("native_powr", builtin_f2arg, F2ARG(pow));
  ADD_BUILTIN("half_recip", builtin_f1arg, frecip);
  ADD_BUILTIN("native_recip", builtin_f1arg, frecip);
  ADD_BUILTIN("half_rsqrt", builtin_f1arg, rsqrt);
  ADD_BUILTIN("native_rsqrt", builtin_f1arg, rsqrt);
  ADD_BUILTIN("half_sin", builtin_f1arg, F1ARG(sin));
  ADD_BUILTIN("native_sin", builtin_f1arg, F1ARG(sin));
  ADD_BUILTIN("half_sqrt", builtin_f1arg, F1ARG(sqrt));
  ADD_BUILTIN("native_sqrt", builtin_f1arg, F1ARG(sqrt));
  ADD_BUILTIN("half_tan", builtin_f1arg, F1ARG(tan));
  ADD_BUILTIN("native_tan", builtin_f1arg, F1ARG(tan));

  // Misc. Vector Functions
  ADD_BUILTIN("shuffle", shuffle_builtin, NULL);
  ADD_BUILTIN("shuffle2", shuffle2_builtin, NULL);

  // Relational Functional
  ADD_BUILTIN("all", all, NULL);
  ADD_BUILTIN("any", any, NULL);
  ADD_BUILTIN("bitselect", bitselect, NULL);
  ADD_BUILTIN("isequal", builtin_rel2arg, isequal_builtin);
  ADD_BUILTIN("isnotequal", builtin_rel2arg, isnotequal_builtin);
  ADD_BUILTIN("isgreater", builtin_rel2arg, isgreater_builtin);
  ADD_BUILTIN("isgreaterequal", builtin_rel2arg, isgreaterequal_builtin);
  ADD_BUILTIN("isless", builtin_rel2arg, isless_builtin);
  ADD_BUILTIN("islessequal", builtin_rel2arg, islessequal_builtin);
  ADD_BUILTIN("islessgreater", builtin_rel2arg, islessgreater_builtin);
  ADD_BUILTIN("isfinite", builtin_rel1arg, isfinite_builtin);
  ADD_BUILTIN("isinf", builtin_rel1arg, isinf_builtin);
  ADD_BUILTIN("isnan", builtin_rel1arg, isnan_builtin);
  ADD_BUILTIN("isnormal", builtin_rel1arg, isnormal_builtin);
  ADD_BUILTIN("isordered", builtin_rel2arg, isordered_builtin);
  ADD_BUILTIN("isunordered", builtin_rel2arg, isunordered_builtin);
  ADD_BUILTIN("select", select_builtin, NULL);
  ADD_BUILTIN("signbit", builtin_rel1arg, signbit_builtin);

  // Synchronization Functions
  ADD_BUILTIN("barrier", barrier, NULL);
  ADD_BUILTIN("mem_fence", mem_fence, NULL);
  ADD_BUILTIN("read_mem_fence", mem_fence, NULL);
  ADD_BUILTIN("write_mem_fence", mem_fence, NULL);

  // Vector Data Load and Store Functions
  ADD_PREFIX_BUILTIN("vload", vload, NULL);
  ADD_PREFIX_BUILTIN("vstore", vstore, NULL);

  // Work-Item Functions
  ADD_BUILTIN("get_global_id", get_global_id, NULL);
  ADD_BUILTIN("get_global_size", get_global_size, NULL);
  ADD_BUILTIN("get_global_offset", get_global_offset, NULL);
  ADD_BUILTIN("get_group_id", get_group_id, NULL);
  ADD_BUILTIN("get_local_id", get_local_id, NULL);
  ADD_BUILTIN("get_local_size", get_local_size, NULL);
  ADD_BUILTIN("get_num_groups", get_num_groups, NULL);
  ADD_BUILTIN("get_work_dim", get_work_dim, NULL);

  // Other Functions
  ADD_PREFIX_BUILTIN("convert_float",  convert_float, NULL); // Floats
  ADD_PREFIX_BUILTIN("convert_double", convert_float, NULL); // Doubles
  ADD_PREFIX_BUILTIN("convert_u",      convert_uint, NULL);  // Unsigned integers
  ADD_PREFIX_BUILTIN("convert_",       convert_sint, NULL);  // Signed integers
  ADD_BUILTIN("printf", printf_builtin, NULL);

  // LLVM Intrinsics
  ADD_BUILTIN("llvm.dbg.declare", llvm_dbg_declare, NULL);
  ADD_BUILTIN("llvm.dbg.value", llvm_dbg_value, NULL);
  ADD_BUILTIN("llvm.lifetime.start", llvm_lifetime_start, NULL);
  ADD_BUILTIN("llvm.lifetime.end", llvm_lifetime_end, NULL);
  ADD_PREFIX_BUILTIN("llvm.memcpy", llvm_memcpy, NULL);
  ADD_PREFIX_BUILTIN("llvm.memset", llvm_memset, NULL);
  ADD_BUILTIN("llvm.trap", llvm_trap, NULL);

  return builtins;
}
