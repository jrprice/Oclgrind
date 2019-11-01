// Context.cpp (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "config.h"
#include "common.h"

#if defined(_WIN32) && !defined(__MINGW32__)
#include <windows.h>
#undef ERROR
#else
#include <dlfcn.h>
#endif

#include <mutex>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Instruction.h"

#include "Context.h"
#include "Kernel.h"
#include "KernelInvocation.h"
#include "Memory.h"
#include "Program.h"
#include "WorkGroup.h"
#include "WorkItem.h"

#include "plugins/InstructionCounter.h"
#include "plugins/WorkloadCharacterisation.h"
#include "plugins/InteractiveDebugger.h"
#include "plugins/Logger.h"
#include "plugins/MemCheck.h"
#include "plugins/RaceDetector.h"
#include "plugins/Uninitialized.h"

using namespace oclgrind;
using namespace std;

Context::Context()
{
  m_llvmContext = new llvm::LLVMContext;

  m_globalMemory = new Memory(AddrSpaceGlobal, sizeof(size_t)==8 ? 16 : 8,
                              this);
  m_kernelInvocation = NULL;

  loadPlugins();
}

Context::~Context()
{
  delete m_llvmContext;
  delete m_globalMemory;

  unloadPlugins();
}

bool Context::isThreadSafe() const
{
  for (const PluginEntry &p : m_plugins)
  {
    if (!p.first->isThreadSafe())
      return false;
  }
  return true;
}

Memory* Context::getGlobalMemory() const
{
  return m_globalMemory;
}

llvm::LLVMContext* Context::getLLVMContext() const
{
  return m_llvmContext;
}

void Context::loadPlugins()
{
  // Create core plugins
  m_plugins.push_back(make_pair(new Logger(this), true));
  m_plugins.push_back(make_pair(new MemCheck(this), true));

  if (checkEnv("OCLGRIND_INST_COUNTS"))
    m_plugins.push_back(make_pair(new InstructionCounter(this), true));

  if (checkEnv("OCLGRIND_WORKLOAD_CHARACTERISATION"))
    m_plugins.push_back(make_pair(new WorkloadCharacterisation(this), true));

  if (checkEnv("OCLGRIND_DATA_RACES"))
    m_plugins.push_back(make_pair(new RaceDetector(this), true));

  if (checkEnv("OCLGRIND_UNINITIALIZED"))
    m_plugins.push_back(make_pair(new Uninitialized(this), true));

  if (checkEnv("OCLGRIND_INTERACTIVE"))
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
      HMODULE library = LoadLibraryA(libpath.c_str());
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
      void *release = GetProcAddress((HMODULE)*plibItr, "releasePlugins");
      if (release)
      {
        ((void(*)(Context*))release)(this);
      }
      FreeLibrary((HMODULE)*plibItr);
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

void Context::logError(const char* error) const
{
  Message msg(ERROR, this);
  msg << error << endl
      << msg.INDENT
      << "Kernel: " << msg.CURRENT_KERNEL << endl
      << "Entity: " << msg.CURRENT_ENTITY << endl
      << msg.CURRENT_LOCATION << endl;
  msg.send();
}

#define NOTIFY(function, ...)                     \
{                                                 \
  PluginList::const_iterator pluginItr;           \
  for (pluginItr = m_plugins.begin();             \
       pluginItr != m_plugins.end(); pluginItr++) \
  {                                               \
    pluginItr->first->function(__VA_ARGS__);      \
  }                                               \
}

void Context::notifyInstructionExecuted(const WorkItem *workItem,
                                        const llvm::Instruction *instruction,
                                        const TypedValue& result) const
{
  NOTIFY(instructionExecuted, workItem, instruction, result);
}

void Context::notifyKernelBegin(const KernelInvocation *kernelInvocation) const
{
  assert(m_kernelInvocation == NULL);
  m_kernelInvocation = kernelInvocation;

  NOTIFY(kernelBegin, kernelInvocation);
}

void Context::notifyKernelEnd(const KernelInvocation *kernelInvocation) const
{
  NOTIFY(kernelEnd, kernelInvocation);

  assert(m_kernelInvocation == kernelInvocation);
  m_kernelInvocation = NULL;
}

void Context::notifyMemoryAllocated(const Memory *memory, size_t address,
                                    size_t size, cl_mem_flags flags,
                                    const uint8_t *initData) const
{
  NOTIFY(memoryAllocated, memory, address, size, flags, initData);
}

void Context::notifyMemoryAtomicLoad(const Memory *memory, AtomicOp op,
                                     size_t address, size_t size) const
{
  if (m_kernelInvocation && m_kernelInvocation->getCurrentWorkItem())
  {
    NOTIFY(memoryAtomicLoad, memory, m_kernelInvocation->getCurrentWorkItem(),
           op, address, size);
  }
}

void Context::notifyMemoryAtomicStore(const Memory *memory, AtomicOp op,
                                      size_t address, size_t size) const
{
  if (m_kernelInvocation && m_kernelInvocation->getCurrentWorkItem())
  {
    NOTIFY(memoryAtomicStore, memory, m_kernelInvocation->getCurrentWorkItem(),
           op, address, size);
  }
}

void Context::notifyMemoryDeallocated(const Memory *memory,
                                      size_t address) const
{
  NOTIFY(memoryDeallocated, memory, address);
}

void Context::notifyMemoryLoad(const Memory *memory, size_t address,
                               size_t size) const
{
  if (m_kernelInvocation)
  {
    if (m_kernelInvocation->getCurrentWorkItem())
    {
      NOTIFY(memoryLoad, memory, m_kernelInvocation->getCurrentWorkItem(),
             address, size);
    }
    else if (m_kernelInvocation->getCurrentWorkGroup())
    {
      NOTIFY(memoryLoad, memory, m_kernelInvocation->getCurrentWorkGroup(),
             address, size);
    }
  }
  else
  {
    NOTIFY(hostMemoryLoad, memory, address, size);
  }
}

void Context::notifyMemoryMap(const Memory *memory, size_t address,
                              size_t offset, size_t size,
                              cl_mem_flags flags) const
{
  NOTIFY(memoryMap, memory, address, offset, size, flags);
}

void Context::notifyMemoryStore(const Memory *memory, size_t address,
                                size_t size, const uint8_t *storeData) const
{
  if (m_kernelInvocation)
  {
    if (m_kernelInvocation->getCurrentWorkItem())
    {
      NOTIFY(memoryStore, memory, m_kernelInvocation->getCurrentWorkItem(),
             address, size, storeData);
    }
    else if (m_kernelInvocation->getCurrentWorkGroup())
    {
      NOTIFY(memoryStore, memory, m_kernelInvocation->getCurrentWorkGroup(),
             address, size, storeData);
    }
  }
  else
  {
    NOTIFY(hostMemoryStore, memory, address, size, storeData);
  }
}

void Context::notifyMessage(MessageType type, const char *message) const
{
  NOTIFY(log, type, message);
}

void Context::notifyMemoryUnmap(const Memory *memory, size_t address,
                                const void *ptr) const
{
  NOTIFY(memoryUnmap, memory, address, ptr);
}

void Context::notifyWorkGroupBarrier(const WorkGroup *workGroup,
                                     uint32_t flags) const
{
  NOTIFY(workGroupBarrier, workGroup, flags);
}

void Context::notifyWorkGroupBegin(const WorkGroup *workGroup) const
{
  NOTIFY(workGroupBegin, workGroup);
}

void Context::notifyWorkGroupComplete(const WorkGroup *workGroup) const
{
  NOTIFY(workGroupComplete, workGroup);
}

void Context::notifyWorkItemBegin(const WorkItem *workItem) const
{
  NOTIFY(workItemBegin, workItem);
}

void Context::notifyWorkItemComplete(const WorkItem *workItem) const
{
  NOTIFY(workItemComplete, workItem);
}

void Context::notifyWorkItemBarrier(const WorkItem *workItem) const
{
  NOTIFY(workItemBarrier, workItem);
}

void Context::notifyWorkItemClearBarrier(const WorkItem *workItem) const
{
  NOTIFY(workItemClearBarrier, workItem);
}

#undef NOTIFY


Context::Message::Message(MessageType type, const Context *context)
{
  m_type             = type;
  m_context          = context;
  m_kernelInvocation = context->m_kernelInvocation;
}

Context::Message& Context::Message::operator<<(const special& id)
{
  switch (id)
  {
  case INDENT:
    m_indentModifiers.push_back( m_stream.tellp());
    break;
  case UNINDENT:
    m_indentModifiers.push_back(-m_stream.tellp());
    break;
  case CURRENT_KERNEL:
    *this << m_kernelInvocation->getKernel()->getName();
    break;
  case CURRENT_WORK_ITEM_GLOBAL:
  {
    const WorkItem *workItem = m_kernelInvocation->getCurrentWorkItem();
    if (workItem)
    {
      *this << workItem->getGlobalID();
    }
    else
    {
      *this << "(none)";
    }
    break;
  }
  case CURRENT_WORK_ITEM_LOCAL:
  {
    const WorkItem *workItem = m_kernelInvocation->getCurrentWorkItem();
    if (workItem)
    {
      *this << workItem->getLocalID();
    }
    else
    {
      *this << "(none)";
    }
    break;
  }
  case CURRENT_WORK_GROUP:
  {
    const WorkGroup *workGroup = m_kernelInvocation->getCurrentWorkGroup();
    if (workGroup)
    {
      *this << workGroup->getGroupID();
    }
    else
    {
      *this << "(none)";
    }
    break;
  }
  case CURRENT_ENTITY:
  {
    const WorkItem *workItem = m_kernelInvocation->getCurrentWorkItem();
    const WorkGroup *workGroup = m_kernelInvocation->getCurrentWorkGroup();
    if (workItem)
    {
      *this << "Global" << workItem->getGlobalID()
            << " Local" << workItem->getLocalID() << " ";
    }
    if (workGroup)
    {
      *this << "Group" << workGroup->getGroupID();
    }
    if (!workItem && ! workGroup)
    {
      *this << "(unknown)";
    }
    break;
  }
  case CURRENT_LOCATION:
  {
    const llvm::Instruction *instruction = NULL;
    const WorkItem *workItem = m_kernelInvocation->getCurrentWorkItem();
    const WorkGroup *workGroup = m_kernelInvocation->getCurrentWorkGroup();
    if (workItem)
    {
      instruction = workItem->getCurrentInstruction();
    }
    else if (workGroup)
    {
      instruction = workGroup->getCurrentBarrier();
    }

    *this << instruction;
    break;
  }
  }
  return *this;
}

Context::Message& Context::Message::operator<<(
  const llvm::Instruction *instruction)
{
  // Use mutex as some part of LLVM used by dumpInstruction() is not thread-safe
  static std::mutex mtx;
  std::lock_guard<std::mutex> lock(mtx);

  if (instruction)
  {
    // Output instruction
    dumpInstruction(m_stream, instruction);
    *this << endl;

    // Output debug information
    llvm::MDNode *md = instruction->getMetadata("dbg");
    if (!md)
    {
      *this << "Debugging information not available." << endl;
    }
    else
    {
      llvm::DILocation *loc = (llvm::DILocation*)md;
      unsigned lineNumber = loc->getLine();
      unsigned columnNumber = loc->getColumn();
      llvm::StringRef filename = loc->getFilename();

      *this << "At line " << dec << lineNumber
            << " (column " << columnNumber << ")"
            << " of " << filename.str() << ":" << endl;

      // Get source line
      const Program *program = m_kernelInvocation->getKernel()->getProgram();
      const char *line = program->getSourceLine(lineNumber);
      if (line)
      {
        while (isspace(line[0]))
          line++;
        *this << "  " << line;
      }
      else
        *this << "  (source not available)";

    }
  }
  else
  {
    *this << "(location unknown)";
  }

  return *this;
}

Context::Message& Context::Message::operator<<(
  std::ostream& (*t)(std::ostream&))
{
  m_stream << t;
  return *this;
}

Context::Message& Context::Message::operator<<(
  std::ios& (*t)(std::ios&))
{
  m_stream << t;
  return *this;
}

Context::Message& Context::Message::operator<<(
  std::ios_base& (*t)(std::ios_base&))
{
  m_stream << t;
  return *this;
}

void Context::Message::send() const
{
  string msg;

  string line;
  int currentIndent = 0;
  list<int>::const_iterator itr = m_indentModifiers.begin();

  m_stream.clear();
  m_stream.seekg(0);
  while (m_stream.good())
  {
    getline(m_stream, line);

    // TODO: Wrap long lines
    msg += line;

    // Check for indentation modifiers
    long pos = m_stream.tellg();
    if (itr != m_indentModifiers.end() && pos >= abs(*itr))
    {
      if (*itr >= 0)
        currentIndent++;
      else
        currentIndent--;
      itr++;
    }

    if (!m_stream.eof())
    {
      // Add newline and indentation
      msg += '\n';
      for (int i = 0; i < currentIndent; i++)
        msg += '\t';
    }
  }

  m_context->notifyMessage(m_type, msg.c_str());
}
