#include "common.h"

class Kernel;
class Memory;
class WorkGroup;

namespace llvm
{
  class DbgValueInst;
}

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
  double getFloatValue(const llvm::Value *operand) const;
  uint64_t getIntValue(const llvm::Value *operand) const;
  State getState() const;
  void outputMemoryError(const llvm::Instruction& instruction,
                         const std::string& msg,
                         unsigned addressSpace,
                         size_t address, size_t size) const;
  void setFloatResult(TypedValue& result, double val) const;
  State step(bool debugOutput = false);
  void updateVariable(const llvm::DbgValueInst *instruction);

  void add(const llvm::Instruction& instruction, TypedValue& result);
  void alloca(const llvm::Instruction& instruction);
  void ashr(const llvm::Instruction& instruction, TypedValue& result);
  void br(const llvm::Instruction& instruction);
  void bwand(const llvm::Instruction& instruction, TypedValue& result);
  void bwor(const llvm::Instruction& instruction, TypedValue& result);
  void bwxor(const llvm::Instruction& instruction, TypedValue& result);
  void call(const llvm::Instruction& instruction, TypedValue& result);
  void fadd(const llvm::Instruction& instruction, TypedValue& result);
  void fcmp(const llvm::Instruction& instruction, TypedValue& result);
  void fdiv(const llvm::Instruction& instruction, TypedValue& result);
  void fmul(const llvm::Instruction& instruction, TypedValue& result);
  void frem(const llvm::Instruction& instruction, TypedValue& result);
  void fsub(const llvm::Instruction& instruction, TypedValue& result);
  void gep(const llvm::Instruction& instruction, TypedValue& result);
  void icmp(const llvm::Instruction& instruction, TypedValue& result);
  void load(const llvm::Instruction& instruction, TypedValue& result);
  void lshr(const llvm::Instruction& instruction, TypedValue& result);
  void mul(const llvm::Instruction& instruction, TypedValue& result);
  void phi(const llvm::Instruction& instruction, TypedValue& result);
  void sdiv(const llvm::Instruction& instruction, TypedValue& result);
  void select(const llvm::Instruction& instruction, TypedValue& result);
  void sext(const llvm::Instruction& instruction, TypedValue& result);
  void shl(const llvm::Instruction& instruction, TypedValue& result);
  void srem(const llvm::Instruction& instruction, TypedValue& result);
  void store(const llvm::Instruction& instruction);
  void trunc(const llvm::Instruction& instruction, TypedValue& result);
  void udiv(const llvm::Instruction& instruction, TypedValue& result);
  void urem(const llvm::Instruction& instruction, TypedValue& result);

private:
  size_t m_globalID[3];
  size_t m_localID[3];
  TypedValueMap m_privateMemory;
  std::map<std::string,const llvm::Value*> m_variables;
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
