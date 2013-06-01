#include "common.h"

class Kernel;
class Memory;
class WorkGroup;

class WorkItem
{
public:
  enum State {READY, BARRIER, FINISHED};

public:
  WorkItem(WorkGroup& workGroup,
           const Kernel& kernel, Memory &globalMem,
           size_t lid_x, size_t lid_y, size_t lid_z);
  virtual ~WorkItem();

  void clearBarrier();
  void dumpPrivateMemory() const;
  void enableDebugOutput(bool enable);
  void execute(const llvm::Instruction& instruction);
  const size_t* getGlobalID() const;
  uint64_t getIntValue(const llvm::Value *operand);
  State getState() const;
  void outputMemoryError(const llvm::Instruction& instruction,
                         const std::string& msg,
                         unsigned addressSpace,
                         size_t address, size_t size) const;
  State step(bool debugOutput = false);

  void add(const llvm::Instruction& instruction, TypedValue& result);
  void alloca(const llvm::Instruction& instruction);
  void br(const llvm::Instruction& instruction);
  void bwand(const llvm::Instruction& instruction, TypedValue& result);
  void bwor(const llvm::Instruction& instruction, TypedValue& result);
  void call(const llvm::Instruction& instruction, TypedValue& result);
  void fadd(const llvm::Instruction& instruction, TypedValue& result);
  void fmul(const llvm::Instruction& instruction, TypedValue& result);
  void gep(const llvm::Instruction& instruction, TypedValue& result);
  void icmp(const llvm::Instruction& instruction, TypedValue& result);
  void load(const llvm::Instruction& instruction, TypedValue& result);
  void mul(const llvm::Instruction& instruction, TypedValue& result);
  void phi(const llvm::Instruction& instruction, TypedValue& result);
  void store(const llvm::Instruction& instruction);

private:
  size_t m_globalID[3];
  size_t m_localID[3];
  TypedValueMap m_privateMemory;
  const Kernel& m_kernel;
  Memory *m_stack;
  Memory& m_globalMemory;
  WorkGroup& m_workGroup;

  State m_state;
  llvm::Function::const_iterator m_prevBlock;
  llvm::Function::const_iterator m_currBlock;
  llvm::Function::const_iterator m_nextBlock;
  llvm::BasicBlock::const_iterator m_currInst;

  bool m_debugOutput;
};
