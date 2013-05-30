#include "common.h"

class Memory;
class Kernel;

class WorkItem
{
public:
  WorkItem(const Kernel& kernel, Memory &globalMem,
           size_t gid_x, size_t gid_y=0, size_t gid_z=0);
  virtual ~WorkItem();

  void dumpPrivateMemory() const;
  void enableDebugOutput(bool enable);
  void execute(const llvm::Instruction& instruction);
  const size_t* getGlobalID() const;
  const llvm::Value* getNextBlock() const;
  void outputMemoryError(const llvm::Instruction& instruction,
                         const std::string& msg,
                         unsigned addressSpace,
                         size_t address, size_t size) const;
  void setCurrentBlock(const llvm::Value *block);

  void add(const llvm::Instruction& instruction, TypedValue& result);
  void alloca(const llvm::Instruction& instruction);
  void br(const llvm::Instruction& instruction);
  void call(const llvm::Instruction& instruction, TypedValue& result);
  void fadd(const llvm::Instruction& instruction, TypedValue& result);
  void fmul(const llvm::Instruction& instruction, TypedValue& result);
  void icmp(const llvm::Instruction& instruction, TypedValue& result);
  void gep(const llvm::Instruction& instruction, TypedValue& result);
  void land(const llvm::Instruction& instruction, TypedValue& result);
  void load(const llvm::Instruction& instruction, TypedValue& result);
  void mul(const llvm::Instruction& instruction, TypedValue& result);
  void phi(const llvm::Instruction& instruction, TypedValue& result);
  void store(const llvm::Instruction& instruction);

private:
  size_t m_globalID[3];
  TypedValueMap m_privateMemory;
  Memory *m_stack;
  Memory& m_globalMemory;

  const llvm::Value *m_prevBlock;
  const llvm::Value *m_currBlock;
  const llvm::Value *m_nextBlock;

  bool m_debugOutput;
};
