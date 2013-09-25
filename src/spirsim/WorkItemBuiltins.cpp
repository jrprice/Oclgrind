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

void WorkItem::builtin_rel1arg(const llvm::CallInst *callInst,
                             TypedValue& result,
                             int (*func)(double))
{
  int t = result.num > 1 ? -1 : 1;
  for (int i = 0; i < result.num; i++)
  {
    setIntResult(result, (int64_t)func(FARGV(0, i))*t, i);
  }
}

void WorkItem::builtin_rel2arg(const llvm::CallInst *callInst,
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
  switch (getOverloadArgType(overload))
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
  switch (getOverloadArgType(overload))
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
      setIntResult(result, umul_hi(UARGV(0, i), UARGV(1, i), result.size<<3) + UARGV(2, i), i);
      break;
    case 'c':
    case 's':
    case 'i':
    case 'l':
      setIntResult(result, smul_hi(SARGV(0, i), SARGV(1, i), result.size<<3) + SARGV(2, i), i);
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
      builtin_f3arg(callInst, result, fbitselect);
      break;
    case 'h':
    case 't':
    case 'j':
    case 'm':
    case 'c':
    case 's':
    case 'i':
    case 'l':
      builtin_u3arg(callInst, result, ibitselect);
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
	m_state = BARRIER;
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
  TypedValue formatPtrData = resolveConstExpr(formatExpr);
  size_t formatPtr = *(size_t*)formatPtrData.data;

  int arg = 1;
  while (true)
  {
    char c;
    m_globalMemory.load((unsigned char*)&c, formatPtr++);
    if (c == '\0')
    {
      break;
    }

    if (c == '%')
    {
      string format = "%";
      while (true)
      {
        m_globalMemory.load((unsigned char*)&c, formatPtr++);
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
