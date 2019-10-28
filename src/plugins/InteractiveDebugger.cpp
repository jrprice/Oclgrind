// InteractiveDebugger.cpp (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "config.h"
#include "core/common.h"

#include <iterator>
#include <sstream>

#if !defined(_WIN32) || defined(__MINGW32__)
#include <signal.h>
#include <unistd.h>
#else
#include <io.h>
#define isatty _isatty
#define STDIN_FILENO _fileno(stdin)
#endif

#if HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

#include "InteractiveDebugger.h"
#include "core/Context.h"
#include "core/Kernel.h"
#include "core/KernelInvocation.h"
#include "core/Memory.h"
#include "core/Program.h"
#include "core/WorkGroup.h"
#include "core/WorkItem.h"

using namespace oclgrind;
using namespace std;

#define LIST_LENGTH 10

static bool sigintBreak = false;
#if !defined(_WIN32) || defined(__MINGW32__)
static struct sigaction m_oldSignalHandler;
void handleSignal(int s)
{
  if (s == SIGINT)
    sigintBreak = true;
}
#endif

InteractiveDebugger::InteractiveDebugger(const Context *context)
  : Plugin(context)
{
  m_running          = true;
  m_forceBreak       = false;
  m_nextBreakpoint   = 1;
  m_program          = NULL;
  m_kernelInvocation = NULL;

  // Set-up commands
#define ADD_CMD(name, sname, func)  \
  m_commands[name] = &InteractiveDebugger::func; \
  m_commands[sname] = &InteractiveDebugger::func;
  ADD_CMD("backtrace",    "bt", backtrace);
  ADD_CMD("break",        "b",  brk);
  ADD_CMD("continue",     "c",  cont);
  ADD_CMD("delete",       "d",  del);
  ADD_CMD("gmem",         "gm", mem);
  ADD_CMD("help",         "h",  help);
  ADD_CMD("info",         "i",  info);
  ADD_CMD("list",         "l",  list);
  ADD_CMD("lmem",         "lm", mem);
  ADD_CMD("next",         "n",  next);
  ADD_CMD("pmem",         "pm", mem);
  ADD_CMD("print",        "p",  print);
  ADD_CMD("quit",         "q",  quit);
  ADD_CMD("step",         "s",  step);
  ADD_CMD("workitem",     "wi", workitem);
}

void InteractiveDebugger::instructionExecuted(
  const WorkItem *workItem, const llvm::Instruction *instruction,
  const TypedValue& result)
{
  if (!shouldShowPrompt(workItem))
    return;

#if !defined(_WIN32) || defined(__MINGW32__)
  // Restore old signal handler
  sigaction(SIGINT, &m_oldSignalHandler, NULL);
#endif

  m_forceBreak = false;
  sigintBreak  = false;

  // Print function if changed
  if (m_previousDepth != workItem->getCallStack().size() &&
      workItem->getState() != WorkItem::FINISHED)
  {
    cout << "In function ";
    printFunction(workItem->getCurrentInstruction());
  }

  printCurrentLine();

  m_listPosition = 0;
  m_continue     = false;
  m_next         = false;

  bool interactive = isatty(STDIN_FILENO);
  while (true)
  {
    // Prompt for command
    bool eof = false;
    string cmd;
  #if HAVE_READLINE
    if (interactive)
    {
      char *line = readline("(oclgrind) ");
      if (line)
      {
        cmd = line;
        free(line);
      }
      else
      {
        eof = true;
      }
    }
    else
  #endif
    {
      if (interactive)
        cout << "(oclgrind) " << flush;
      getline(cin, cmd);
      eof = cin.eof();
    }

    // Quit on EOF
    if (eof)
    {
      if (interactive)
        cout << "(quit)" << endl;
      quit(vector<string>());
      return;
    }

    // Split command into tokens
    vector<string> tokens;
    istringstream iss(cmd);
    copy(istream_iterator<string>(iss),
         istream_iterator<string>(),
         back_inserter< vector<string> >(tokens));

    // Skip empty lines
    if (tokens.size() == 0)
    {
      continue;
    }

  #if HAVE_READLINE
    if (interactive)
      add_history(cmd.c_str());
  #endif

    // Find command in map and execute
    map<string,Command>::iterator itr = m_commands.find(tokens[0]);
    if (itr != m_commands.end())
    {
      if ((this->*itr->second)(tokens))
        break;
    }
    else
    {
      cout << "Unrecognized command '" << tokens[0] << "'" << endl;
    }
  }
}

bool InteractiveDebugger::isThreadSafe() const
{
  return false;
}

void InteractiveDebugger::kernelBegin(const KernelInvocation *kernelInvocation)
{
  m_continue      = false;
  m_lastBreakLine = 0;
  m_listPosition  = 0;
  m_next          = false;
  m_previousDepth = 0;
  m_previousLine  = 0;

  m_kernelInvocation = kernelInvocation;
  m_program = kernelInvocation->getKernel()->getProgram();
}

void InteractiveDebugger::kernelEnd(const KernelInvocation *kernelInvocation)
{
  m_kernelInvocation = NULL;

#if !defined(_WIN32) || defined(__MINGW32__)
  // Restore old signal handler
  sigaction(SIGINT, &m_oldSignalHandler, NULL);
#endif
}

void InteractiveDebugger::log(MessageType type, const char *message)
{
  if (type == ERROR)
    m_forceBreak = true;
}

///////////////////////////
//// Utility Functions ////
///////////////////////////

size_t InteractiveDebugger::getCurrentLineNumber() const
{
  const WorkItem *workItem = m_kernelInvocation->getCurrentWorkItem();
  if (!workItem || workItem->getState() == WorkItem::FINISHED)
  {
    return 0;
  }

  return getLineNumber(workItem->getCurrentInstruction());
}

size_t InteractiveDebugger::getLineNumber(
  const llvm::Instruction *instruction) const
{
  llvm::MDNode *md = instruction->getMetadata("dbg");
  if (md)
  {
    return ((llvm::DILocation*)md)->getLine();
  }
  return 0;
}

bool InteractiveDebugger::hasHitBreakpoint()
{
  if (m_breakpoints.empty())
    return false;

  // Check if we have passed over the previous breakpoint
  if (m_lastBreakLine)
  {
    if (getCurrentLineNumber() != m_lastBreakLine)
      m_lastBreakLine = 0;
    else
      return false;;
  }

  // Check if we're at a breakpoint
  size_t line = getCurrentLineNumber();
  map<size_t, size_t>::iterator itr;
  for (itr = m_breakpoints[m_program].begin();
       itr != m_breakpoints[m_program].end(); itr++)
  {
    if (itr->second == line)
    {
      cout << "Breakpoint " << itr->first
           << " hit at line " << itr->second
           << " by work-item "
           << m_kernelInvocation->getCurrentWorkItem()->getGlobalID()
           << endl;
      m_lastBreakLine = line;
      m_listPosition = 0;
      return true;
    }
  }
  return false;
}

void InteractiveDebugger::printCurrentLine() const
{
  const WorkItem *workItem = m_kernelInvocation->getCurrentWorkItem();
  if (!workItem || workItem->getState() == WorkItem::FINISHED)
  {
    return;
  }

  size_t lineNum = getCurrentLineNumber();
  if (m_program->getNumSourceLines() && lineNum > 0)
  {
    printSourceLine(lineNum);
  }
  else
  {
    cout << "Source line not available." << endl;
    dumpInstruction(cout, workItem->getCurrentInstruction());
    cout << endl;
  }
}

void InteractiveDebugger::printFunction(
  const llvm::Instruction *instruction) const
{
  // Get function
  const llvm::Function *function = instruction->getParent()->getParent();
  cout << function->getName().str() << "(";

  // Print arguments
  llvm::Function::const_arg_iterator argItr;
  for (argItr = function->arg_begin();
       argItr != function->arg_end(); argItr++)
  {
    if (argItr != function->arg_begin())
    {
      cout << ", ";
    }
    cout << argItr->getName().str() << "=";
    m_kernelInvocation->getCurrentWorkItem()->printValue(&*argItr);
  }

  cout << ") at line " << dec << getLineNumber(instruction) << endl;
}

void InteractiveDebugger::printSourceLine(size_t lineNum) const
{
  const char *line = m_program->getSourceLine(lineNum);
  if (line)
  {
    cout << dec << lineNum << "\t" << line << endl;
  }
  else
  {
    cout << "Invalid line number: " << lineNum << endl;
  }
}

bool InteractiveDebugger::shouldShowPrompt(const WorkItem *workItem)
{
  if (!m_running)
    return false;

  if (m_forceBreak || sigintBreak)
    return true;

  if (hasHitBreakpoint())
    return true;

  if (m_continue)
    return false;

  if (workItem->getState() == WorkItem::BARRIER)
    return true;
  if (workItem->getState() == WorkItem::FINISHED)
    return true;

  if (!m_program->getNumSourceLines())
    return true;

  size_t line = getCurrentLineNumber();
  if (m_next && workItem->getCallStack().size() > m_previousDepth)
    return false;
  if (!line || line == m_previousLine)
    return false;

  return true;
}

//////////////////////////////
//// Interactive Commands ////
//////////////////////////////

bool InteractiveDebugger::backtrace(vector<string> args)
{
  const WorkItem *workItem = m_kernelInvocation->getCurrentWorkItem();
  if (!workItem || workItem->getState() == WorkItem::FINISHED)
  {
    return false;
  }

  stack<const llvm::Instruction*> callStack = workItem->getCallStack();

  // Print current instruction
  cout << "#" << callStack.size() <<  " ";
  printFunction(workItem->getCurrentInstruction());

  // Print call stack
  while (!callStack.empty())
  {
    cout << "#" << (callStack.size()-1) <<  " ";
    printFunction(callStack.top());
    callStack.pop();
  }

  return false;
}

bool InteractiveDebugger::brk(vector<string> args)
{
  if (!m_program->getNumSourceLines())
  {
    cout << "Breakpoints only valid when source is available." << endl;
    return false;
  }

  size_t lineNum = getCurrentLineNumber();
  if (args.size() > 1)
  {
    // Parse argument as a target line number
    istringstream ss(args[1]);
    ss >> lineNum;
    if (!ss.eof() || !lineNum || lineNum > m_program->getNumSourceLines()+1)
    {
      cout << "Invalid line number." << endl;
      return false;
    }
  }

  if (lineNum)
  {
    m_breakpoints[m_program][m_nextBreakpoint++] = lineNum;
  }
  else
  {
    cout << "Not currently on a line." << endl;
  }

  return false;
}

bool InteractiveDebugger::cont(vector<string> args)
{
#if !defined(_WIN32) || defined(__MINGW32__)
  // Register a signal handler to catch interrupts
  struct sigaction sigHandler;
  sigHandler.sa_handler = handleSignal;
  sigemptyset(&sigHandler.sa_mask);
  sigHandler.sa_flags = 0;
  sigaction(SIGINT, &sigHandler, &m_oldSignalHandler);
#endif

  m_continue = true;
  return true;
}

bool InteractiveDebugger::del(vector<string> args)
{
  if (args.size() > 1)
  {
    // Parse argument as a target breakpoint
    size_t bpNum = 0;
    istringstream ss(args[1]);
    ss >> bpNum;
    if (!ss.eof())
    {
      cout << "Invalid breakpoint number." << endl;
      return false;
    }

    // Ensure breakpoint exists
    if (!m_breakpoints[m_program].count(bpNum))
    {
      cout << "Breakpoint not found." << endl;
      return false;
    }
    m_breakpoints[m_program].erase(bpNum);
  }
  else
  {
    // Prompt for confimation
    string confirm;
    cout << "Delete all breakpoints? (y/n) " << flush;
    cin >> confirm;
    cin.ignore();
    if (confirm == "y")
    {
      m_breakpoints.clear();
    }
  }

  return false;
}

bool InteractiveDebugger::help(vector<string> args)
{
  if (args.size() < 2)
  {
    cout << "Command list:" << endl;
    cout << "  backtrace    (bt)" << endl;
    cout << "  break        (b)" << endl;
    cout << "  continue     (c)" << endl;
    cout << "  delete       (d)" << endl;
    cout << "  gmem         (gm)" << endl;
    cout << "  help         (h)" << endl;
    cout << "  info         (i)" << endl;
    cout << "  list         (l)" << endl;
    cout << "  next         (n)" << endl;
    cout << "  lmem         (lm)" << endl;
    cout << "  pmem         (pm)" << endl;
    cout << "  print        (p)" << endl;
    cout << "  quit         (q)" << endl;
    cout << "  step         (s)" << endl;
    cout << "  workitem     (wi)" << endl;
    cout << "(type 'help command' for more information)" << endl;
    return false;
  }

  if (args[1] == "backtrace" || args[1] == "bt")
  {
    cout << "Print function call stack." << endl;
  }
  else if (args[1] == "break" || args[1] == "b")
  {
    cout << "Set a breakpoint"
         << " (only functional when source is available)." << endl
         << "With no arguments, sets a breakpoint at the current line." << endl
         << "Use a numeric argument to set a breakpoint at a specific line."
         << endl;
  }
  else if (args[1] == "continue" || args[1] == "c")
  {
    cout << "Continue kernel execution until next breakpoint." << endl;
  }
  else if (args[1] == "delete" || args[1] == "d")
  {
    cout << "Delete a breakpoint." << endl
         << "With no arguments, deletes all breakpoints." << endl;
  }
  else if (args[1] == "help" || args[1] == "h")
  {
    cout << "Display usage information for a command." << endl;
  }
  else if (args[1] == "info" || args[1] == "i")
  {
    cout << "Display information about current debugging context." << endl
         << "With no arguments, displays general information." << endl
         << "'info break' lists breakpoints."
         << endl;
  }
  else if (args[1] == "list" || args[1] == "l")
  {
    cout << "List source lines." << endl
         << "With no argument, lists " << LIST_LENGTH
         << " lines after previous listing." << endl
         << "Use - to list " << LIST_LENGTH
         << " lines before the previous listing" << endl
         << "Use a numeric argument to list around a specific line number."
         << endl;
  }
  else if (args[1] == "gmem" || args[1] == "lmem" || args[1] == "pmem" ||
           args[1] == "gm"   || args[1] == "lm"   || args[1] == "pm")
  {
    cout << "Examine contents of ";
    if (args[1] == "gmem") cout << "global";
    if (args[1] == "lmem") cout << "local";
    if (args[1] == "pmem") cout << "private";
    cout << " memory." << endl
         << "With no arguments, dumps entire contents of memory." << endl
         << "'" << args[1] << " address [size]'" << endl
         << "address is hexadecimal and 4-byte aligned." << endl;
  }
  else if (args[1] == "next" || args[1] == "n")
  {
    cout << "Step forward,"
         << " treating function calls as single instruction." << endl;
  }
  else if (args[1] == "print" || args[1] == "p")
  {
    cout << "Print the values of one or more variables." << endl
         << "'print x y' prints the values of x and y" << endl
         << "'print foo[i]' prints a value at a constant array index" << endl;
  }
  else if (args[1] == "quit" || args[1] == "q")
  {
    cout << "Quit interactive debugger." << endl;
  }
  else if (args[1] == "step" || args[1] == "s")
  {
    cout << "Step forward a single source line,"
         << " or an instruction if no source available." << endl;
  }
  else if (args[1] == "workitem" || args[1] == "wi")
  {
    cout << "Switch to a different work-item." << endl
         << "Up to three (space separated) arguments allowed,"
         << " specifying the global ID of the work-item." << endl;
  }
  else
  {
    cout << "Unrecognized command '" << args[1] << "'" << endl;
  }

  return false;
}

bool InteractiveDebugger::info(vector<string> args)
{
  if (args.size() > 1)
  {
    if (args[1] == "break")
    {
      // List breakpoints
      map<size_t, size_t>::iterator itr;
      for (itr = m_breakpoints[m_program].begin();
           itr != m_breakpoints[m_program].end(); itr++)
      {
        cout << "Breakpoint " << itr->first << ": Line " << itr->second << endl;
      }
    }
    else
    {
      cout << "Invalid info command: " << args[1] << endl;
    }
    return false;
  }

  // Kernel invocation information
  cout
    << dec
    << "Running kernel '" << m_kernelInvocation->getKernel()->getName() << "'"
    << endl
    << "-> Global work size:   " << m_kernelInvocation->getGlobalSize()
    << endl
    << "-> Global work offset: " << m_kernelInvocation->getGlobalOffset()
    << endl
    << "-> Local work size:    " << m_kernelInvocation->getLocalSize()
    << endl;

  // Current work-item
  const WorkItem *workItem = m_kernelInvocation->getCurrentWorkItem();
  if (workItem)
  {
    cout << endl << "Current work-item: " << workItem->getGlobalID() << endl;
    if (workItem->getState() == WorkItem::FINISHED)
    {
      cout << "Work-item has finished." << endl;
    }
    else
    {
      cout << "In function ";
      printFunction(workItem->getCurrentInstruction());
      printCurrentLine();
    }
  }
  else
  {
    cout << "All work-items finished." << endl;
  }

  return false;
}

bool InteractiveDebugger::list(vector<string> args)
{
  const WorkItem *workItem = m_kernelInvocation->getCurrentWorkItem();
  if (!workItem)
  {
    cout << "All work-items finished." << endl;
    return false;
  }
  if (!m_program->getNumSourceLines())
  {
    cout << "No source code available." << endl;
    return false;
  }

  // Check for an argument
  size_t start = 0;
  bool forwards = true;
  if (args.size() > 1)
  {
    if (args[1] == "-")
    {
      forwards = false;
    }
    else
    {
      // Parse argument as a target line number
      istringstream ss(args[1]);
      ss >> start;
      if (!ss.eof())
      {
        cout << "Invalid line number." << endl;
        return false;
      }
      start = start > LIST_LENGTH/2 ? start - LIST_LENGTH/2 : 1;
    }
  }

  if (!start)
  {
    if (forwards)
    {
      // Starting position is the previous list position + LIST_LENGTH
      start = m_listPosition ?
        m_listPosition + LIST_LENGTH : getCurrentLineNumber() + 1;
      if (start >= m_program->getNumSourceLines() + 1)
      {
        m_listPosition = m_program->getNumSourceLines() + 1;
        return false;
      }
    }
    else
    {
      // Starting position is the previous list position - LIST_LENGTH
      start = m_listPosition ? m_listPosition : getCurrentLineNumber();
      start = start > LIST_LENGTH ? start - LIST_LENGTH : 1;
    }
  }

  // Display lines
  for (int i = 0; i < LIST_LENGTH; i++)
  {
    if (start + i >= m_program->getNumSourceLines() + 1)
    {
      break;
    }
    printSourceLine(start + i);
  }

  m_listPosition = start;

  return false;
}

bool InteractiveDebugger::mem(vector<string> args)
{
  // Get target memory object
  Memory *memory = NULL;
  if (args[0][0] == 'g')
  {
    memory = m_context->getGlobalMemory();
  }
  else if (args[0][0] == 'l')
  {
    memory = m_kernelInvocation->getCurrentWorkGroup()->getLocalMemory();
  }
  else if (args[0][0] == 'p')
  {
    memory = m_kernelInvocation->getCurrentWorkItem()->getPrivateMemory();
  }

  // If no arguments, dump memory
  if (args.size() == 1)
  {
    memory->dump();
    return false;
  }
  else if (args.size() > 3)
  {
    cout << "Invalid number of arguments." << endl;
    return false;
  }

  // Get target address
  size_t address;
  stringstream ss(args[1]);
  ss >> hex >> address;
  if (!ss.eof() || address%4 != 0)
  {
    cout << "Invalid address." << endl;
    return false;
  }

  // Get optional size
  size_t size = 8;
  if (args.size() == 3)
  {
    stringstream ss(args[2]);
    ss >> dec >> size;
    if (!ss.eof() || !size)
    {
      cout << "Invalid size" << endl;
      return false;
    }
  }

  // Check address is valid
  if (!memory->isAddressValid(address, size))
  {
    cout << "Invalid memory address." << endl;
    return false;
  }

  // Output data
  unsigned char *data = (unsigned char*)memory->getPointer(address);
  for (unsigned i = 0; i < size; i++)
  {
    if (i%4 == 0)
    {
      cout << endl << hex << uppercase
           << setw(16) << setfill(' ') << right
           << (address + i) << ":";
    }
    cout << " " << hex << uppercase << setw(2) << setfill('0') << (int)data[i];
  }
  cout << endl << endl;

  return false;
}

bool InteractiveDebugger::next(vector<string> args)
{
  const WorkItem *workItem = m_kernelInvocation->getCurrentWorkItem();
  if (!workItem)
  {
    cout << "All work-items finished." << endl;
    return false;
  }

  if (workItem->getState() == WorkItem::FINISHED)
  {
    cout << "Work-item has finished." << endl;
    return false;
  }
  else if (workItem->getState() == WorkItem::BARRIER)
  {
    cout << "Work-item is at barrier." << endl;
    return false;
  }

  // Step until we return to the same depth
  m_previousDepth = workItem->getCallStack().size();
  m_previousLine = getCurrentLineNumber();
  m_next = true;

  return true;
}

bool InteractiveDebugger::print(vector<string> args)
{
  if (args.size() < 2)
  {
    cout << "Variable name(s) required." << endl;
    return false;
  }

  const WorkItem *workItem = m_kernelInvocation->getCurrentWorkItem();
  for (unsigned i = 1; i < args.size(); i++)
  {
    cout << args[i] << " = ";
    try
    {
      workItem->printExpression(args[i]);
    }
    catch (FatalError err)
    {
      cout << "fatal error: " << err.what();
    }
    cout << endl;
  }

  return false;
}

bool InteractiveDebugger::quit(vector<string> args)
{
#if !defined(_WIN32) || defined(__MINGW32__)
  // Restore old signal handler
  sigaction(SIGINT, &m_oldSignalHandler, NULL);
#endif

  m_running = false;
  return true;
}

bool InteractiveDebugger::step(vector<string> args)
{
  const WorkItem *workItem = m_kernelInvocation->getCurrentWorkItem();
  if (!workItem)
  {
    cout << "All work-items finished." << endl;
    return false;
  }

  if (workItem->getState() == WorkItem::FINISHED)
  {
    cout << "Work-item has finished." << endl;
    return false;
  }
  else if (workItem->getState() == WorkItem::BARRIER)
  {
    cout << "Work-item is at barrier." << endl;
    return false;
  }

  // Save current position
  m_previousDepth = workItem->getCallStack().size();
  m_previousLine = getCurrentLineNumber();

  return true;
}

bool InteractiveDebugger::workitem(vector<string> args)
{
  // TODO: Take offsets into account?
  Size3 gid(0,0,0);
  for (unsigned i = 1; i < args.size(); i++)
  {
    // Parse argument as a target line number
    istringstream ss(args[i]);
    ss >> gid[i-1];
    if (!ss.eof() || gid[i-1] >= m_kernelInvocation->getGlobalSize()[i-1])
    {
      cout << "Invalid global ID." << endl;
      return false;
    }
  }

  // Ugly const_cast since this operation actually changes something about
  // the simulation. This goes against the idea that plugins are entirely
  // passive.
  if (!const_cast<KernelInvocation*>(m_kernelInvocation)->switchWorkItem(gid))
  {
    cout << "Work-item has already finished, unable to load state." << endl;
    return false;
  }

  // Print new WI id
  cout << "Switched to work-item: (" << gid[0] << ","
                                     << gid[1] << ","
                                     << gid[2] << ")" << endl;
  if (m_kernelInvocation->getCurrentWorkItem()->getState() ==
      WorkItem::FINISHED)
  {
    cout << "Work-item has finished execution." << endl;
  }
  else
  {
    printCurrentLine();
  }

  return false;
}
