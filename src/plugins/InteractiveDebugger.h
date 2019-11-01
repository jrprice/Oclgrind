// InteractiveDebugger.h (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/Plugin.h"

namespace oclgrind
{
  class Program;

  class InteractiveDebugger : public Plugin
  {
  public:
    InteractiveDebugger(const Context *context);

    virtual void instructionExecuted(const WorkItem *workItem,
                                     const llvm::Instruction *instruction,
                                     const TypedValue& result) override;
    virtual void kernelBegin(const KernelInvocation *kernelInvocation) override;
    virtual void kernelEnd(const KernelInvocation *kernelInvocation) override;
    virtual void log(MessageType type, const char *message) override;

    virtual bool isThreadSafe() const override;

  private:

    bool m_continue;
    bool m_running;
    bool m_forceBreak;
    size_t m_listPosition;
    bool m_next;
    size_t m_lastBreakLine;
    size_t m_nextBreakpoint;
    size_t m_previousDepth;
    size_t m_previousLine;
    std::map<const Program*, std::map<size_t, size_t> > m_breakpoints;
    const Program *m_program;
    const KernelInvocation *m_kernelInvocation;

    size_t getCurrentLineNumber() const;
    size_t getLineNumber(const llvm::Instruction *instruction) const;
    bool hasHitBreakpoint();
    void printCurrentLine() const;
    void printFunction(const llvm::Instruction *instruction) const;
    void printSourceLine(size_t lineNum) const;
    bool shouldShowPrompt(const WorkItem *workItem);

    // Interactive commands
    typedef bool (InteractiveDebugger::*Command)(std::vector<std::string>);
    std::map<std::string, Command> m_commands;
#define CMD(name) bool name(std::vector<std::string> args);
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
#undef CMD

  };
}
