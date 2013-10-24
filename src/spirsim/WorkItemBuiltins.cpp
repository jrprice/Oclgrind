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
#include <fenv.h>

#pragma STDC FENV_ACCESS ON

#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Metadata.h"

#include "Device.h"
#include "Memory.h"
#include "WorkGroup.h"
#include "WorkItem.h"

using namespace spirsim;
using namespace std;

namespace spirsim
{
  class WorkItemBuiltins
  {
    // Utility macros for creating builtins
#define DEFINE_BUILTIN(name)                                           \
  static void name(WorkItem *workItem, const llvm::CallInst *callInst, \
                   std::string fnName, std::string overload,           \
                   TypedValue& result, void *)
#define ARG(i) (callInst->getArgOperand(i))
#define UARG(i) workItem->getUnsignedInt(callInst->getArgOperand(i))
#define SARG(i) workItem->getSignedInt(callInst->getArgOperand(i))
#define FARG(i) workItem->getFloatValue(callInst->getArgOperand(i))
#define UARGV(i,v) workItem->getUnsignedInt(callInst->getArgOperand(i), v)
#define SARGV(i,v) workItem->getSignedInt(callInst->getArgOperand(i), v)
#define FARGV(i,v) workItem->getFloatValue(callInst->getArgOperand(i), v)

    // Functions that apply generic builtins to each component of a vector
    static void f1arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      string name, string overload, TypedValue& result,
                      double (*func)(double))
    {
      for (int i = 0; i < result.num; i++)
      {
        double r = func(FARGV(0, i));
        WorkItem::setFloatResult(result, r, i);
      }
    }
    static void f2arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      string name, string overload, TypedValue& result,
                      double (*func)(double, double))
    {
      for (int i = 0; i < result.num; i++)
      {
        double r = func(FARGV(0, i), FARGV(1, i));
        WorkItem::setFloatResult(result, r, i);
      }
    }
    static void f3arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      string name, string overload, TypedValue& result,
                      double (*func)(double, double, double))
    {
      for (int i = 0; i < result.num; i++)
      {
        double r = func(FARGV(0, i), FARGV(1, i), FARGV(2, i));
        WorkItem::setFloatResult(result, r, i);
      }
    }
    static void u1arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      string name, string overload, TypedValue& result,
                      uint64_t (*func)(uint64_t))
    {
      for (int i = 0; i < result.num; i++)
      {
        uint64_t r = func(UARGV(0, i));
        WorkItem::setIntResult(result, r, i);
      }
    }
    static void u2arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      string name, string overload, TypedValue& result,
                      uint64_t (*func)(uint64_t, uint64_t))
    {
      for (int i = 0; i < result.num; i++)
      {
        uint64_t r = func(UARGV(0, i), UARGV(1, i));
        WorkItem::setIntResult(result, r, i);
      }
    }
    static void u3arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      string name, string overload, TypedValue& result,
                      uint64_t (*func)(uint64_t, uint64_t, uint64_t))
    {
      for (int i = 0; i < result.num; i++)
      {
        uint64_t r = func(UARGV(0, i), UARGV(1, i), UARGV(2, i));
        WorkItem::setIntResult(result, r, i);
      }
    }
    static void s1arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      string name, string overload, TypedValue& result,
                      int64_t (*func)(int64_t))
    {
      for (int i = 0; i < result.num; i++)
      {
        int64_t r = func(SARGV(0, i));
        WorkItem::setIntResult(result, r, i);
      }
    }
    static void s2arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      string name, string overload, TypedValue& result,
                      int64_t (*func)(int64_t, int64_t))
    {
      for (int i = 0; i < result.num; i++)
      {
        int64_t r = func(SARGV(0, i), SARGV(1, i));
        WorkItem::setIntResult(result, r, i);
      }
    }
    static void s3arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      string name, string overload, TypedValue& result,
                      int64_t (*func)(int64_t, int64_t, int64_t))
    {
      for (int i = 0; i < result.num; i++)
      {
        int64_t r = func(SARGV(0, i), SARGV(1, i), SARGV(2, i));
        WorkItem::setIntResult(result, r, i);
      }
    }
    static void rel1arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      string name, string overload, TypedValue& result,
                      int64_t (*func)(double))
    {
      int64_t t = result.num > 1 ? -1 : 1;
      for (int i = 0; i < result.num; i++)
      {
        int64_t r = func(FARGV(0, i))*t;
        WorkItem::setIntResult(result, r, i);
      }
    }
    static void rel2arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      string name, string overload, TypedValue& result,
                      int64_t (*func)(double, double))
    {
      int64_t t = result.num > 1 ? -1 : 1;
      for (int i = 0; i < result.num; i++)
      {
        int64_t r = func(FARGV(0, i), FARGV(1, i))*t;
        WorkItem::setIntResult(result, r, i);
      }
    }

    // Extract the (first) argument type from an overload string
    static char getOverloadArgType(string overload)
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
      size_t dest = *(size_t*)(workItem->m_instResults[destOp].data);
      size_t src = *(size_t*)(workItem->m_instResults[srcOp].data);

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
      WorkGroup::AsyncCopy copy =
      {
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
      WorkItem::setIntResult(result, event);
    }

    DEFINE_BUILTIN(wait_group_events)
    {
      uint64_t num = UARG(0);
      const llvm::Value *ptrOp = ARG(1);
      size_t address = *(size_t*)(workItem->m_instResults[ptrOp].data);
      for (int i = 0; i < num; i++)
      {
        // TODO: Can we safely assume this is private/stack data?
        uint64_t event;
        if (!workItem->m_privateMemory->load((unsigned char*)&event,
         address, sizeof(uint64_t)))
        {
          workItem->m_device->notifyMemoryError(true, AddrSpacePrivate,
                                                address, sizeof(uint64_t));
          return;
        }
        workItem->m_workGroup.wait_event(event);
        address += sizeof(uint64_t);
      }
      workItem->m_state = WorkItem::BARRIER;
      workItem->m_workGroup.notifyBarrier(workItem);
    }

    DEFINE_BUILTIN(prefetch)
    {
      // Do nothing.
    }


    //////////////////////
    // Atomic Functions //
    //////////////////////

    DEFINE_BUILTIN(atomic_add)
    {
      // Get address
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());
      size_t ptr = UARG(0);

      // Load old value
      uint32_t old;
      memory->load((unsigned char*)&old, ptr, 4);

      // Store old value in result
      workItem->setIntResult(result, (uint64_t)old);

      // Compute and store new value
      old += UARG(1);
      memory->store((unsigned char*)&old, ptr, 4);
    }

    DEFINE_BUILTIN(atomic_and)
    {
      // Get address
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());
      size_t ptr = UARG(0);

      // Load old value
      uint32_t old;
      memory->load((unsigned char*)&old, ptr, 4);

      // Store old value in result
      workItem->setIntResult(result, (uint64_t)old);

      // Compute and store new value
      old &= UARG(1);
      memory->store((unsigned char*)&old, ptr, 4);
    }

    DEFINE_BUILTIN(atomic_cmpxchg)
    {
      // Get address
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());
      size_t ptr = UARG(0);

      // Load old value
      uint32_t old;
      memory->load((unsigned char*)&old, ptr, 4);

      // Store old value in result
      workItem->setIntResult(result, (uint64_t)old);

      // Compute and store new value
      if (old == UARG(1))
      {
        old = UARG(2);
      }
      memory->store((unsigned char*)&old, ptr, 4);
    }

    DEFINE_BUILTIN(atomic_dec)
    {
      // Get address
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());
      size_t ptr = UARG(0);

      // Load old value
      uint32_t old;
      memory->load((unsigned char*)&old, ptr, 4);

      // Store old value in result
      workItem->setIntResult(result, (uint64_t)old);

      // Compute and store new value
      old--;
      memory->store((unsigned char*)&old, ptr, 4);
    }

    DEFINE_BUILTIN(atomic_inc)
    {
      // Get address
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());
      size_t ptr = UARG(0);

      // Load old value
      uint32_t old;
      memory->load((unsigned char*)&old, ptr, 4);

      // Store old value in result
      workItem->setIntResult(result, (uint64_t)old);

      // Compute and store new value
      old++;
      memory->store((unsigned char*)&old, ptr, 4);
    }

    DEFINE_BUILTIN(atomic_max)
    {
      // Get address
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());
      size_t ptr = UARG(0);

      // Load old value
      uint32_t old;
      memory->load((unsigned char*)&old, ptr, 4);

      // Store old value in result
      workItem->setIntResult(result, (uint64_t)old);

      // Compute and store new value
      uint32_t val = UARG(1);
      old = old > val ? old : val;
      memory->store((unsigned char*)&old, ptr, 4);
    }

    DEFINE_BUILTIN(atomic_min)
    {
      // Get address
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());
      size_t ptr = UARG(0);

      // Load old value
      uint32_t old;
      memory->load((unsigned char*)&old, ptr, 4);

      // Store old value in result
      workItem->setIntResult(result, (uint64_t)old);

      // Compute and store new value
      uint32_t val = UARG(1);
      old = old < val ? old : val;
      memory->store((unsigned char*)&old, ptr, 4);
    }

    DEFINE_BUILTIN(atomic_or)
    {
      // Get address
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());
      size_t ptr = UARG(0);

      // Load old value
      uint32_t old;
      memory->load((unsigned char*)&old, ptr, 4);

      // Store old value in result
      workItem->setIntResult(result, (uint64_t)old);

      // Compute and store new value
      old |= UARG(1);
      memory->store((unsigned char*)&old, ptr, 4);
    }

    DEFINE_BUILTIN(atomic_sub)
    {
      // Get address
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());
      size_t ptr = UARG(0);

      // Load old value
      uint32_t old;
      memory->load((unsigned char*)&old, ptr, 4);

      // Store old value in result
      workItem->setIntResult(result, (uint64_t)old);

      // Compute and store new value
      old -= UARG(1);
      memory->store((unsigned char*)&old, ptr, 4);
    }

    DEFINE_BUILTIN(atomic_xchg)
    {
      // Get address
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());
      size_t ptr = UARG(0);

      // Load old value
      uint32_t old;
      memory->load((unsigned char*)&old, ptr, 4);

      // Store old value in result
      workItem->setIntResult(result, (uint64_t)old);

      // Compute and store new value
      old = UARG(1);
      memory->store((unsigned char*)&old, ptr, 4);
    }

    DEFINE_BUILTIN(atomic_xor)
    {
      // Get address
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());
      size_t ptr = UARG(0);

      // Load old value
      uint32_t old;
      memory->load((unsigned char*)&old, ptr, 4);

      // Store old value in result
      workItem->setIntResult(result, (uint64_t)old);

      // Compute and store new value
      old ^= UARG(1);
      memory->store((unsigned char*)&old, ptr, 4);
    }


    //////////////////////
    // Common Functions //
    //////////////////////

    template<typename T> T static _max_(T a, T b){return a > b ? a : b;}
    template<typename T> T static _min_(T a, T b){return a < b ? a : b;}
    template<typename T> T static _clamp_(T x, T min, T max)
    {
      return _min_(_max_(x, min), max);
    }

    static double _degrees_(double x)
    {
      return x * (180 / M_PI);
    }

    static double _mix_(double x, double y, double a)
    {
      return x + (y - x) * a;
    }

    static double _radians_(double x)
    {
      return x * (M_PI / 180);
    }

    static double _sign_(double x)
    {
      if (isnan(x))  return  0.0;
      if (x  >  0.0) return  1.0;
      if (x == -0.0) return -0.0;
      if (x ==  0.0) return  0.0;
      if (x  <  0.0) return -1.0;
      return 0.0;
    }

    static double _smoothstep_(double edge0, double edge1, double x)
    {
      double t = _clamp_<double>((x - edge0) / (edge1 - edge0), 0, 1);
      return t * t * (3 - 2*t);
    }
    static double _step_(double edge, double x)
    {
      return (x < edge) ? 0.0 : 1.0;
    }

    DEFINE_BUILTIN(clamp)
    {
      switch (getOverloadArgType(overload))
      {
        case 'f':
        case 'd':
          f3arg(workItem, callInst, fnName, overload, result, _clamp_);
          break;
        case 'h':
        case 't':
        case 'j':
        case 'm':
          u3arg(workItem, callInst, fnName, overload, result, _clamp_);
          break;
        case 'c':
        case 's':
        case 'i':
        case 'l':
          s3arg(workItem, callInst, fnName, overload, result, _clamp_);
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
          f2arg(workItem, callInst, fnName, overload, result, fmax);
          break;
        case 'h':
        case 't':
        case 'j':
        case 'm':
          u2arg(workItem, callInst, fnName, overload, result, _max_);
          break;
        case 'c':
        case 's':
        case 'i':
        case 'l':
          s2arg(workItem, callInst, fnName, overload, result, _max_);
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
          f2arg(workItem, callInst, fnName, overload, result, fmin);
          break;
        case 'h':
        case 't':
        case 'j':
        case 'm':
          u2arg(workItem, callInst, fnName, overload, result, _min_);
          break;
        case 'c':
        case 's':
        case 'i':
        case 'l':
          s2arg(workItem, callInst, fnName, overload, result, _min_);
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
      WorkItem::setFloatResult(result, u2*v3 - u3*v2, 0);
      WorkItem::setFloatResult(result, u3*v1 - u1*v3, 1);
      WorkItem::setFloatResult(result, u1*v2 - u2*v1, 2);
      WorkItem::setFloatResult(result, 0, 3);
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
      WorkItem::setFloatResult(result, r);
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
      WorkItem::setFloatResult(result, sqrt(distSq));
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
      WorkItem::setFloatResult(result, sqrt(lengthSq));
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
        WorkItem::setFloatResult(result, FARGV(0, i)/length, i);
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
            WorkItem::setIntResult(result, UARGV(0,i), i);
            break;
          case 'c':
          case 's':
          case 'i':
          case 'l':
            WorkItem::setIntResult(result, abs(SARGV(0,i)), i);
            break;
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
            WorkItem::setIntResult(result, _max_(a,b) - _min_(a,b), i);
            break;
          }
          case 'c':
          case 's':
          case 'i':
          case 'l':
          {
            int64_t a = SARGV(0, i);
            int64_t b = SARGV(1, i);
            WorkItem::setIntResult(result, _max_(a,b) - _min_(a,b), i);
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
            uresult = _min_<uint64_t>(uresult, UINT8_MAX);
            WorkItem::setIntResult(result, uresult, i);
            break;
          case 't':
            uresult = _min_<uint64_t>(uresult, UINT16_MAX);
            WorkItem::setIntResult(result, uresult, i);
            break;
          case 'j':
            uresult = _min_<uint64_t>(uresult, UINT32_MAX);
            WorkItem::setIntResult(result, uresult, i);
            break;
          case 'm':
            uresult = (UARGV(1, i) > uresult) ? UINT64_MAX : uresult;
            WorkItem::setIntResult(result, uresult, i);
            break;
          case 'c':
            sresult = _clamp_<int64_t>(sresult, INT8_MIN, INT8_MAX);
            WorkItem::setIntResult(result, sresult, i);
            break;
          case 's':
            sresult = _clamp_<int64_t>(sresult, INT16_MIN, INT16_MAX);
            WorkItem::setIntResult(result, sresult, i);
            break;
          case 'i':
            sresult = _clamp_<int64_t>(sresult, INT32_MIN, INT32_MAX);
            WorkItem::setIntResult(result, sresult, i);
            break;
          case 'l':
            if ((SARGV(0,i)>0) == (SARGV(1,i)>0) &&
                (SARGV(0,i)>0) != (sresult>0))
            {
              sresult = (SARGV(0,i)>0) ? INT64_MAX : INT64_MIN;
            }
            WorkItem::setIntResult(result, sresult, i);
            break;
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

        uint64_t r = ((result.size<<3) - nz);
        WorkItem::setIntResult(result, r, i);
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
            WorkItem::setIntResult(result, ((a + b) >> 1) | c, i);
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
            WorkItem::setIntResult(result, (a>>1) + (b>>1) + c, i);
            break;
          }
          default:
            assert(false);
        }
      }
    }

    static uint64_t _mad_(uint64_t a, uint64_t b, uint64_t c)
    {
      return a*b + c;
    }

    static uint64_t _umul_hi_(uint64_t x, uint64_t y, uint64_t bits)
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

    static int64_t _smul_hi_(int64_t x, int64_t y, int64_t bits)
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
          {
            uint64_t r =
              _umul_hi_(UARGV(0, i), UARGV(1, i), result.size<<3) + UARGV(2, i);
            WorkItem::setIntResult(result, r, i);
            break;
          }
          case 'c':
          case 's':
          case 'i':
          case 'l':
          {
            int64_t r =
              _smul_hi_(SARGV(0, i), SARGV(1, i), result.size<<3) + SARGV(2, i);
            WorkItem::setIntResult(result, r, i);
            break;
          }
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
            uresult = _min_<uint64_t>(uresult, UINT8_MAX);
            WorkItem::setIntResult(result, uresult, i);
            break;
          case 't':
            uresult = _min_<uint64_t>(uresult, UINT16_MAX);
            WorkItem::setIntResult(result, uresult, i);
            break;
          case 'j':
            uresult = _min_<uint64_t>(uresult, UINT16_MAX);
            WorkItem::setIntResult(result, uresult, i);
            break;
          case 'm':
          {
            uint64_t hi = _umul_hi_(UARGV(0, i), UARGV(1, i), 64);
            if (hi || UARGV(2, i) > uresult)
            {
              uresult = UINT64_MAX;
            }
            WorkItem::setIntResult(result, uresult, i);
            break;
          }
          case 'c':
            sresult = _clamp_<int64_t>(sresult, INT8_MIN, INT8_MAX);
            WorkItem::setIntResult(result, sresult, i);
            break;
          case 's':
            sresult = _clamp_<int64_t>(sresult, INT16_MIN, INT16_MAX);
            WorkItem::setIntResult(result, sresult, i);
            break;
          case 'i':
            sresult = _clamp_<int64_t>(sresult, INT32_MIN, INT32_MAX);
            WorkItem::setIntResult(result, sresult, i);
            break;
          case 'l':
            // Check for overflow in multiplication
            if (_smul_hi_(SARGV(0, i), SARGV(1, i), 64))
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
            WorkItem::setIntResult(result, sresult, i);
            break;
          default:
            assert(false);
        }
      }
    }

    static uint64_t _mul_(uint64_t a, uint64_t b)
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
          {
            uint64_t r = _umul_hi_(UARGV(0, i), UARGV(1, i), result.size<<3);
            WorkItem::setIntResult(result, r, i);
            break;
          }
          case 'c':
          case 's':
          case 'i':
          case 'l':
          {
            int64_t r = _smul_hi_(SARGV(0, i), SARGV(1, i), result.size<<3);
            WorkItem::setIntResult(result, r, i);
            break;
          }
          default:
            assert(false);
        }
      }
    }

    static uint64_t _popcount_(uint64_t x)
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
            WorkItem::setIntResult(result, ((a + b + 1) >> 1) | c, i);
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
            WorkItem::setIntResult(result, (a>>1) + (b>>1) + c, i);
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
        WorkItem::setIntResult(result, (v << ls) | (v >> rs), i);
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
            uresult = uresult > UINT8_MAX ? 0 : uresult;
            WorkItem::setIntResult(result, uresult, i);
            break;
          case 't':
            uresult = uresult > UINT16_MAX ? 0 : uresult;
            WorkItem::setIntResult(result, uresult, i);
            break;
          case 'j':
            uresult = uresult > UINT32_MAX ? 0 : uresult;
            WorkItem::setIntResult(result, uresult, i);
            break;
          case 'm':
            uresult = (UARGV(1, i) > UARGV(0, i)) ? 0 : uresult;
            WorkItem::setIntResult(result, uresult, i);
            break;
          case 'c':
            sresult = _clamp_<int64_t>(sresult, INT8_MIN, INT8_MAX);
            WorkItem::setIntResult(result, sresult, i);
            break;
          case 's':
            sresult = _clamp_<int64_t>(sresult, INT16_MIN, INT16_MAX);
            WorkItem::setIntResult(result, sresult, i);
            break;
          case 'i':
            sresult = _clamp_<int64_t>(sresult, INT32_MIN, INT32_MAX);
            WorkItem::setIntResult(result, sresult, i);
            break;
          case 'l':
            if ((SARGV(0,i)>0) != (SARGV(1,i)>0) &&
                (SARGV(0,i)>0) != (sresult>0))
            {
              sresult = (SARGV(0,i)>0) ? INT64_MAX : INT64_MIN;
            }
            WorkItem::setIntResult(result, sresult, i);
            break;
          default:
            assert(false);
        }
      }
    }

    DEFINE_BUILTIN(upsample)
    {
      for (int i = 0; i < result.num; i++)
      {
        uint64_t r = (UARGV(0,i)<<(result.size<<2)) | UARGV(1, i);
        WorkItem::setIntResult(result, r, i);
      }
    }


    ////////////////////
    // Math Functions //
    ////////////////////

    static double _acospi_(double x){ return (acos(x) / M_PI); }
    static double _asinpi_(double x){ return (asin(x) / M_PI); }
    static double _atanpi_(double x){ return (atan(x) / M_PI); }
    static double _atan2pi_(double x, double y){ return (atan2(x, y) / M_PI); }
    static double _cospi_(double x){ return (cos(x * M_PI)); }
    static double _exp10_(double x){ return pow(10, x); }
    static double _fdivide_(double x, double y){ return x/y; }
    static double _frecip_(double x){ return 1.0/x; }
    static double _rsqrt_(double x){ return 1.0 / sqrt(x); }
    static double _sinpi_(double x){ return (sin(x * M_PI)); }
    static double _tanpi_(double x){ return (tan(x * M_PI)); }

    static double _maxmag_(double x, double y)
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

    static double _minmag_(double x, double y)
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
      Memory *memory =
        workItem->getMemory(ARG(1)->getType()->getPointerAddressSpace());

      size_t iptr = UARG(1);
      for (int i = 0; i < result.num; i++)
      {
        double x = FARGV(0, i);
        double fl = floor(x);
        double r = fmin(x - fl, 0x1.fffffep-1f);

        size_t offset = i*result.size;
        WorkItem::setFloatResult(result, fl, i);
        memory->store(result.data + offset, iptr + offset, result.size);
        WorkItem::setFloatResult(result, r, i);
      }
    }

    DEFINE_BUILTIN(frexp_builtin)
    {
      Memory *memory =
        workItem->getMemory(ARG(1)->getType()->getPointerAddressSpace());

      size_t iptr = UARG(1);
      for (int i = 0; i < result.num; i++)
      {
        int32_t e;
        double r = frexp(FARGV(0, i), &e);
        memory->store((const unsigned char*)&e, iptr + i*4, 4);
        WorkItem::setFloatResult(result, r, i);
      }
    }

    DEFINE_BUILTIN(ilogb_builtin)
    {
      for (int i = 0; i < result.num; i++)
      {
        WorkItem::setIntResult(result, (int64_t)ilogb(FARGV(0, i)), i);
      }
    }

    DEFINE_BUILTIN(ldexp_builtin)
    {
      for (int i = 0; i < result.num; i++)
      {
        WorkItem::setFloatResult(result, ldexp(FARGV(0, i), SARGV(1, i)), i);
      }
    }

    DEFINE_BUILTIN(lgamma_r)
    {
      Memory *memory =
        workItem->getMemory(ARG(1)->getType()->getPointerAddressSpace());

      size_t signp = UARG(1);
      for (int i = 0; i < result.num; i++)
      {
        double r = lgamma(FARGV(0, i));
        int32_t s = (tgamma(FARGV(0, i)) < 0 ? -1 : 1);
        memory->store((const unsigned char*)&s, signp + i*4, 4);
        WorkItem::setFloatResult(result, r, i);
      }
    }

    DEFINE_BUILTIN(modf_builtin)
    {
      Memory *memory =
        workItem->getMemory(ARG(1)->getType()->getPointerAddressSpace());

      size_t iptr = UARG(1);
      for (int i = 0; i < result.num; i++)
      {
        double x = FARGV(0, i);
        double integral = trunc(x);
        double fractional = copysign(isinf(x) ? 0.0 : x - integral, x);

        size_t offset = i*result.size;
        WorkItem::setFloatResult(result, integral, i);
        memory->store(result.data + offset, iptr + offset, result.size);
        WorkItem::setFloatResult(result, fractional, i);
      }
    }

    DEFINE_BUILTIN(nan_builtin)
    {
      for (int i = 0; i < result.num; i++)
      {
        uint64_t nancode = UARGV(0, i);
        WorkItem::setFloatResult(result, nan(""), i);
      }
    }

    DEFINE_BUILTIN(pown)
    {
      for (int i = 0; i < result.num; i++)
      {
        double x = FARGV(0, i);
        int32_t y = SARGV(1, i);
        WorkItem::setFloatResult(result, pow(x, y), i);
      }
    }

    DEFINE_BUILTIN(remquo_builtin)
    {
      Memory *memory =
        workItem->getMemory(ARG(2)->getType()->getPointerAddressSpace());

      size_t quop = UARG(2);
      for (int i = 0; i < result.num; i++)
      {
        double x = FARGV(0, i);
        double y = FARGV(1, i);

        int32_t quo;
        double rem = remquo(x, y, &quo);
        memory->store((const unsigned char*)&quo, quop + i*4, 4);
        WorkItem::setFloatResult(result, rem, i);
      }
    }

    DEFINE_BUILTIN(rootn)
    {
      for (int i = 0; i < result.num; i++)
      {
        double x = FARGV(0, i);
        int y = SARGV(1, i);
        WorkItem::setFloatResult(result, pow(x, (double)(1.0/y)), i);
      }
    }

    DEFINE_BUILTIN(sincos)
    {
      Memory *memory =
        workItem->getMemory(ARG(1)->getType()->getPointerAddressSpace());

      size_t cv = UARG(1);
      for (int i = 0; i < result.num; i++)
      {
        double x = FARGV(0, i);
        size_t offset = i*result.size;
        WorkItem::setFloatResult(result, cos(x), i);
        memory->store(result.data + offset, cv + offset, result.size);
        WorkItem::setFloatResult(result, sin(x), i);
      }
    }


    ////////////////////////////
    // Misc. Vector Functions //
    ////////////////////////////

    DEFINE_BUILTIN(shuffle_builtin)
    {
      for (int i = 0; i < result.num; i++)
      {
        WorkItem::setIntResult(result, UARGV(0, UARGV(1, i)), i);
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
        WorkItem::setIntResult(result, UARGV(src, index), i);
      }
    }


    //////////////////////////
    // Relational Functions //
    //////////////////////////

    static int64_t _iseq_(double x, double y){ return x == y; }
    static int64_t _isneq_(double x, double y){ return x != y; }
    static int64_t _isgt_(double x, double y){ return isgreater(x, y); }
    static int64_t _isge_(double x, double y){ return isgreaterequal(x, y); }
    static int64_t _islt_(double x, double y){ return isless(x, y); }
    static int64_t _isle_(double x, double y){ return islessequal(x, y); }
    static int64_t _islg_(double x, double y){ return islessgreater(x, y); }
    static int64_t _isfin_(double x){ return isfinite(x); }
    static int64_t _isinf_(double x){ return isinf(x); }
    static int64_t _isnan_(double x){ return isnan(x); }
    static int64_t _isnorm_(double x){ return isnormal(x); }
    static int64_t _isord_(double x, double y){ return !isunordered(x, y); }
    static int64_t _isuord_(double x, double y){ return isunordered(x, y); }
    static int64_t _signbit_(double x){ return signbit(x); }

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
          WorkItem::setIntResult(result, (int64_t)0);
          return;
        }
      }
      WorkItem::setIntResult(result, (int64_t)1);
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
          WorkItem::setIntResult(result, (int64_t)1);
          return;
        }
      }
      WorkItem::setIntResult(result, (int64_t)0);
    }

    static uint64_t _ibitselect_(uint64_t a, uint64_t b, uint64_t c)
    {
      return ((a & ~c) | (b & c));
    }

    static double _fbitselect_(double a, double b, double c)
    {
      uint64_t _a = *(uint64_t*)&a;
      uint64_t _b = *(uint64_t*)&b;
      uint64_t _c = *(uint64_t*)&c;
      uint64_t _r = _ibitselect_(_a, _b, _c);
      return *(double*)&_r;
    }

    DEFINE_BUILTIN(bitselect)
    {
      switch (getOverloadArgType(overload))
      {
        case 'f':
        case 'd':
          f3arg(workItem, callInst, fnName, overload, result, _fbitselect_);
          break;
        case 'h':
        case 't':
        case 'j':
        case 'm':
        case 'c':
        case 's':
        case 'i':
        case 'l':
          u3arg(workItem, callInst, fnName, overload, result, _ibitselect_);
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
            WorkItem::setFloatResult(result, _c ? FARGV(1, i) : FARGV(0, i), i);
            break;
          case 'h':
          case 't':
          case 'j':
          case 'm':
          case 'c':
          case 's':
          case 'i':
          case 'l':
            WorkItem::setIntResult(result, _c ? SARGV(1, i) : SARGV(0, i), i);
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
      workItem->m_state = WorkItem::BARRIER;
      workItem->m_workGroup.notifyBarrier(workItem);
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
      size_t base = *(size_t*)(workItem->m_instResults[ptrOp].data);
      uint64_t offset = UARG(0);

      string addrSpaceStr = overload.substr(overload.length()-2);
      unsigned int addressSpace = atoi(addrSpaceStr.c_str());

      size_t address = base + offset*result.size*result.num;
      size_t size = result.size*result.num;
      Memory *memory = workItem->getMemory(addressSpace);
      if (!memory->load(result.data, address, size))
      {
        workItem->m_device->notifyMemoryError(true, addressSpace,
                                              address, size);
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
        memcpy(data, workItem->m_instResults[value].data, size);
      }
      uint64_t offset = UARG(1);

      const llvm::Value *ptrOp = ARG(2);
      size_t base = *(size_t*)(workItem->m_instResults[ptrOp].data);

      string addrSpaceStr = overload.substr(overload.length()-2);
      unsigned int addressSpace = atoi(addrSpaceStr.c_str());

      size_t address = base + offset*size;
      Memory *memory = workItem->getMemory(addressSpace);
      if (!memory->store(data, address, size))
      {
        workItem->m_device->notifyMemoryError(false, addressSpace,
                                              address, size);
      }
      delete[] data;
    }


    /////////////////////////
    // Work-Item Functions //
    /////////////////////////

    DEFINE_BUILTIN(get_global_id)
    {
      uint64_t dim = UARG(0);
      size_t r = dim < 3 ? workItem->m_globalID[dim] : 0;
      *((size_t*)result.data) = r;
    }

    DEFINE_BUILTIN(get_global_size)
    {
      uint64_t dim = UARG(0);
      size_t r = dim < 3 ? workItem->m_workGroup.getGlobalSize()[dim] : 0;
      *((size_t*)result.data) = r;
    }

    DEFINE_BUILTIN(get_global_offset)
    {
      uint64_t dim = UARG(0);
      size_t r = dim < 3 ? workItem->m_workGroup.getGlobalOffset()[dim] : 0;
      *((size_t*)result.data) = r;
    }

    DEFINE_BUILTIN(get_group_id)
    {
      uint64_t dim = UARG(0);
      size_t r = dim < 3 ? workItem->m_workGroup.getGroupID()[dim] : 0;
      *((size_t*)result.data) = r;
    }

    DEFINE_BUILTIN(get_local_id)
    {
      uint64_t dim = UARG(0);
      size_t r = dim < 3 ? workItem->m_localID[dim] : 0;
      *((size_t*)result.data) = r;
    }

    DEFINE_BUILTIN(get_local_size)
    {
      uint64_t dim = UARG(0);
      size_t r = dim < 3 ? workItem->m_workGroup.getGroupSize()[dim] : 0;
      *((size_t*)result.data) = r;
    }

    DEFINE_BUILTIN(get_num_groups)
    {
      uint64_t dim = UARG(0);
      size_t r = 0;
      if (dim < 3)
      {
        r = workItem->m_workGroup.getGlobalSize()[dim] /
            workItem->m_workGroup.getGroupSize()[dim];
      }
      *((size_t*)result.data) = r;
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
            WorkItem::setFloatResult(result, (float)UARGV(0, i), i);
            break;
          case 'c':
          case 's':
          case 'i':
          case 'l':
            WorkItem::setFloatResult(result, (float)SARGV(0, i), i);
            break;
          case 'f':
          case 'd':
            WorkItem::setFloatResult(result, FARGV(0, i), i);
            break;
          default:
            assert(false);
        }
      }
    }

    static void setConvertRoundingMode(string name)
    {
      size_t rpos = name.find("_rt");
      if (rpos != string::npos)
      {
        switch (name[rpos+3])
        {
        case 'e':
          fesetround(FE_TONEAREST);
          break;
        case 'z':
          fesetround(FE_TOWARDZERO);
          break;
        case 'p':
          fesetround(FE_UPWARD);
          break;
        case 'n':
          fesetround(FE_DOWNWARD);
          break;
        default:
          assert(false);
        }
      }
      else
      {
        fesetround(FE_TOWARDZERO);
      }
    }

    DEFINE_BUILTIN(convert_uint)
    {
      // Check for saturation modifier
      bool sat = fnName.find("_sat") != string::npos;
      uint64_t max = (1UL<<(result.size*8)) - 1;

      // Use rounding mode
      const int origRnd = fegetround();
      setConvertRoundingMode(fnName);

      for (int i = 0; i < result.num; i++)
      {
        uint64_t r;
        switch (getOverloadArgType(overload))
        {
          case 'h':
          case 't':
          case 'j':
          case 'm':
            r = UARGV(0, i);
            if (sat)
            {
              r = _min_(r, max);
            }
            break;
          case 'c':
          case 's':
          case 'i':
          case 'l':
          {
            int64_t si = SARGV(0, i);
            r = si;
            if (sat)
            {
              if (si < 0)
              {
                r = 0;
              }
              else if (si > max)
              {
                r = max;
              }
            }
            break;
          }
          case 'f':
          case 'd':
            if (sat)
            {
              r = rint(_clamp_(FARGV(0, i), 0.0, (double)max));
            }
            else
            {
              r = rint(FARGV(0, i));
            }
            break;
          default:
            assert(false);
        }

        WorkItem::setIntResult(result, r, i);
      }
    }

    DEFINE_BUILTIN(convert_sint)
    {
      // Check for saturation modifier
      bool sat = fnName.find("_sat") != string::npos;
      int64_t min, max;
      switch (result.size)
      {
      case 1:
        min = INT8_MIN;
        max = INT8_MAX;
        break;
      case 2:
        min = INT16_MIN;
        max = INT16_MAX;
        break;
      case 4:
        min = INT32_MIN;
        max = INT32_MAX;
        break;
      case 8:
        min = INT64_MIN;
        max = INT64_MAX;
        break;
      }

      // Use rounding mode
      const int origRnd = fegetround();
      setConvertRoundingMode(fnName);

      for (int i = 0; i < result.num; i++)
      {
        int64_t r;
        switch (getOverloadArgType(overload))
        {
          case 'h':
          case 't':
          case 'j':
          case 'm':
            r = UARGV(0, i);
            if (sat)
            {
              r = _min_((uint64_t)r, (uint64_t)max);
            }
            break;
          case 'c':
          case 's':
          case 'i':
          case 'l':
            r = SARGV(0, i);
            if (sat)
            {
              r = _clamp_(r, min, max);
            }
            break;
          case 'f':
          case 'd':
            if (sat)
            {
              r = rint(_clamp_(FARGV(0, i), (double)min, (double)max));
            }
            else
            {
              r = rint(FARGV(0, i));
            }
            break;
          default:
            assert(false);
        }

        WorkItem::setIntResult(result, r, i);
      }
      fesetround(origRnd);
    }

    DEFINE_BUILTIN(printf_builtin)
    {
      const llvm::ConstantExpr *formatExpr = (llvm::ConstantExpr*)ARG(0);
      TypedValue formatPtrData = workItem->resolveConstExpr(formatExpr);
      size_t formatPtr = *(size_t*)formatPtrData.data;
      Memory *memory = workItem->m_device->getGlobalMemory();

      int arg = 1;
      while (true)
      {
        char c;
        memory->load((unsigned char*)&c, formatPtr++);
        if (c == '\0')
        {
          break;
        }

        if (c == '%')
        {
          string format = "%";
          while (true)
          {
            memory->load((unsigned char*)&c, formatPtr++);
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
      const llvm::DbgDeclareInst *dbgInst =
        (const llvm::DbgDeclareInst*)callInst;
      const llvm::Value *addr = dbgInst->getAddress();
      const llvm::MDNode *var = dbgInst->getVariable();
      const llvm::MDString *name = ((const llvm::MDString*)var->getOperand(2));
      workItem->m_variables[name->getString().str()] = addr;
    }

    DEFINE_BUILTIN(llvm_dbg_value)
    {
      const llvm::DbgValueInst *dbgInst = (const llvm::DbgValueInst*)callInst;
      const llvm::Value *value = dbgInst->getValue();
      const llvm::MDNode *var = dbgInst->getVariable();
      uint64_t offset = dbgInst->getOffset();

      // TODO: Use offset?
      const llvm::MDString *name = ((const llvm::MDString*)var->getOperand(2));
      workItem->m_variables[name->getString().str()] = value;
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
      size_t dest =
        *(size_t*)(workItem->m_instResults[memcpyInst->getDest()].data);
      size_t src =
        *(size_t*)(workItem->m_instResults[memcpyInst->getSource()].data);
      size_t size = workItem->getUnsignedInt(memcpyInst->getLength());
      unsigned destAddrSpace = memcpyInst->getDestAddressSpace();
      unsigned srcAddrSpace = memcpyInst->getSourceAddressSpace();
      Memory *destMemory = workItem->getMemory(destAddrSpace);
      Memory *srcMemory = workItem->getMemory(srcAddrSpace);

      unsigned char *buffer = new unsigned char[size];
      if (!srcMemory->load(buffer, src, size))
      {
        workItem->m_device->notifyMemoryError(true, srcAddrSpace, src, size);
      }
      else if (!destMemory->store(buffer, dest, size))
      {
        workItem->m_device->notifyMemoryError(false, destAddrSpace, dest, size);
      }
      delete[] buffer;
    }

    DEFINE_BUILTIN(llvm_memset)
    {
      const llvm::MemSetInst *memsetInst = (const llvm::MemSetInst*)callInst;
      size_t dest =
        *(size_t*)(workItem->m_instResults[memsetInst->getDest()].data);
      size_t size = workItem->getUnsignedInt(memsetInst->getLength());
      unsigned addressSpace = memsetInst->getDestAddressSpace();
      Memory *memory = workItem->getMemory(addressSpace);

      unsigned char *buffer = new unsigned char[size];
      unsigned char value = workItem->getUnsignedInt(ARG(1));
      memset(buffer, value, size);
      if (!memory->store(buffer, dest, size))
      {
        workItem->m_device->notifyMemoryError(false, addressSpace, dest, size);
      }
      delete[] buffer;
    }

    DEFINE_BUILTIN(llvm_trap)
    {
      workItem->trap();
    }

  public:
    static std::map<std::string, BuiltinFunction> initBuiltins();
  };

  // Utility macros for generating builtin function map
#define CAST                                \
  void(*)(WorkItem*, const llvm::CallInst*, \
  std::string, std::string, TypedValue& result, void*)
#define F1ARG(name) (double(*)(double))name
#define F2ARG(name) (double(*)(double,double))name
#define F3ARG(name) (double(*)(double,double,double))name
#define ADD_BUILTIN(name, func, op)         \
  workItemBuiltins[name] = BuiltinFunction((CAST)func, (void*)op);
#define ADD_PREFIX_BUILTIN(name, func, op)  \
  workItemPrefixBuiltins.push_back(                 \
    make_pair(name, BuiltinFunction((CAST)func, (void*)op)));

  // Generate builtin function map
  list< pair<string, BuiltinFunction> > workItemPrefixBuiltins;
  map<string,BuiltinFunction> workItemBuiltins =
    WorkItemBuiltins::initBuiltins();
  map<string,BuiltinFunction> WorkItemBuiltins::initBuiltins()
  {
    map<string, BuiltinFunction> builtins;

    // Async Copy and Prefetch Functions
    ADD_BUILTIN("async_work_group_copy", async_work_group_copy, NULL);
    ADD_BUILTIN("async_work_group_strided_copy", async_work_group_copy, NULL);
    ADD_BUILTIN("wait_group_events", wait_group_events, NULL);
    ADD_BUILTIN("prefetch", prefetch, NULL);

    // Atomic Functions
    ADD_BUILTIN("atom_add", atomic_add, NULL);
    ADD_BUILTIN("atomic_add", atomic_add, NULL);
    ADD_BUILTIN("atom_and", atomic_and, NULL);
    ADD_BUILTIN("atomic_and", atomic_and, NULL);
    ADD_BUILTIN("atom_cmpxchg", atomic_cmpxchg, NULL);
    ADD_BUILTIN("atomic_cmpxchg", atomic_cmpxchg, NULL);
    ADD_BUILTIN("atom_dec", atomic_dec, NULL);
    ADD_BUILTIN("atomic_dec", atomic_dec, NULL);
    ADD_BUILTIN("atom_inc", atomic_inc, NULL);
    ADD_BUILTIN("atomic_inc", atomic_inc, NULL);
    ADD_BUILTIN("atom_max", atomic_max, NULL);
    ADD_BUILTIN("atomic_max", atomic_max, NULL);
    ADD_BUILTIN("atom_min", atomic_min, NULL);
    ADD_BUILTIN("atomic_min", atomic_min, NULL);
    ADD_BUILTIN("atom_or", atomic_or, NULL);
    ADD_BUILTIN("atomic_or", atomic_or, NULL);
    ADD_BUILTIN("atom_sub", atomic_sub, NULL);
    ADD_BUILTIN("atomic_sub", atomic_sub, NULL);
    ADD_BUILTIN("atom_xchg", atomic_xchg, NULL);
    ADD_BUILTIN("atomic_xchg", atomic_xchg, NULL);
    ADD_BUILTIN("atom_xor", atomic_xor, NULL);
    ADD_BUILTIN("atomic_xor", atomic_xor, NULL);

    // Common Functions
    ADD_BUILTIN("clamp", clamp, NULL);
    ADD_BUILTIN("degrees", f1arg, _degrees_);
    ADD_BUILTIN("max", max, NULL);
    ADD_BUILTIN("min", min, NULL);
    ADD_BUILTIN("mix", f3arg, _mix_);
    ADD_BUILTIN("radians", f1arg, _radians_);
    ADD_BUILTIN("sign", f1arg, _sign_);
    ADD_BUILTIN("smoothstep", f3arg, _smoothstep_);
    ADD_BUILTIN("step", f2arg, _step_);

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
    ADD_BUILTIN("mad24", u3arg, _mad_);
    ADD_BUILTIN("mad_hi", mad_hi, NULL);
    ADD_BUILTIN("mad_sat", mad_sat, NULL);
    ADD_BUILTIN("mul24", u2arg, _mul_);
    ADD_BUILTIN("mul_hi", mul_hi, NULL);
    ADD_BUILTIN("popcount", u1arg, _popcount_);
    ADD_BUILTIN("rhadd", rhadd, NULL);
    ADD_BUILTIN("rotate", rotate, NULL);
    ADD_BUILTIN("sub_sat", sub_sat, NULL);
    ADD_BUILTIN("upsample", upsample, NULL);

    // Math Functions
    ADD_BUILTIN("acos", f1arg, F1ARG(acos));
    ADD_BUILTIN("acosh", f1arg, F1ARG(acosh));
    ADD_BUILTIN("acospi", f1arg, _acospi_);
    ADD_BUILTIN("asin", f1arg, F1ARG(asin));
    ADD_BUILTIN("asinh", f1arg, F1ARG(asinh));
    ADD_BUILTIN("asinpi", f1arg, _asinpi_);
    ADD_BUILTIN("atan", f1arg, F1ARG(atan));
    ADD_BUILTIN("atan2", f2arg, F2ARG(atan2));
    ADD_BUILTIN("atanh", f1arg, F1ARG(atanh));
    ADD_BUILTIN("atanpi", f1arg, _atanpi_);
    ADD_BUILTIN("atan2pi", f2arg, _atan2pi_);
    ADD_BUILTIN("cbrt", f1arg, F1ARG(cbrt));
    ADD_BUILTIN("ceil", f1arg, F1ARG(ceil));
    ADD_BUILTIN("copysign", f2arg, F2ARG(copysign));
    ADD_BUILTIN("cos", f1arg, F1ARG(cos));
    ADD_BUILTIN("cosh", f1arg, F1ARG(cosh));
    ADD_BUILTIN("cospi", f1arg, _cospi_);
    ADD_BUILTIN("erfc", f1arg, F1ARG(erfc));
    ADD_BUILTIN("erf", f1arg, F1ARG(erf));
    ADD_BUILTIN("exp", f1arg, F1ARG(exp));
    ADD_BUILTIN("exp2", f1arg, F1ARG(exp2));
    ADD_BUILTIN("exp10", f1arg, _exp10_);
    ADD_BUILTIN("expm1", f1arg, F1ARG(expm1));
    ADD_BUILTIN("fabs", f1arg, F1ARG(fabs));
    ADD_BUILTIN("fdim", f2arg, F2ARG(fdim));
    ADD_BUILTIN("floor", f1arg, F1ARG(floor));
    ADD_BUILTIN("fma", f3arg, F3ARG(fma));
    ADD_BUILTIN("fmax", f2arg, F2ARG(fmax));
    ADD_BUILTIN("fmin", f2arg, F2ARG(fmin));
    ADD_BUILTIN("fmod", f2arg, F2ARG(fmod));
    ADD_BUILTIN("fract", fract, NULL);
    ADD_BUILTIN("frexp", frexp_builtin, NULL);
    ADD_BUILTIN("hypot", f2arg, F2ARG(hypot));
    ADD_BUILTIN("ilogb", ilogb_builtin, NULL);
    ADD_BUILTIN("ldexp", ldexp_builtin, NULL);
    ADD_BUILTIN("lgamma", f1arg, F1ARG(lgamma));
    ADD_BUILTIN("lgamma_r", lgamma_r, NULL);
    ADD_BUILTIN("log", f1arg, F1ARG(log));
    ADD_BUILTIN("log2", f1arg, F1ARG(log2));
    ADD_BUILTIN("log10", f1arg, F1ARG(log10));
    ADD_BUILTIN("log1p", f1arg, F1ARG(log1p));
    ADD_BUILTIN("logb", f1arg, F1ARG(logb));
    ADD_BUILTIN("mad", f3arg, F3ARG(fma));
    ADD_BUILTIN("maxmag", f2arg, _maxmag_);
    ADD_BUILTIN("minmag", f2arg, _minmag_);
    ADD_BUILTIN("modf", modf_builtin, NULL);
    ADD_BUILTIN("nan", nan_builtin, NULL);
    ADD_BUILTIN("nextafter", f2arg, F2ARG(nextafter));
    ADD_BUILTIN("pow", f2arg, F2ARG(pow));
    ADD_BUILTIN("pown", pown, NULL);
    ADD_BUILTIN("powr", f2arg, F2ARG(pow));
    ADD_BUILTIN("remainder", f2arg, F2ARG(remainder));
    ADD_BUILTIN("remquo", remquo_builtin, NULL);
    ADD_BUILTIN("rint", f1arg, F1ARG(rint));
    ADD_BUILTIN("rootn", rootn, NULL);
    ADD_BUILTIN("round", f1arg, F1ARG(round));
    ADD_BUILTIN("rsqrt", f1arg, _rsqrt_);
    ADD_BUILTIN("sin", f1arg, F1ARG(sin));
    ADD_BUILTIN("sinh", f1arg, F1ARG(sinh));
    ADD_BUILTIN("sinpi", f1arg, _sinpi_);
    ADD_BUILTIN("sincos", sincos, NULL);
    ADD_BUILTIN("sqrt", f1arg, F1ARG(sqrt));
    ADD_BUILTIN("tan", f1arg, F1ARG(tan));
    ADD_BUILTIN("tanh", f1arg, F1ARG(tanh));
    ADD_BUILTIN("tanpi", f1arg, _tanpi_);
    ADD_BUILTIN("tgamma", f1arg, F1ARG(tgamma));
    ADD_BUILTIN("trunc", f1arg, F1ARG(trunc));

    // Native Math Functions
    ADD_BUILTIN("half_cos", f1arg, F1ARG(cos));
    ADD_BUILTIN("native_cos", f1arg, F1ARG(cos));
    ADD_BUILTIN("half_divide", f2arg, _fdivide_);
    ADD_BUILTIN("native_divide", f2arg, _fdivide_);
    ADD_BUILTIN("half_exp", f1arg, F1ARG(exp));
    ADD_BUILTIN("native_exp", f1arg, F1ARG(exp));
    ADD_BUILTIN("half_exp2", f1arg, F1ARG(exp2));
    ADD_BUILTIN("native_exp2", f1arg, F1ARG(exp2));
    ADD_BUILTIN("half_exp10", f1arg, _exp10_);
    ADD_BUILTIN("native_exp10", f1arg, _exp10_);
    ADD_BUILTIN("half_log", f1arg, F1ARG(log));
    ADD_BUILTIN("native_log", f1arg, F1ARG(log));
    ADD_BUILTIN("half_log2", f1arg, F1ARG(log2));
    ADD_BUILTIN("native_log2", f1arg, F1ARG(log2));
    ADD_BUILTIN("half_log10", f1arg, F1ARG(log10));
    ADD_BUILTIN("native_log10", f1arg, F1ARG(log10));
    ADD_BUILTIN("half_powr", f2arg, F2ARG(pow));
    ADD_BUILTIN("native_powr", f2arg, F2ARG(pow));
    ADD_BUILTIN("half_recip", f1arg, _frecip_);
    ADD_BUILTIN("native_recip", f1arg, _frecip_);
    ADD_BUILTIN("half_rsqrt", f1arg, _rsqrt_);
    ADD_BUILTIN("native_rsqrt", f1arg, _rsqrt_);
    ADD_BUILTIN("half_sin", f1arg, F1ARG(sin));
    ADD_BUILTIN("native_sin", f1arg, F1ARG(sin));
    ADD_BUILTIN("half_sqrt", f1arg, F1ARG(sqrt));
    ADD_BUILTIN("native_sqrt", f1arg, F1ARG(sqrt));
    ADD_BUILTIN("half_tan", f1arg, F1ARG(tan));
    ADD_BUILTIN("native_tan", f1arg, F1ARG(tan));

    // Misc. Vector Functions
    ADD_BUILTIN("shuffle", shuffle_builtin, NULL);
    ADD_BUILTIN("shuffle2", shuffle2_builtin, NULL);

    // Relational Functional
    ADD_BUILTIN("all", all, NULL);
    ADD_BUILTIN("any", any, NULL);
    ADD_BUILTIN("bitselect", bitselect, NULL);
    ADD_BUILTIN("isequal", rel2arg, _iseq_);
    ADD_BUILTIN("isnotequal", rel2arg, _isneq_);
    ADD_BUILTIN("isgreater", rel2arg, _isgt_);
    ADD_BUILTIN("isgreaterequal", rel2arg, _isge_);
    ADD_BUILTIN("isless", rel2arg, _islt_);
    ADD_BUILTIN("islessequal", rel2arg, _isle_);
    ADD_BUILTIN("islessgreater", rel2arg, _islg_);
    ADD_BUILTIN("isfinite", rel1arg, _isfin_);
    ADD_BUILTIN("isinf", rel1arg, _isinf_);
    ADD_BUILTIN("isnan", rel1arg, _isnan_);
    ADD_BUILTIN("isnormal", rel1arg, _isnorm_);
    ADD_BUILTIN("isordered", rel2arg, _isord_);
    ADD_BUILTIN("isunordered", rel2arg, _isuord_);
    ADD_BUILTIN("select", select_builtin, NULL);
    ADD_BUILTIN("signbit", rel1arg, _signbit_);

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
    ADD_PREFIX_BUILTIN("convert_float",  convert_float, NULL);
    ADD_PREFIX_BUILTIN("convert_double", convert_float, NULL);
    ADD_PREFIX_BUILTIN("convert_u",      convert_uint, NULL);
    ADD_PREFIX_BUILTIN("convert_",       convert_sint, NULL);
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
}
