// Device.cpp (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"
#include <istream>
#include <iterator>
#include <sstream>

#include "llvm/DebugInfo.h"
#include "llvm/Instructions.h"

#include "Kernel.h"
#include "Memory.h"
#include "Device.h"
#include "Program.h"
#include "WorkGroup.h"
#include "WorkItem.h"

using namespace spirsim;
using namespace std;

#define LIST_LENGTH 10

// Compute flattened 1D index from 3D index and sizes
#define INDEX(id, num) (id[0] + (id[1] + id[2]*num[1])*num[0])

Device::Device()
{
  m_globalMemory = new Memory();
  m_interactive = false;
  m_nextBreakpoint = 1;

  // Check for interactive environment variable
  const char *env = getenv("OCLGRIND_INTERACTIVE");
  if (env && strcmp(env, "1") == 0)
  {
    m_interactive = true;
  }

  // Set-up interactive commands
#define ADD_CMD(name, sname, func)  \
  m_commands[name] = &Device::func; \
  m_commands[sname] = &Device::func;
  ADD_CMD("backtrace",    "bt", backtrace);
  ADD_CMD("break",        "b",  brk);
  ADD_CMD("continue",     "c",  cont);
  ADD_CMD("delete",       "d",  del);
  ADD_CMD("gmem",         "gm", mem);
  ADD_CMD("help",         "h",  help);
  ADD_CMD("info",         "i",  info);
  ADD_CMD("list",         "l",  list);
  ADD_CMD("lmem",         "lm", mem);
  ADD_CMD("next",         "n", next);
  ADD_CMD("pmem",         "pm", mem);
  ADD_CMD("print",        "p",  print);
  ADD_CMD("quit",         "q",  quit);
  ADD_CMD("step",         "s",  step);
  ADD_CMD("workitem",     "wi", workitem);
}

Device::~Device()
{
  delete m_globalMemory;
}

WorkGroup* Device::createWorkGroup(size_t x, size_t y, size_t z)
{
  return new WorkGroup(
    this, *m_kernel, *m_globalMemory, m_workDim, x, y, z,
    m_globalOffset, m_globalSize, m_localSize);
}

size_t Device::getCurrentLineNumber() const
{
  if (!m_currentWorkItem || m_currentWorkItem->getState() == WorkItem::FINISHED)
  {
    return 0;
  }
  return getLineNumber(m_currentWorkItem->getCurrentInstruction());
}

Memory* Device::getGlobalMemory() const
{
  return m_globalMemory;
}

size_t Device::getLineNumber(const llvm::Instruction *instruction) const
{
  llvm::MDNode *md = instruction->getMetadata("dbg");
  if (md)
  {
    llvm::DILocation loc(md);
    return loc.getLineNumber();
  }
  return 0;
}

bool Device::isInteractive() const
{
  return m_interactive;
}

bool Device::nextWorkItem()
{
  if (m_currentWorkGroup)
  {
    // Switch to next ready work-item
    m_currentWorkItem = m_currentWorkGroup->getNextWorkItem();
    if (m_currentWorkItem)
    {
      return true;
    }

    // No work-items in READY state
    // Check if there are work-items at a barrier
    if (m_currentWorkGroup->hasBarrier())
    {
      // Resume execution
      m_currentWorkGroup->clearBarrier();
      m_currentWorkItem = m_currentWorkGroup->getNextWorkItem();
      return true;
    }

    // All work-items must have finished, destroy work-group
    delete m_currentWorkGroup;
    m_currentWorkGroup = NULL;
  }

  // Switch to next work-group
  if (!m_runningGroups.empty())
  {
    // Take work-group from running pool
    m_currentWorkGroup = m_runningGroups.front();
    m_runningGroups.pop_front();
  }
  else if (!m_pendingGroups.empty())
  {
    // Take work-group from pending pool
    PendingWorkGroup group = m_pendingGroups.front();
    m_pendingGroups.pop_front();
    m_currentWorkGroup = createWorkGroup(group[0], group[1], group[2]);
  }
  else
  {
    return false;
  }

  m_currentWorkItem = m_currentWorkGroup->getNextWorkItem();

  // Check if this work-group has already finished
  if (!m_currentWorkItem)
  {
    return nextWorkItem();
  }

  return true;
}

void Device::printErrorContext() const
{
  // Work item
  if (m_currentWorkItem)
  {
    const size_t *gid = m_currentWorkItem->getGlobalID();
    const size_t *lid = m_currentWorkItem->getLocalID();
    cerr << "\tWork-item:  Global(" << dec
         << gid[0] << ","
         << gid[1] << ","
         << gid[2] << ")"
         << " Local(" << dec
         << lid[0] << ","
         << lid[1] << ","
         << lid[2] << ")"
         << endl;
  }

  // Work group
  if (m_currentWorkGroup)
  {
    const size_t *group = m_currentWorkGroup->getGroupID();
    cerr << "\tWork-group: (" << dec
         << group[0] << ","
         << group[1] << ","
         << group[2] << ")"
         << endl;
  }

  // Kernel
  cerr << "\tKernel:     " << m_kernel->getName() << endl;

  // Instruction
  if (m_currentWorkItem)
  {
    cerr << "\t";
    const llvm::Instruction *instruction =
      m_currentWorkItem->getCurrentInstruction();
    dumpInstruction(cerr, *instruction);
    cerr << endl;

    // Output debug information
    cerr << "\t";
    llvm::MDNode *md = instruction->getMetadata("dbg");
    if (!md)
    {
      cerr << "Debugging information not available." << endl;
    }
    else
    {
      llvm::DILocation loc(md);
      cerr << "At line " << dec << loc.getLineNumber()
           << " of " << loc.getFilename().str() << endl;
    }
  }

  cerr << endl;
}

void Device::notifyMemoryError(bool read, unsigned int addrSpace,
                               size_t address, size_t size)
{
  string memType;
  switch (addrSpace)
  {
  case AddrSpacePrivate:
    memType = "private";
    break;
  case AddrSpaceGlobal:
    memType = "global";
    break;
  case AddrSpaceConstant:
    memType = "constant";
    break;
  case AddrSpaceLocal:
    memType = "local";
    break;
  default:
    assert(false && "Memory error in unsupported address space.");
    break;
  }

  // Error info
  cerr << endl << "Invalid " << (read ? "read" : "write")
       << " of size " << size
       << " at " << memType
       << " memory address " << hex << address << endl;

  printErrorContext();

  m_forceBreak = true;
}

void Device::run(Kernel& kernel, unsigned int workDim,
                 const size_t *globalOffset,
                 const size_t *globalSize,
                 const size_t *localSize)
{
  assert(m_runningGroups.empty());

  // Set-up offsets and sizes
  m_workDim = workDim;
  m_globalSize[0] = m_globalSize[1] = m_globalSize[2] = 1;
  m_globalOffset[0] = m_globalOffset[1] = m_globalOffset[2] = 0;
  m_localSize[0] = m_localSize[1] = m_localSize[2] = 1;
  for (int i = 0; i < workDim; i++)
  {
    m_globalSize[i] = globalSize[i];
    if (globalOffset[i])
    {
      m_globalOffset[i] = globalOffset[i];
    }
    if (localSize[i])
    {
      m_localSize[i] = localSize[i];
    }
  }

  try
  {
    // Allocate and initialise constant memory
    kernel.allocateConstants(m_globalMemory);
  }
  catch (FatalError& err)
  {
    cerr << endl << "OCLGRIND FATAL ERROR "
         << "(" << err.getFile() << ":" << err.getLine() << ")"
         << endl << err.what()
         << endl << "When allocating kernel constants for '"
         << kernel.getName() << "'"
         << endl;
    return;
  }

  // Create pool of pending work-groups
  m_numGroups[0] = m_globalSize[0]/m_localSize[0];
  m_numGroups[1] = m_globalSize[1]/m_localSize[1];
  m_numGroups[2] = m_globalSize[2]/m_localSize[2];
  for (size_t k = 0; k < m_numGroups[2]; k++)
  {
    for (size_t j = 0; j < m_numGroups[1]; j++)
    {
      for (size_t i = 0; i < m_numGroups[0]; i++)
      {
        PendingWorkGroup workGroup = {{i, j, k}};
        m_pendingGroups.push_back(workGroup);
      }
    }
  }

  // Prepare kernel invocation
  m_program = &kernel.getProgram();
  m_kernel = &kernel;
  m_listPosition = 0;
  m_currentWorkGroup = NULL;
  m_currentWorkItem = NULL;
  nextWorkItem();

  try
  {
    // Check if we're in interactive mode
    if (m_interactive)
    {
      m_running = true;

      // Get source code (if available) and split into lines
      string source = m_program->getSource();
      m_sourceLines.clear();
      if (!source.empty())
      {
        std::stringstream ss(source);
        std::string line;
        while(std::getline(ss, line, '\n'))
        {
          m_sourceLines.push_back(line);
        }
      }

      cout << endl;
      info(vector<string>());
    }
    else
    {
      // If not, just run kernel
      cont(vector<string>());
      m_running = false;
    }

    // Interactive debugging loop
    while (m_running)
    {
      // Prompt for command
      string cmd;
      cout << "(oclgrind) " << std::flush;
      getline(cin, cmd);

      // Split command into tokens
      vector<string> tokens;
      istringstream iss(cmd);
      copy(istream_iterator<string>(iss),
           istream_iterator<string>(),
           back_inserter< vector<string> >(tokens));

      // Check for end of stream or empty command
      if (cin.eof())
      {
        cout << "(quit)" << endl;
        quit(tokens);
      }
      if (tokens.size() == 0)
      {
        continue;
      }

      // Find command in map and execute
      map<string,Command>::iterator itr = m_commands.find(tokens[0]);
      if (itr != m_commands.end())
      {
        (this->*itr->second)(tokens);
      }
      else
      {
        cout << "Unrecognized command '" << tokens[0] << "'" << endl;
      }
    }
  }
  catch (FatalError& err)
  {
    cerr << endl << "OCLGRIND FATAL ERROR "
         << "(" << err.getFile() << ":" << err.getLine() << ")"
         << endl << err.what() << endl;
    printErrorContext();
  }

  // Destroy any remaining work-groups
  while (!m_runningGroups.empty())
  {
    delete m_runningGroups.front();
    m_runningGroups.pop_front();
  }
  if (m_currentWorkGroup)
  {
    delete m_currentWorkGroup;
    m_currentWorkGroup = NULL;
  }

  // Deallocate constant memory
  kernel.deallocateConstants(m_globalMemory);
}

void Device::printCurrentLine() const
{
  if (!m_currentWorkItem)
  {
    return;
  }
  if (m_currentWorkItem->getState() == WorkItem::FINISHED)
  {
    return;
  }

  size_t lineNum = getCurrentLineNumber();
  if (!m_sourceLines.empty() && lineNum > 0)
  {
    printSourceLine(lineNum);
  }
  else
  {
    cout << "Source line not available." << endl;
    dumpInstruction(cout, *m_currentWorkItem->getCurrentInstruction());
    cout << endl;
  }
}

void Device::printFunction(const llvm::Instruction *instruction) const
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
    m_currentWorkItem->printValue(argItr);
  }

  cout << ") at line " << dec << getLineNumber(instruction) << endl;
}

void Device::printSourceLine(size_t lineNum) const
{
  if (lineNum && lineNum <= m_sourceLines.size())
  {
    cout << dec << lineNum << "\t" << m_sourceLines[lineNum-1] << endl;
  }
  else
  {
    cout << "Invalid line number: " << lineNum << endl;
  }
}

void Device::step()
{
  if (m_currentWorkItem->getState() == WorkItem::BARRIER)
  {
    cout << "Work-item is at a barrier." << endl;
    return;
  }
  else if (m_currentWorkItem->getState() == WorkItem::FINISHED)
  {
    cout << "Work-item has finished execution." << endl;
    return;
  }

  // Step whole source lines, if available
  size_t prevLine = getCurrentLineNumber();
  size_t currLine = prevLine;
  WorkItem::State state = m_currentWorkItem->getState();
  do
  {
    state = m_currentWorkItem->step();
    if (state != WorkItem::READY)
    {
      break;
    }
    currLine = getCurrentLineNumber();
  }
  while (!m_sourceLines.empty() && (currLine == prevLine || currLine == 0));
}


////////////////////////////////
//// Interactive Debugging  ////
////////////////////////////////

void Device::backtrace(vector<string> args)
{
  if (!m_currentWorkItem || m_currentWorkItem->getState() == WorkItem::FINISHED)
  {
    return;
  }

  stack<ReturnAddress> callStack = m_currentWorkItem->getCallStack();

  // Print current instruction
  cout << "#" << callStack.size() <<  " ";
  printFunction(m_currentWorkItem->getCurrentInstruction());

  // Print call stack
  while (!callStack.empty())
  {
    cout << "#" << (callStack.size()-1) <<  " ";
    printFunction(callStack.top().second);
    callStack.pop();
  }
}

void Device::brk(vector<string> args)
{
  if (m_sourceLines.empty())
  {
    cout << "Breakpoints only valid when source is available." << endl;
    return;
  }

  size_t lineNum = getCurrentLineNumber();
  if (args.size() > 1)
  {
    // Parse argument as a target line number
    istringstream ss(args[1]);
    ss >> lineNum;
    if (!ss.eof() || !lineNum || lineNum > m_sourceLines.size()+1)
    {
      cout << "Invalid line number." << endl;
      return;
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
}

void Device::cont(vector<string> args)
{
  bool canBreak = false;
  m_forceBreak = false;
  m_running = true;
  static size_t lastBreakLine = 0;
  while (m_currentWorkItem && m_running)
  {
    // Run current work-item as far as possible
    while (m_currentWorkItem->getState() == WorkItem::READY && m_running)
    {
      m_currentWorkItem->step();

      if (!m_interactive)
      {
        continue;
      }

      if (m_forceBreak)
      {
        m_listPosition = 0;
        m_forceBreak = false;
        return;
      }

      if (!m_breakpoints.empty())
      {
        if (!canBreak)
        {
          // Check if we have passed over the previous breakpoint
          if (getCurrentLineNumber() != lastBreakLine)
          {
            canBreak = true;
          }
          else
          {
            continue;
          }
        }

        // Check if we're at a breakpoint
        size_t line = getCurrentLineNumber();
        map<size_t, size_t>::iterator itr;
        for (itr = m_breakpoints[m_program].begin();
             itr != m_breakpoints[m_program].end(); itr++)
        {
          if (itr->second == line)
          {
            const size_t *gid = m_currentWorkItem->getGlobalID();
            cout << "Breakpoint " << itr->first
                 << " hit at line " << itr->second
                 << " by work-item (" << gid[0] << ","
                                      << gid[1] << ","
                                      << gid[2] << ")" << endl;
            printCurrentLine();
            lastBreakLine = line;
            m_listPosition = 0;
            return;
          }
        }
      }
    }

    nextWorkItem();
  }
  m_running = false;
}

void Device::del(vector<string> args)
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
      return;
    }

    // Ensure breakpoint exists
    if (!m_breakpoints[m_program].count(bpNum))
    {
      cout << "Breakpoint not found." << endl;
      return;
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
}

void Device::help(vector<string> args)
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
    return;
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
    cout << "Print the values of one or more variables." << endl;
  }
  else if (args[1] == "quit" || args[1] == "q")
  {
    cout << "Quit interactive debugger "
        << "(and terminate current kernel invocation)." << endl;
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
}

void Device::info(vector<string> args)
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
    return;
  }

  // Kernel invocation information
  cout << "Running kernel '" << m_kernel->getName() << "'" << endl
       << "-> Global work size:   (" << m_globalSize[0] << ","
                                     << m_globalSize[1] << ","
                                     << m_globalSize[2] << ")" << endl
       << "-> Global work offset: (" << m_globalOffset[0] << ","
                                     << m_globalOffset[1] << ","
                                     << m_globalOffset[2] << ")" << endl
       << "-> Local work size:    (" << m_localSize[0] << ","
                                     << m_localSize[1] << ","
                                     << m_localSize[2] << ")" << endl;

  // Current work-item
  if (m_currentWorkItem)
  {
    const size_t *gid = m_currentWorkItem->getGlobalID();
    cout << endl << "Current work-item: (" << gid[0] << ","
                                           << gid[1] << ","
                                           << gid[2] << ")" << endl;
    printCurrentLine();
  }
  else
  {
    cout << "All work-items finished." << endl;
  }
}

void Device::list(vector<string> args)
{
  if (!m_currentWorkItem)
  {
    cout << "All work-items finished." << endl;
    return;
  }
  if (m_sourceLines.empty())
  {
    cout << "No source code available." << endl;
    return;
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
        return;
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
      if (start >= m_sourceLines.size() + 1)
      {
        m_listPosition = m_sourceLines.size() + 1;
        return;
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
    if (start + i >= m_sourceLines.size() + 1)
    {
      break;
    }
    printSourceLine(start + i);
  }

  m_listPosition = start;
}

void Device::mem(vector<string> args)
{
  // Get target memory object
  Memory *memory = NULL;
  if (args[0][0] == 'g')
  {
    memory = m_globalMemory;
  }
  else if (args[0][0] == 'l')
  {
    memory = m_currentWorkGroup->getLocalMemory();
  }
  else if (args[0][0] == 'p')
  {
    memory = m_currentWorkItem->getPrivateMemory();
  }

  // If no arguments, dump memory
  if (args.size() == 1)
  {
    memory->dump();
    return;
  }
  else if (args.size() > 3)
  {
    cout << "Invalid number of arguments." << endl;
    return;
  }

  // Get target address
  size_t address;
  stringstream ss(args[1]);
  ss >> hex >> address;
  if (!ss.eof() || address%4 != 0)
  {
    cout << "Invalid address." << endl;
    return;
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
      return;
    }
  }

  // Read data from memory
  unsigned char *data = new unsigned char[size];
  if (!memory->load(data, address, size))
  {
    cout << "Failed to read data from memory." << endl;
    return;
  }

  // Output data
  for (int i = 0; i < size; i++)
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

  delete[] data;
}

void Device::next(vector<string> args)
{
  if (!m_currentWorkItem)
  {
    cout << "All work-items finished." << endl;
    return;
  }

  // Step until we return to the same depth
  size_t prevDepth = m_currentWorkItem->getCallStack().size();
  do
  {
    step();
  }
  while (m_currentWorkItem->getCallStack().size() > prevDepth);

  // Print function if changed
  if (prevDepth != m_currentWorkItem->getCallStack().size() &&
      m_currentWorkItem->getState() != WorkItem::FINISHED)
  {
    printFunction(m_currentWorkItem->getCurrentInstruction());
  }

  printCurrentLine();
  m_listPosition = 0;
}

void Device::print(vector<string> args)
{
  if (args.size() < 2)
  {
    cout << "Variable name(s) required." << endl;
    return;
  }

  for (int i = 1; i < args.size(); i++)
  {
    cout << args[i] << " = ";

    // Check for subscript operator
    size_t start = args[i].find("[");
    if (start != -1)
    {
      // Find end of subscript
      size_t end = args[i].find(']');
      if (end == -1)
      {
        cout << "missing ']'" << endl;
        return;
      }
      if (end != args[i].length() - 1)
      {
        cout << "invalid variable" << endl;
        return;
      }

      // Parse index value
      size_t index = 0;
      string var = args[i].substr(0, start);
      stringstream ss(args[i].substr(start+1, end-start-1));
      ss >> index;
      if (!ss.eof())
      {
        cout << "invalid index" << endl;
        return;
      }

      // Get variable value and type
      const llvm::Value *ptr = m_currentWorkItem->getVariable(var);
      if (!ptr)
      {
        cout << "not found" << endl;
        return;
      }
      const llvm::Type *ptrType = ptr->getType();

      // Check for alloca instruction, in which case look at allocated type
      bool alloca = false;
      if (ptr->getValueID() >= llvm::Value::InstructionVal &&
          ((llvm::Instruction*)ptr)->getOpcode() == llvm::Instruction::Alloca)
      {
        ptrType = ((const llvm::AllocaInst*)ptr)->getAllocatedType();
        alloca = true;
      }

      // Ensure type is a pointer
      if (!ptrType->isPointerTy())
      {
        cout << "not a pointer" << endl;
        return;
      }

      // Get base address
      size_t base = *(size_t*)m_currentWorkItem->getValueData(ptr);
      if (alloca)
      {
        // Load base address from private memory
        m_currentWorkItem->getPrivateMemory()->load((unsigned char*)&base,
                                                    base, sizeof(size_t));
      }

      // Get target memory object
      Memory *memory = NULL;
      switch (ptrType->getPointerAddressSpace())
      {
      case AddrSpacePrivate:
        memory = m_currentWorkItem->getPrivateMemory();
        break;
      case AddrSpaceGlobal:
      case AddrSpaceConstant:
        memory = m_globalMemory;
        break;
      case AddrSpaceLocal:
        memory = m_currentWorkGroup->getLocalMemory();
        break;
      default:
        cout << "invalid address space" << endl;
        return;
      }

      // Get element type
      const llvm::Type *elemType = ptrType->getPointerElementType();
      size_t elemSize = getTypeSize(elemType);

      // Load data
      unsigned char *data = new unsigned char[elemSize];
      if (!memory->load(data, base + index*elemSize, elemSize))
      {
        cout << "failed to load data" << endl;
      }
      else
      {
        // Print data
        printTypedData(elemType, data);
        cout << endl;
      }
      delete[] data;
    }
    else
    {
      if (!m_currentWorkItem->printVariable(args[i]))
      {
        cout << "not found";
      }
      cout << endl;
    }
  }
}

void Device::quit(vector<string> args)
{
  m_interactive = false;
  m_running = false;
  m_breakpoints.clear();
}

void Device::step(vector<string> args)
{
  if (!m_currentWorkItem)
  {
    cout << "All work-items finished." << endl;
    return;
  }

  // Get current call stack depth
  size_t prevDepth = m_currentWorkItem->getCallStack().size();

  step();

  // Print function if changed
  if (prevDepth != m_currentWorkItem->getCallStack().size() &&
      m_currentWorkItem->getState() != WorkItem::FINISHED)
  {
    printFunction(m_currentWorkItem->getCurrentInstruction());
  }

  printCurrentLine();
  m_listPosition = 0;
}

void Device::workitem(vector<string> args)
{
  // TODO: Take offsets into account?
  size_t gid[3] = {0,0,0};
  for (int i = 1; i < args.size(); i++)
  {
    // Parse argument as a target line number
    istringstream ss(args[i]);
    ss >> gid[i-1];
    if (!ss.eof() || gid[i-1] >= m_globalSize[i-1])
    {
      cout << "Invalid global ID." << endl;
      return;
    }
  }

  // Compute work-group ID
  size_t group[3] =
  {
    gid[0]/m_localSize[0],
    gid[1]/m_localSize[1],
    gid[2]/m_localSize[2]
  };

  bool found = false;
  WorkGroup *previousWorkGroup = m_currentWorkGroup;

  // Check if we're already running the work-group
  const size_t *_group = m_currentWorkGroup->getGroupID();
  if (group[0] == _group[0] &&
      group[1] == _group[1] &&
      group[2] == _group[2])
  {
    found = true;
  }

  // Check if work-group is in running pool
  if (!found)
  {
    std::list<WorkGroup*>::iterator rItr;
    for (rItr = m_runningGroups.begin(); rItr != m_runningGroups.end(); rItr++)
    {
      const size_t *_group = (*rItr)->getGroupID();
      if (group[0] == _group[0] &&
          group[1] == _group[1] &&
          group[2] == _group[2])
      {
        m_currentWorkGroup = *rItr;
        m_runningGroups.erase(rItr);
        found = true;
        break;
      }
    }
  }

  // Check if work-group is in pending pool
  if (!found)
  {
    std::list<PendingWorkGroup>::iterator pItr;
    for (pItr = m_pendingGroups.begin(); pItr != m_pendingGroups.end(); pItr++)
    {
      const size_t *_group = pItr->group;
      if (group[0] == _group[0] &&
          group[1] == _group[1] &&
          group[2] == _group[2])
      {
        m_currentWorkGroup = createWorkGroup(group[0], group[1], group[2]);
        m_pendingGroups.erase(pItr);
        found = true;
        break;
      }
    }
  }

  if (!found)
  {
    cout << "Work-item has already finished, unable to load state." << endl;
    return;
  }

  if (previousWorkGroup != m_currentWorkGroup)
  {
    m_runningGroups.push_back(previousWorkGroup);
  }

  // Get work-item
  size_t lid[3] =
  {
    gid[0]%m_localSize[0],
    gid[1]%m_localSize[1],
    gid[2]%m_localSize[2]
  };
  m_currentWorkItem = m_currentWorkGroup->getWorkItem(lid);

  // Print new WI id
  cout << "Switched to work-item: (" << gid[0] << ","
                                     << gid[1] << ","
                                     << gid[2] << ")" << endl;
  if (m_currentWorkItem->getState() == WorkItem::FINISHED)
  {
    cout << "Work-item has finished execution." << endl;
  }
  else
  {
    printCurrentLine();
  }
}
