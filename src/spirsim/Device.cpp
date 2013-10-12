// Device.cpp (oclgrind)
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
#include <istream>
#include <iterator>
#include <sstream>

#include "Kernel.h"
#include "Memory.h"
#include "Device.h"
#include "WorkGroup.h"
#include "WorkItem.h"

using namespace spirsim;
using namespace std;

// Compute flattened 1D index from 3D index and sizes
#define INDEX(id, num) (id[0] + (id[1] + id[2]*num[1])*num[0])

Device::Device()
{
  m_globalMemory = new Memory();
  m_interactive = false;

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
  ADD_CMD("clear",        "cl", clear);
  ADD_CMD("continue",     "c",  cont);
  ADD_CMD("help",         "h",  help);
  ADD_CMD("info",         "i",  info);
  ADD_CMD("list",         "l",  list);
  ADD_CMD("print",        "p",  print);
  ADD_CMD("printglobal",  "pg", printglobal);
  ADD_CMD("printlocal",   "pl", printlocal);
  ADD_CMD("printprivate", "pp", printprivate);
  ADD_CMD("quit",         "q",  quit);
  ADD_CMD("step",         "s",  step);
  ADD_CMD("workitem",     "wi", workitem);
}

Device::~Device()
{
  delete m_globalMemory;
}

Memory* Device::getGlobalMemory() const
{
  return m_globalMemory;
}

bool Device::nextWorkItem()
{
  // Switch to next ready work-item
  m_currentWorkItem = m_currentWorkGroup->getNextWorkItem();
  if (m_currentWorkItem)
  {
    return true;
  }

  // Check if there are work-items at a barrier
  if (m_currentWorkGroup->hasBarrier())
  {
    // Resume execution
    m_currentWorkGroup->clearBarrier();
    m_currentWorkItem = m_currentWorkGroup->getNextWorkItem();
    return true;
  }

  // Switch to next work-group
  m_runningGroups.erase(m_currentWorkGroup);
  if (m_runningGroups.empty())
  {
    return false;
  }
  m_currentWorkGroup = *m_runningGroups.begin();
  m_currentWorkItem = m_currentWorkGroup->getNextWorkItem();
  return true;
}

void Device::run(Kernel& kernel, unsigned int workDim,
                 const size_t *globalOffset,
                 const size_t *globalSize,
                 const size_t *localSize)
{
  assert(m_runningGroups.empty());

  // Set-up offsets and sizes
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

  // Allocate and initialise constant memory
  kernel.allocateConstants(m_globalMemory);

  // Prepare kernel invocation
  m_kernel = &kernel;
  m_numGroups[0] = m_globalSize[0]/m_localSize[0];
  m_numGroups[1] = m_globalSize[1]/m_localSize[1];
  m_numGroups[2] = m_globalSize[2]/m_localSize[2];
  m_workGroups = new WorkGroup*[m_numGroups[0]*m_numGroups[1]*m_numGroups[2]];
  for (int k = 0; k < m_numGroups[2]; k++)
  {
    for (int j = 0; j < m_numGroups[1]; j++)
    {
      for (int i = 0; i < m_numGroups[0]; i++)
      {
        WorkGroup *workGroup =
          new WorkGroup(kernel, *m_globalMemory, workDim, i, j, k,
                        m_globalOffset, m_globalSize, m_localSize);
        m_workGroups[i + (k*m_numGroups[1] + j)*m_numGroups[0]] = workGroup;
        m_runningGroups.insert(workGroup);
      }
    }
  }

  m_currentWorkGroup = m_workGroups[0];
  m_currentWorkItem = m_currentWorkGroup->getNextWorkItem();

  // Check if we're in interactive mode
  if (m_interactive)
  {
    m_running = true;
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

  // Destroy work-groups
  for (int i = 0; i < m_numGroups[0]*m_numGroups[1]*m_numGroups[2]; i++)
  {
    delete m_workGroups[i];
  }
  delete[] m_workGroups;

  // Deallocate constant memory
  kernel.deallocateConstants(m_globalMemory);
}


////////////////////////////////
//// Interactive Debugging  ////
////////////////////////////////

void Device::backtrace(vector<string> args)
{
  // TODO: Implement
  cout << "Unimplemented command 'backtrace'" << endl;
}

void Device::brk(vector<string> args)
{
  // TODO: Implement
  cout << "Unimplemented command 'brk'" << endl;
}

void Device::clear(vector<string> args)
{
  // TODO: Implement
  cout << "Unimplemented command 'clear'" << endl;
}

void Device::cont(vector<string> args)
{
  while (m_currentWorkItem)
  {
    // Run current work-item as far as possible
    while (m_currentWorkItem->step() == WorkItem::READY);

    nextWorkItem();
  }
  m_running = false;
}

void Device::help(vector<string> args)
{
  if (args.size() < 2)
  {
    cout << "Command list:" << endl;
    cout << "  backtrace    (bt)" << endl;
    cout << "  break        (b)" << endl;
    cout << "  clear        (cl)" << endl;
    cout << "  continue     (c)" << endl;
    cout << "  help         (h)" << endl;
    cout << "  info         (i)" << endl;
    cout << "  list         (l)" << endl;
    cout << "  print        (p)" << endl;
    cout << "  printglobal  (pg)" << endl;
    cout << "  printlocal   (pl)" << endl;
    cout << "  printprivate (pp)" << endl;
    cout << "  quit         (q)" << endl;
    cout << "  step         (s)" << endl;
    cout << "  workitem     (wi)" << endl;
    cout << "(type 'help command' for more information)" << endl;
    return;
  }

  if (args[1] == "backtrace")
  {
    // TODO: Help message
  }
  else if (args[1] == "break")
  {
    // TODO: Help message
  }
  else if (args[1] == "clear")
  {
    // TODO: Help message
  }
  else if (args[1] == "continue")
  {
    cout << "Continue kernel execution until next breakpoint." << endl;
  }
  else if (args[1] == "help")
  {
    cout << "Display usage information for a command." << endl;
  }
  else if (args[1] == "info")
  {
    cout << "Display information about current debugging context." << endl;
  }
  else if (args[1] == "list")
  {
    // TODO: Help message
  }
  else if (args[1] == "print")
  {
    // TODO: Help message
  }
  else if (args[1] == "printglobal")
  {
    // TODO: Help message
  }
  else if (args[1] == "printlocal")
  {
    // TODO: Help message
  }
  else if (args[1] == "printprivate")
  {
    // TODO: Help message
  }
  else if (args[1] == "quit")
  {
    cout << "Quit interactive debugger "
        << "(and terminate current kernel invocation)." << endl;
  }
  else if (args[1] == "step")
  {
    // TODO: Help message
  }
  else if (args[1] == "workitem")
  {
    // TODO: Help message
  }
  else
  {
    cout << "Unrecognized command '" << args[1] << "'" << endl;
  }
}

void Device::info(vector<string> args)
{
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
  const size_t *gid = m_currentWorkItem->getGlobalID();
  cout << endl << "Current work-item: (" << gid[0] << ","
                                         << gid[1] << ","
                                         << gid[2] << ")" << endl;
  // TODO: Show source line if available
  dumpInstruction(cout, *m_currentWorkItem->getCurrentInstruction());
}

void Device::list(vector<string> args)
{
  // TODO: Implement
  cout << "Unimplemented command 'list'" << endl;
}

void Device::print(vector<string> args)
{
  // TODO: Implement
  cout << "Unimplemented command 'print'" << endl;
}

void Device::printglobal(vector<string> args)
{
  // TODO: Implement
  cout << "Unimplemented command 'printglobal'" << endl;
}

void Device::printlocal(vector<string> args)
{
  // TODO: Implement
  cout << "Unimplemented command 'printlocal'" << endl;
}

void Device::printprivate(vector<string> args)
{
  // TODO: Implement
  cout << "Unimplemented command 'printprivate'" << endl;
}

void Device::quit(vector<string> args)
{
  m_interactive = false;
  m_running = false;
}

void Device::step(vector<string> args)
{
  if (!m_currentWorkItem)
  {
    cerr << "All work-items finished." << endl;
    return;
  }

  // TODO: Step whole source line if available
  if (m_currentWorkItem->step() != WorkItem::READY)
  {
    // Switch to next work-item
    if (nextWorkItem())
    {
      // Print new WI id
      const size_t *gid = m_currentWorkItem->getGlobalID();
      cout << "Now executing work-item: (" << gid[0] << ","
                                           << gid[1] << ","
                                           << gid[2] << ")" << endl;
    }
  }
  if (m_currentWorkItem)
  {
    // TODO: Show source line if available
    dumpInstruction(cout, *m_currentWorkItem->getCurrentInstruction());
  }
}

void Device::workitem(vector<string> args)
{
  // TODO: Implement
  cout << "Unimplemented command 'workitem'" << endl;
}
