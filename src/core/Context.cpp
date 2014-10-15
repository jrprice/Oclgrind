// Context.cpp (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

#include <cassert>
#include <sstream>

#if defined(_WIN32) && !defined(__MINGW32__)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "llvm/DebugInfo.h"
#include "llvm/Instruction.h"

#include "Context.h"
#include "Kernel.h"
#include "KernelInvocation.h"
#include "Memory.h"
#include "WorkGroup.h"
#include "WorkItem.h"

#include "plugins/InstructionCounter.h"
#include "plugins/InteractiveDebugger.h"
#include "plugins/RaceDetector.h"

using namespace oclgrind;
using namespace std;

Context::Context()
{
  m_globalMemory = new Memory(AddrSpaceGlobal, this);
  m_kernelInvocation = NULL;

  loadPlugins();
}

Context::~Context()
{
  delete m_globalMemory;

  unloadPlugins();
}

Memory* Context::getGlobalMemory() const
{
  return m_globalMemory;
}

void Context::loadPlugins()
{
  // Create core plugins
  const char *instCounts = getenv("OCLGRIND_INST_COUNTS");
  if (instCounts && strcmp(instCounts, "1") == 0)
    m_plugins.push_back(make_pair(new InstructionCounter(this), true));

  const char *dataRaces = getenv("OCLGRIND_DATA_RACES");
  if (dataRaces && strcmp(dataRaces, "1") == 0)
    m_plugins.push_back(make_pair(new RaceDetector(this), true));

  const char *interactive = getenv("OCLGRIND_INTERACTIVE");
  if (interactive && strcmp(interactive, "1") == 0)
    m_plugins.push_back(make_pair(new InteractiveDebugger(this), true));


  // Load dynamic plugins
  const char *dynamicPlugins = getenv("OCLGRIND_PLUGINS");
  if (dynamicPlugins)
  {
    std::istringstream ss(dynamicPlugins);
    std::string libpath;
    while(std::getline(ss, libpath, ':'))
    {
#if defined(_WIN32) && !defined(__MINGW32__)
      HMODULE library = LoadLibrary(libpath.c_str());
      if (!library)
      {
        cerr << "Loading Oclgrind plugin failed (LoadLibrary): "
             << GetLastError() << endl;
        continue;
      }

      void *initialize = GetProcAddress(library, "initializePlugins");
      if (!initialize)
      {
        cerr << "Loading Oclgrind plugin failed (GetProcAddress): "
             << GetLastError() << endl;
        continue;
      }
#else
      void *library = dlopen(libpath.c_str(), RTLD_NOW);
      if (!library)
      {
        cerr << "Loading Oclgrind plugin failed (dlopen): "
             << dlerror() << endl;
        continue;
      }

      void *initialize = dlsym(library, "initializePlugins");
      if (!initialize)
      {
        cerr << "Loading Oclgrind plugin failed (dlsym): "
             << dlerror() << endl;
        continue;
      }
#endif

      ((void(*)(Context*))initialize)(this);
      m_pluginLibraries.push_back(library);
    }
  }
}

void Context::unloadPlugins()
{
  // Release dynamic plugin libraries
  list<void*>::iterator plibItr;
  for (plibItr = m_pluginLibraries.begin();
       plibItr != m_pluginLibraries.end(); plibItr++)
  {
#if defined(_WIN32) && !defined(__MINGW32__)
      void *release = GetProcAddress(*plibItr, "releasePlugins");
      if (release)
      {
        ((void(*)(Context*))release)(this);
      }
      FreeLibrary(*plibItr);
#else
      void *release = dlsym(*plibItr, "releasePlugins");
      if (release)
      {
        ((void(*)(Context*))release)(this);
      }
      dlclose(*plibItr);
#endif
  }

  // Destroy internal plugins
  PluginList::iterator pItr;
  for (pItr = m_plugins.begin(); pItr != m_plugins.end(); pItr++)
  {
    if (pItr->second)
      delete pItr->first;
  }

  m_plugins.clear();
}

void Context::registerPlugin(Plugin *plugin)
{
  m_plugins.push_back(make_pair(plugin, false));
}

void Context::unregisterPlugin(Plugin *plugin)
{
  m_plugins.remove(make_pair(plugin, false));
}

void Context::logDataRace(DataRaceType type, unsigned int addrSpace,
                          size_t address,
                          size_t lastWorkItem,
                          size_t lastWorkGroup,
                          const llvm::Instruction *lastInstruction) const
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
    assert(false && "Data race in unsupported address space.");
    break;
  }

  // Error info
  cerr << endl;
  switch (type)
  {
    case ReadWriteRace:
      cerr << "Read-write";
      break;
    case WriteWriteRace:
      cerr << "Write-write";
      break;
    default:
      cerr << "Unrecognized";
      break;
  }
  cerr << " data race"
       << " at " << memType
       << " memory address " << hex << address << endl;

  printErrorContext();
  cerr << endl;

  // Show details of other entity involved in race
  if (lastWorkItem != -1)
  {
    size_t gx, gy, gz;
    gx = lastWorkItem % m_kernelInvocation->getGlobalSize().x;
    gy = (lastWorkItem - gx) / m_kernelInvocation->getGlobalSize().y;
    gz = (lastWorkItem - gy - gx) / m_kernelInvocation->getGlobalSize().z;
    cerr << "\tRace occurred with work-item (" << dec
         << gx << ","
         << gy << ","
         << gz << ")" << endl;
  }
  else if (lastWorkGroup != -1)
  {
    size_t gx, gy, gz;
    gx = lastWorkGroup % m_kernelInvocation->getNumGroups().x;
    gy = (lastWorkGroup - gx) / m_kernelInvocation->getNumGroups().y;
    gz = (lastWorkGroup - gy - gx) / m_kernelInvocation->getNumGroups().z;
    cerr << "\tRace occurred with work-group (" << dec
         << gx << ","
         << gy << ","
         << gz << ")" << endl;
  }
  else
  {
    cerr << "\tRace occurred with unknown entity" << endl;
  }

  // Show conflicting instruction
  if (lastInstruction)
  {
    cerr << "\t";
    printInstruction(lastInstruction);
  }

  cerr << endl;

  // TODO: THIS (notifyError)
  //m_device->forceBreak();
}

void Context::logDivergence(const llvm::Instruction *instruction,
                            string divergence,
                            string currentInfo, string previousInfo) const
{
  // Error info
  cerr << endl
       << "Work-group divergence detected ("
       << divergence
       << "):" << endl;
  printErrorContext();
  if (!currentInfo.empty())
  {
    cerr << "\t" << currentInfo << endl;
  }
  cerr << endl;

  // Show divergent instruction/info
  cerr << "Previous work-items executed this instruction:" << endl;
  cerr << "\t";
  printInstruction(instruction);
  if (!previousInfo.empty())
  {
    cerr << "\t" << previousInfo << endl;
  }

  cerr << endl;

  // TODO: THIS (notifyError)
  //m_device->forceBreak();
}

void Context::logError(const char* error, const char* info) const
{
  // Error info
  cerr << endl << error << endl;
  printErrorContext();
  if (info)
  {
    cerr << "\t" << info << endl;
  }
  cerr << endl;

  // TODO: THIS (notifyError)
  //m_device->forceBreak();
}

void Context::logMemoryError(bool read, unsigned int addrSpace,
                             size_t address, size_t size) const
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
  cerr << endl;

  // TODO: THIS (notifyError)
  //m_device->forceBreak();
}

void Context::printErrorContext() const
{
  // Work item
  const WorkItem *workItem = m_kernelInvocation->getCurrentWorkItem();
  if (workItem)
  {
    cerr << "\tWork-item: "
         << " Global" << workItem->getGlobalID()
         << " Local" << workItem->getLocalID() << endl;
  }

  // Work group
  const WorkGroup *workGroup = m_kernelInvocation->getCurrentWorkGroup();
  if (workGroup)
  {
    Size3 group = workGroup->getGroupID();
    cerr << "\tWork-group: " << group << endl;
  }

  // Kernel
  const Kernel *kernel = m_kernelInvocation->getKernel();
  if (kernel)
  {
    cerr << "\tKernel:     " << kernel->getName() << endl;
  }

  // Instruction
  if (workItem)
  {
    cerr << "\t";
    printInstruction(workItem->getCurrentInstruction());
  }
}

void Context::printInstruction(const llvm::Instruction *instruction) const
{
  dumpInstruction(cerr, instruction);
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

#define NOTIFY(function, ...)                     \
  PluginList::const_iterator pluginItr;           \
  for (pluginItr = m_plugins.begin();             \
       pluginItr != m_plugins.end(); pluginItr++) \
  {                                               \
    pluginItr->first->function(__VA_ARGS__);      \
  }

void Context::notifyInstructionExecuted(const WorkItem *workItem,
                                        const llvm::Instruction *instruction,
                                        const TypedValue& result) const
{
  NOTIFY(instructionExecuted, workItem, instruction, result);
}

void Context::notifyKernelBegin(KernelInvocation *kernelInvocation) const
{
  assert(m_kernelInvocation == NULL);
  m_kernelInvocation = kernelInvocation;

  NOTIFY(kernelBegin, kernelInvocation);
}

void Context::notifyKernelEnd(KernelInvocation *kernelInvocation) const
{
  NOTIFY(kernelEnd, kernelInvocation);

  assert(m_kernelInvocation == kernelInvocation);
  m_kernelInvocation = NULL;
}

void Context::notifyMemoryAllocated(const Memory *memory, size_t address,
                                    size_t size) const
{
  NOTIFY(memoryAllocated, memory, address, size);
}

void Context::notifyMemoryAtomic(const Memory *memory, size_t address,
                                 size_t size) const
{
  NOTIFY(memoryAtomic, memory, address, size);
}

void Context::notifyMemoryDeallocated(const Memory *memory,
                                      size_t address) const
{
  NOTIFY(memoryDeallocated, memory, address);
}

void Context::notifyMemoryLoad(const Memory *memory, size_t address,
                               size_t size) const
{
  NOTIFY(memoryLoad, memory, address, size);
}

void Context::notifyMemoryStore(const Memory *memory, size_t address,
                                size_t size, const uint8_t *storeData) const
{
  NOTIFY(memoryStore, memory, address, size, storeData);
}

void Context::notifyWorkGroupBarrier(const WorkGroup *workGroup,
                                     uint32_t flags) const
{
  NOTIFY(workGroupBarrier, workGroup, flags);
}

#undef NOTIFY
