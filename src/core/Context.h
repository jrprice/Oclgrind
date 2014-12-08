// Context.h (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

namespace oclgrind
{
  class KernelInvocation;
  class Memory;
  class Plugin;
  class WorkGroup;
  class WorkItem;

  typedef std::pair<Plugin*, bool> PluginEntry;
  typedef std::list<PluginEntry> PluginList;

  class Context
  {
  public:
    Context();
    virtual ~Context();

    Memory* getGlobalMemory() const;

    void logError(const char* error) const;

    // Simulation callbacks
    void notifyInstructionExecuted(const WorkItem *workItem,
                                   const llvm::Instruction *instruction,
                                   const TypedValue& result) const;
    void notifyKernelBegin(KernelInvocation *kernelInvocation) const;
    void notifyKernelEnd(KernelInvocation *kernelInvocation) const;
    void notifyMemoryAllocated(const Memory *memory, size_t address,
                               size_t size) const;
    void notifyMemoryAtomic(const Memory *memory, AtomicOp op,
                            size_t address, size_t size) const;
    void notifyMemoryDeallocated(const Memory *memory, size_t address) const;
    void notifyMemoryLoad(const Memory *memory, size_t address,
                          size_t size) const;
    void notifyMemoryStore(const Memory *memory, size_t address, size_t size,
                           const uint8_t *storeData) const;
    void notifyMessage(MessageType type, const char *message) const;
    void notifyWorkGroupBarrier(const WorkGroup *workGroup,
                                uint32_t flags) const;
    void notifyWorkGroupComplete(const WorkGroup *workGroup) const;
    void notifyWorkItemComplete(const WorkItem *workItem) const;


    // Plugins
    void registerPlugin(Plugin *plugin);
    void unregisterPlugin(Plugin *plugin);

  private:
    mutable const KernelInvocation *m_kernelInvocation;
    Memory *m_globalMemory;

    PluginList m_plugins;
    std::list<void*> m_pluginLibraries;
    void loadPlugins();
    void unloadPlugins();

  public:
    class Message
    {
    public:
      enum special
      {
        INDENT,
        UNINDENT,
        CURRENT_KERNEL,
        CURRENT_WORK_ITEM_GLOBAL,
        CURRENT_WORK_ITEM_LOCAL,
        CURRENT_WORK_GROUP,
        CURRENT_ENTITY,
        CURRENT_LOCATION,
      };

      Message(MessageType type, const Context *context);

      Message& operator<<(const special& id);
      Message& operator<<(const llvm::Instruction *instruction);

      template<typename T>
      Message& operator<<(const T& t);
      Message& operator<<(std::ostream& (*t)(std::ostream&));
      Message& operator<<(std::ios& (*t)(std::ios&));
      Message& operator<<(std::ios_base& (*t)(std::ios_base&));

      void send() const;

    private:
      MessageType                m_type;
      const Context             *m_context;
      const KernelInvocation    *m_kernelInvocation;
      mutable std::stringstream  m_stream;
      std::list<int>             m_indentModifiers;
    };
  };

  template<typename T>
  Context::Message& Context::Message::operator<<(const T& t)
  {
    m_stream << t;
    return *this;
  }
}
