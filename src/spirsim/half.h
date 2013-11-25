// half.h (oclgrind)
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
			f_exponent = 0;
			f_mantissa = 0;
		}
		else
		{
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
		f_exponent = 0xFF << 23;
		f_mantissa = h_mantissa;
	}
	else
	{
		f_exponent = (((int32_t)(h_exponent >> 10)) - 15 + 127) << 23;
		f_mantissa = ((uint32_t)h_mantissa) << 13;
	}

	uint32_t result = f_sign | f_exponent | f_mantissa;
	return *(float*)&result;
}
