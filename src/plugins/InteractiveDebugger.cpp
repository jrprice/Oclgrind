#include "core/common.h"

#include <iterator>
#include <sstream>

#if HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "llvm/DebugInfo.h"
#include "llvm/Function.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"

#include "InteractiveDebugger.h"
#include "core/Context.h"
#include "core/Device.h"
#include "core/Kernel.h"
#include "core/Memory.h"
#include "core/Program.h"
#include "core/WorkGroup.h"
#include "core/WorkItem.h"

using namespace oclgrind;
using namespace std;

#define LIST_LENGTH 10

InteractiveDebugger::InteractiveDebugger(const Context *context)
  : Plugin(context)
{
  m_running          = true;
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

  while (true)
  {
    // Prompt for command
    bool eof = false;
    string cmd;
  #if HAVE_READLINE
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
  #else
    cout << "(oclgrind) " << flush;
    getline(cin, cmd);
    eof = cin.eof();
  #endif

    // Quit on EOF
    if (eof)
    {
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

void InteractiveDebugger::kernelBegin(const KernelInvocation *kernelInvocation)
{
  // Get source code (if available) and split into lines
  m_kernelInvocation = kernelInvocation;
  m_program = kernelInvocation->kernel->getProgram();
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

  m_continue      = false;
  m_lastBreakLine = 0;
  m_listPosition  = 0;
  m_next          = false;
  m_previousDepth = 0;
  m_previousLine  = 0;
}

void InteractiveDebugger::kernelEnd(const KernelInvocation *kernelInvocation)
{
  m_kernelInvocation = NULL;
}

///////////////////////////
//// Utility Functions ////
///////////////////////////

size_t InteractiveDebugger::getCurrentLineNumber() const
{
  const WorkItem *workItem = m_context->getDevice()->getCurrentWorkItem();
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
    llvm::DILocation loc(md);
    return loc.getLineNumber();
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
      const size_t *gid =
        m_context->getDevice()->getCurrentWorkItem()->getGlobalID();
      cout << "Breakpoint " << itr->first
           << " hit at line " << itr->second
           << " by work-item (" << gid[0] << ","
                                << gid[1] << ","
                                << gid[2] << ")" << endl;
      m_lastBreakLine = line;
      m_listPosition = 0;
      return true;
    }
  }
  return false;
}

void InteractiveDebugger::printCurrentLine() const
{
  const WorkItem *workItem = m_context->getDevice()->getCurrentWorkItem();
  if (!workItem || workItem->getState() == WorkItem::FINISHED)
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
    dumpInstruction(cout, *workItem->getCurrentInstruction());
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
    m_context->getDevice()->getCurrentWorkItem()->printValue(argItr);
  }

  cout << ") at line " << dec << getLineNumber(instruction) << endl;
}

void InteractiveDebugger::printSourceLine(size_t lineNum) const
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

bool InteractiveDebugger::shouldShowPrompt(const WorkItem *workItem)
{
  if (!m_running)
    return false;

  if (hasHitBreakpoint())
    return true;

  if (m_continue)
    return false;

  if (workItem->getState() == WorkItem::BARRIER)
    return true;
  if (workItem->getState() == WorkItem::FINISHED)
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
  const WorkItem *workItem = m_context->getDevice()->getCurrentWorkItem();
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
  if (m_sourceLines.empty())
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
    if (!ss.eof() || !lineNum || lineNum > m_sourceLines.size()+1)
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
    << "Running kernel '" << m_kernelInvocation->kernel->getName() << "'"
    << endl
    << "-> Global work size:   (" << m_kernelInvocation->globalSize[0] << ","
                                  << m_kernelInvocation->globalSize[1] << ","
                                  << m_kernelInvocation->globalSize[2] << ")"
    << endl
    << "-> Global work offset: (" << m_kernelInvocation->globalOffset[0] << ","
                                  << m_kernelInvocation->globalOffset[1] << ","
                                  << m_kernelInvocation->globalOffset[2] << ")"
    << endl
    << "-> Local work size:    (" << m_kernelInvocation->localSize[0] << ","
                                  << m_kernelInvocation->localSize[1] << ","
                                  << m_kernelInvocation->localSize[2] << ")"
    << endl;

  // Current work-item
  const WorkItem *workItem = m_context->getDevice()->getCurrentWorkItem();
  if (workItem)
  {
    const size_t *gid = workItem->getGlobalID();
    cout << endl << "Current work-item: (" << gid[0] << ","
                                           << gid[1] << ","
                                           << gid[2] << ")" << endl;
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
  const WorkItem *workItem = m_context->getDevice()->getCurrentWorkItem();
  if (!workItem)
  {
    cout << "All work-items finished." << endl;
    return false;
  }
  if (m_sourceLines.empty())
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
      if (start >= m_sourceLines.size() + 1)
      {
        m_listPosition = m_sourceLines.size() + 1;
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
    if (start + i >= m_sourceLines.size() + 1)
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
    memory = m_context->getDevice()->getCurrentWorkGroup()->getLocalMemory();
  }
  else if (args[0][0] == 'p')
  {
    memory = m_context->getDevice()->getCurrentWorkItem()->getPrivateMemory();
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

  return false;
}

bool InteractiveDebugger::next(vector<string> args)
{
  const WorkItem *workItem = m_context->getDevice()->getCurrentWorkItem();
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

  const WorkItem *workItem = m_context->getDevice()->getCurrentWorkItem();
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
        return false;
      }
      if (end != args[i].length() - 1)
      {
        cout << "invalid variable" << endl;
        return false;
      }

      // Parse index value
      size_t index = 0;
      string var = args[i].substr(0, start);
      stringstream ss(args[i].substr(start+1, end-start-1));
      ss >> index;
      if (!ss.eof())
      {
        cout << "invalid index" << endl;
        return false;
      }

      // Get variable value and type
      const llvm::Value *ptr = workItem->getVariable(var);
      if (!ptr)
      {
        cout << "not found" << endl;
        return false;
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
        return false;
      }

      // Get base address
      size_t base = *(size_t*)workItem->getValueData(ptr);
      if (alloca)
      {
        // Load base address from private memory
        workItem->getPrivateMemory()->load((unsigned char*)&base,
                                                    base, sizeof(size_t));
      }

      // Get target memory object
      Memory *memory = NULL;
      switch (ptrType->getPointerAddressSpace())
      {
      case AddrSpacePrivate:
        memory = workItem->getPrivateMemory();
        break;
      case AddrSpaceGlobal:
      case AddrSpaceConstant:
        memory = m_context->getGlobalMemory();
        break;
      case AddrSpaceLocal:
        memory = m_context->getDevice()->getCurrentWorkGroup()->getLocalMemory();
        break;
      default:
        cout << "invalid address space" << endl;
        return false;
      }

      // Get element type
      const llvm::Type *elemType = ptrType->getPointerElementType();
      size_t elemSize = getTypeSize(elemType);

      // Load data
      if (!memory->isAddressValid(base + index*elemSize, elemSize))
      {
        cout << "invalid memory address" << endl;
      }
      else
      {
        // Print data
        void *data = (void*)memory->getPointer(base+index*elemSize);
        printTypedData(elemType, (unsigned char*)data);
        cout << endl;
      }
    }
    else
    {
      if (!workItem->printVariable(args[i]))
      {
        cout << "not found";
      }
      cout << endl;
    }
  }

  return false;
}

bool InteractiveDebugger::quit(vector<string> args)
{
  m_running = false;
  return true;
}

bool InteractiveDebugger::step(vector<string> args)
{
  const WorkItem *workItem = m_context->getDevice()->getCurrentWorkItem();
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
  size_t gid[3] = {0,0,0};
  for (int i = 1; i < args.size(); i++)
  {
    // Parse argument as a target line number
    istringstream ss(args[i]);
    ss >> gid[i-1];
    if (!ss.eof() || gid[i-1] >= m_kernelInvocation->globalSize[i-1])
    {
      cout << "Invalid global ID." << endl;
      return false;
    }
  }

  if (!m_context->getDevice()->switchWorkItem(gid))
  {
    cout << "Work-item has already finished, unable to load state." << endl;
    return false;
  }

  // Print new WI id
  cout << "Switched to work-item: (" << gid[0] << ","
                                     << gid[1] << ","
                                     << gid[2] << ")" << endl;
  if (m_context->getDevice()->getCurrentWorkItem()->getState() ==
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
