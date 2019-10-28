// half.h (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

namespace oclgrind
{
  enum HalfRoundMode
  {
    // Towards negative infinity
    Half_RTN,
    // Towards zero
    Half_RTZ,
    // Towards positive infinity
    Half_RTP,
    // Towards nearest even
    Half_RTE
  };

  float halfToFloat(uint16_t half);

  uint16_t floatToHalf(float sp, HalfRoundMode round = Half_RTZ);
  uint16_t doubleToHalf(double dp, HalfRoundMode round = Half_RTZ);
}
