// half.cpp (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "half.h"

namespace oclgrind
{
  float halfToFloat(uint16_t half)
  {
    uint16_t h_sign, h_exponent, h_mantissa;
    uint32_t f_sign, f_exponent, f_mantissa;

    h_sign     = half & 0x8000; // 1000 0000 0000 0000
    h_exponent = half & 0x7C00; // 0111 1100 0000 0000
    h_mantissa = half & 0x03FF; // 0000 0011 1111 1111

    f_sign     = ((uint32_t)h_sign) << 16;

    if (h_exponent == 0)
    {
      if (h_mantissa == 0)
      {
        // Zero
        f_exponent = 0;
        f_mantissa = 0;
      }
      else
      {
        // Denorm - convert to normalized float
        int e = -1;
        do
        {
          e++;
          h_mantissa <<= 1;
        }
        while((h_mantissa & 0x0400) == 0);

        f_exponent = (-15 + 127 - e) << 23;
        f_mantissa = ((uint32_t)(h_mantissa & 0x03FF)) << 13;
      }
    }
    else if (h_exponent == 0x7C00)
    {
      // Inf or NaN
      f_exponent = 0xFF << 23;
      f_mantissa = h_mantissa;
    }
    else
    {
      // Normalized
      f_exponent = (((int32_t)(h_exponent >> 10)) - 15 + 127) << 23;
      f_mantissa = ((uint32_t)h_mantissa) << 13;
    }

    uint32_t result = f_sign | f_exponent | f_mantissa;
    return *(float*)&result;
  }

  uint16_t floatToHalf(float sp, HalfRoundMode round)
  {
    uint16_t h_sign, h_exponent, h_mantissa;
    uint32_t f_sign, f_exponent, f_mantissa;

    union
    {
      float f;
      uint32_t ui;
    } FtoUI;
    FtoUI.f = sp;
    uint32_t f = FtoUI.ui;
    f_sign     = f & 0x80000000; // 1000 0000 0000 0000 0000 0000 0000 0000
    f_exponent = f & 0x7F800000; // 0111 1111 1000 0000 0000 0000 0000 0000
    f_mantissa = f & 0x007FFFFF; // 0000 0000 0111 1111 1111 1111 1111 1111

    h_sign     = f_sign >> 16;

    if (f_exponent == 0)
    {
      // Zero
      h_exponent = 0;
      h_mantissa = 0;
    }
    else if (f_exponent == 0x7F800000)
    {
      // Inf or NaN
      h_exponent = 0x7C00;
      if (f_mantissa)
        h_mantissa = 0x1FF;
      else
        h_mantissa = 0;
    }
    else
    {
      int e = (((int32_t)(f_exponent >> 23)) - 127 + 15);
      if (e >= 0x1F)
      {
        // Value will overflow
        h_exponent = 0x7C00;
        h_mantissa = 0;

        if (round == Half_RTZ)
          h_mantissa = -1;
        if (round == Half_RTP && h_sign)
          h_mantissa = -1;
        if (round == Half_RTN && !h_sign)
          h_mantissa = -1;
      }
      else if (e <= 0)
      {
        // Value will underflow
        h_exponent = 0;
        if (14 - e > 24)
        {
          // Too small - flush to zero
          h_mantissa = 0;
        }
        else
        {
          // Convert to denorm
          f_mantissa |= 0x800000;
          h_mantissa = (f_mantissa >> (14-e));
          if ((f_mantissa >> (13 - e)) & 0x1)
          {
            h_mantissa += 0x1;
          }
        }
      }
      else
      {
        // Normalized
        h_exponent = e << 10;
        h_mantissa = f_mantissa >> 13;
        // The current f_mantissa is done in RTZ
        if (round == Half_RTE && (f & 0x00001000) != 0)
        {
          if ((f & 0x00002FFF) != 0)
            h_mantissa += 1;
        }
        else if (round == Half_RTP)
        {
          FtoUI.ui &= 0xFFFFE000;
          if (FtoUI.f < sp)
            h_mantissa += 1;
        }
        else if (round == Half_RTN)
        {
          FtoUI.ui &= 0xFFFFE000;
          if (sp < FtoUI.f)
            h_mantissa += 1;
        }
      }
    }

    return h_sign + h_exponent + h_mantissa;
  }

  uint16_t doubleToHalf(double dp, HalfRoundMode round)
  {
    uint16_t h_sign, h_exponent, h_mantissa;
    uint64_t d_sign, d_exponent, d_mantissa;

    union
    {
      double d;
      uint64_t ui;
    } DtoUI;
    DtoUI.d = dp;
    uint64_t d = DtoUI.ui;
    d_sign     = d & 0x8000000000000000;
    d_exponent = d & 0x7FF0000000000000;
    d_mantissa = d & 0x000FFFFFFFFFFFFF;

    h_sign     = d_sign >> 48;

    if (d_exponent == 0)
    {
      // Zero
      h_exponent = 0;
      h_mantissa = 0;
    }
    else if (d_exponent == 0x7FF0000000000000)
    {
      // Inf or NaN
      h_exponent = 0x7C00;
      if (d_mantissa)
        h_mantissa = 0x1FF;
      else
        h_mantissa = 0;
    }
    else
    {
      int e = (((int64_t)(d_exponent >> 52)) - 1023 + 15);
      if (e >= 0x1F)
      {
        // Value will overflow
        h_exponent = 0x7C00;
        h_mantissa = 0;

        if (round == Half_RTZ)
          h_mantissa = -1;
        if (round == Half_RTP && h_sign)
          h_mantissa = -1;
        if (round == Half_RTN && !h_sign)
          h_mantissa = -1;
      }
      else if (e <= 0)
      {
        // Value will underflow
        h_exponent = 0;
        if (14 - e > 24)
        {
          // Too small - flush to zero
          h_mantissa = 0;
        }
        else
        {
          // Convert to denorm
          d_mantissa |= 0x0010000000000000;
          h_mantissa = (d_mantissa >> (14-e));
          if ((d_mantissa >> (13 - e)) & 0x1)
          {
            h_mantissa += 0x1;
          }
        }
      }
      else
      {
        // Normalized
        h_exponent = e << 10;
        h_mantissa = d_mantissa >> 42;
        // The current f_mantissa is done in RTZ
        if (round == Half_RTE && (d & 0x20000000000) != 0)
        {
          if ((d & 0x5FFFFFFFFFF) != 0)
            h_mantissa += 1;
        }
        else if (round == Half_RTP)
        {
          DtoUI.ui &= 0xFFFFFC0000000000;
          if (DtoUI.d < dp)
            h_mantissa += 1;
        }
        else if (round == Half_RTN)
        {
          DtoUI.ui &= 0xFFFFFC0000000000;
          if (dp < DtoUI.d)
            h_mantissa += 1;
        }
      }
    }

    return h_sign + h_exponent + h_mantissa;
  }
}
