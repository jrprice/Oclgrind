// Device.h (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

namespace spirsim
{
  class Kernel;
  class Memory;
  class Program;
  class WorkGroup;
  class WorkItem;

  class Device
  {
  public:
    Device();
    virtual ~Device();

    Memory *getGlobalMemory() const;
    const WorkGroup* getCurrentWorkGroup() const;
    const WorkItem* getCurrentWorkItem() const;
    bool isInteractive() const;
    bool isShowingInstructionCounts() const;
    void notifyDataRace(DataRaceType type, unsigned int addrSpace,
                        size_t address, size_t lastWorkItem,
                        const llvm::Instruction *lastInstruction);
    void notifyMemoryError(bool read, unsigned int addrSpace,
                           size_t address, size_t size);
    void run(Kernel& kernel, unsigned int workDim,
             const size_t *globalOffset,
             const size_t *globalSize,
             const size_t *localSize);

  private:
    Memory *m_globalMemory;

    struct PendingWorkGroup
    {
      size_t group[3];
      size_t operator[](int i) const
      {
        return group[i];
      }
    };

    // Current kernel invocation
    const Program *m_program;
    Kernel *m_kernel;
    WorkGroup *m_currentWorkGroup;
    WorkItem *m_currentWorkItem;
    std::list<PendingWorkGroup> m_pendingGroups;
    std::list<WorkGroup*> m_runningGroups;
    size_t m_workDim;
    size_t m_globalSize[3];
    size_t m_globalOffset[3];
    size_t m_localSize[3];
    size_t m_numGroups[3];
    std::vector<std::string> m_sourceLines;
    size_t m_listPosition;
    size_t m_nextBreakpoint;
    std::map<const Program*, std::map<size_t, size_t> > m_breakpoints;

    bool m_quickMode;
    bool m_showInstCounts;

    bool m_interactive;
    bool m_running;
    bool m_forceBreak;
    typedef void (Device::*Command)(std::vector<std::string>);
    std::map<std::string, Command> m_commands;

    WorkGroup* createWorkGroup(size_t x, size_t y, size_t z);
    size_t getCurrentLineNumber() const;
    size_t getLineNumber(const llvm::Instruction *instruction) const;
    bool nextWorkItem();
    void printCurrentLine() const;
    void printErrorContext() const;
    void printFunction(const llvm::Instruction *instruction) const;
    void printInstruction(const llvm::Instruction *instruction) const;
    void printSourceLine(size_t lineNum) const;
    void step();

    // Interactive commands
#define CMD(name) void name(std::vector<std::string> args);
    CMD(backtrace);
    CMD(brk);
    CMD(cont);
    CMD(del);
    CMD(help);
    CMD(info);
    CMD(list);
    CMD(mem);
    CMD(next);
    CMD(print);
    CMD(quit);
    CMD(step);
    CMD(workitem);
  };
}
