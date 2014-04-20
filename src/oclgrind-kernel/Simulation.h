// Simulation.h (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "config.h"
#include <fstream>
#include <string>

namespace oclgrind
{
  class Device;
  class Kernel;
  class Program;
};

class Simulation
{
  public:
    Simulation();
    virtual ~Simulation();

    bool load(std::string filename);
    void run(bool dumpGlobalMemory=false);

  private:
    oclgrind::Device *m_device;
    oclgrind::Kernel *m_kernel;
    oclgrind::Program *m_program;

    size_t m_ndrange[3];
    size_t m_wgsize[3];

    std::ifstream m_simfile;
    std::string m_parsing;
    void get(std::string& result);
    template<typename T> void get(T& result);
};
