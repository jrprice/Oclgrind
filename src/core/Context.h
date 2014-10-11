#include "common.h"

namespace oclgrind
{
  class Device;
  class Kernel;
  class Memory;
  class Plugin;
  class WorkGroup;
  class WorkItem;

  typedef std::list<Plugin*> PluginList;

  class Context
  {
  public:
    Context();
    virtual ~Context();

    Device* getDevice() const;
    Memory* getGlobalMemory() const;

    // Error logging
    void logDataRace(DataRaceType type, unsigned int addrSpace,
                     size_t address,
                     size_t lastWorkGroup,
                     size_t lastWorkItem,
                     const llvm::Instruction *lastInstruction) const;
    void logDivergence(const llvm::Instruction *instruction,
                       std::string divergence,
                       std::string currentInfo="",
                       std::string lastInfo="") const;
    void logError(const char* error, const char* info=0) const;
    void logMemoryError(bool read, unsigned int addrSpace,
                        size_t address, size_t size) const;

    // Simulation callbacks
    void notifyInstructionExecuted(const WorkItem *workItem,
                                   const llvm::Instruction *instruction,
                                   const TypedValue& result) const;
    void notifyKernelBegin(const Kernel *kernel) const;
    void notifyKernelEnd(const Kernel *kernel) const;
    void notifyMemoryAllocated(const Memory *memory, size_t address,
                               size_t size) const;
    void notifyMemoryAtomic(const Memory *memory, size_t address,
                            size_t size) const;
    void notifyMemoryDeallocated(const Memory *memory, size_t address) const;
    void notifyMemoryLoad(const Memory *memory, size_t address,
                          size_t size) const;
    void notifyMemoryStore(const Memory *memory, size_t address, size_t size,
                           const uint8_t *storeData) const;
    void notifyWorkGroupBarrier(const WorkGroup *workGroup,
                                uint32_t flags) const;

    void registerPlugin(Plugin *plugin);
    void unregisterPlugin(Plugin *plugin);

  private:
    Device *m_device;
    Memory *m_globalMemory;

    PluginList m_plugins;
    void loadPlugins();

    void printErrorContext() const;
    void printInstruction(const llvm::Instruction *instruction) const;
  };
}