// Device.h (oclgrind)
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

namespace spirsim
{
  class Kernel;
  class Memory;

  class Device
  {
  public:
    static const unsigned char OUTPUT_GLOBAL_MEM = 0x01;
    static const unsigned char OUTPUT_LOCAL_MEM = 0x02;
    static const unsigned char OUTPUT_PRIVATE_MEM = 0x04;
    static const unsigned char OUTPUT_INSTRUCTIONS = 0x08;

  public:
    Device();
    virtual ~Device();

    Memory *getGlobalMemory() const;
    void run(Kernel& kernel, unsigned int workDim,
             const size_t *globalOffset,
             const size_t *globalSize,
             const size_t *localSize);
    void setOutputMask(unsigned char mask);

  private:
    Memory *m_globalMemory;
    unsigned char m_outputMask;
  };
}
