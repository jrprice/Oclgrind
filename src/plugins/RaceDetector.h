#include "core/Plugin.h"

namespace llvm
{
  class Function;
}

namespace oclgrind
{
  class RaceDetector : public Plugin
  {
  public:
    RaceDetector(const Context *context);

    virtual void kernelBegin(const KernelInvocation *kernelInvocation);
    virtual void kernelEnd(const KernelInvocation *kernelInvocation);
    virtual void memoryAllocated(const Memory *memory, size_t address,
                                 size_t size);
    virtual void memoryAtomic(const Memory *memory, size_t address,
                              size_t size);
    virtual void memoryDeallocated(const Memory *memory, size_t address);
    virtual void memoryLoad(const Memory *memory, size_t address, size_t size);
    virtual void memoryStore(const Memory *memory, size_t address, size_t size,
                             const uint8_t *storeData);
    virtual void workGroupBarrier(const WorkGroup *workGroup, uint32_t flags);

  private:
    struct State
    {
      const llvm::Instruction *instruction;
      size_t workItem;
      size_t workGroup;
      bool canAtomic;
      bool canRead;
      bool canWrite;
      bool wasWorkItem;

      State();
    };

    typedef std::map<
                      std::pair<const Memory*, size_t>,
                      std::pair<State*, size_t>
                    > StateMap;
    StateMap m_state;

    bool m_runningKernel;
    bool m_allowUniformWrites;

    void registerLoadStore(const Memory *memory, size_t address,
                           size_t size, const uint8_t *storeData);
    void synchronize(const Memory *memory, bool workGroup);
  };
}
