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
  class WorkGroup;
  class WorkItem;

  class Device
  {
  public:
    Device();
    virtual ~Device();

    Memory *getGlobalMemory() const;
    void run(Kernel& kernel, unsigned int workDim,
             const size_t *globalOffset,
             const size_t *globalSize,
             const size_t *localSize);

  private:
    Memory *m_globalMemory;

    // Comparator for ordering work-groups
    struct WorkGroupCmp
    {
      bool operator()(const WorkGroup *lhs, const WorkGroup *rhs) const;
    };

    // Current kernel invocation
    Kernel *m_kernel;
    WorkGroup **m_workGroups;
    WorkGroup *m_currentWorkGroup;
    WorkItem *m_currentWorkItem;
    std::set<WorkGroup*, WorkGroupCmp> m_runningGroups;
    size_t m_globalSize[3];
    size_t m_globalOffset[3];
    size_t m_localSize[3];
    size_t m_numGroups[3];
    std::vector<std::string> m_sourceLines;

    bool m_interactive;
    bool m_running;
    bool m_break;
    typedef void (Device::*Command)(std::vector<std::string>);
    std::map<std::string, Command> m_commands;

    size_t getCurrentLineNumber() const;
    bool nextWorkItem();
    void printCurrentLine() const;

    // Interactive commands
#define CMD(name) void name(std::vector<std::string> args);
    CMD(backtrace);
    CMD(brk);
    CMD(clear);
    CMD(cont);
    CMD(help);
    CMD(info);
    CMD(list);
    CMD(print);
    CMD(printglobal);
    CMD(printlocal);
    CMD(printprivate);
    CMD(quit);
    CMD(step);
    CMD(workitem);
  };
}
