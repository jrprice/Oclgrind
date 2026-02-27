// WorkItem.h (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

namespace llvm
{
class BasicBlock;
class CallInst;
class ConstExpr;
class DILocalVariable;
class Function;
class Module;
} // namespace llvm

namespace oclgrind
{
class Context;
class Kernel;
class KernelInvocation;
class Memory;
class WorkGroup;
class WorkItem;
class WorkItemBuiltins;

// Data structures for builtin functions
struct BuiltinFunction
{
  void (*func)(WorkItem*, const llvm::CallInst*, const std::string&,
               const std::string&, TypedValue&, void*);
  void* op;
  BuiltinFunction(){};
  BuiltinFunction(void (*f)(WorkItem*, const llvm::CallInst*,
                            const std::string&, const std::string&, TypedValue&,
                            void*),
                  void* o)
      : func(f), op(o){};
};
typedef std::unordered_map<std::string, BuiltinFunction> BuiltinFunctionMap;
typedef std::list<std::pair<std::string, BuiltinFunction>>
  BuiltinFunctionPrefixList;

extern BuiltinFunctionMap workItemBuiltins;
extern BuiltinFunctionPrefixList workItemPrefixBuiltins;

// Per-kernel cache for various interpreter state information
class InterpreterCache
{
public:
  struct Builtin
  {
    BuiltinFunction function;
    std::string name, overload;
  };

  InterpreterCache(llvm::Function* kernel);
  ~InterpreterCache();

  void addBuiltin(const llvm::Function* function);
  Builtin getBuiltin(const llvm::Function* function) const;

  void addConstant(const llvm::Value* constant);
  TypedValue getConstant(const llvm::Value* operand) const;
  const llvm::Instruction* getConstantExpr(const llvm::Value* expr) const;

  unsigned addValueID(const llvm::Value* value);
  unsigned getValueID(const llvm::Value* value) const;
  unsigned getNumValues() const;
  bool hasValue(const llvm::Value* value) const;

private:
  typedef std::unordered_map<const llvm::Value*, unsigned> ValueMap;
  typedef std::unordered_map<const llvm::Function*, Builtin> BuiltinMap;
  typedef std::unordered_map<const llvm::Value*, TypedValue> ConstantMap;
  typedef std::unordered_map<const llvm::Value*, llvm::Instruction*>
    ConstExprMap;

  BuiltinMap m_builtins;
  ConstantMap m_constants;
  ConstExprMap m_constExpressions;
  ValueMap m_valueIDs;

  void addOperand(const llvm::Value* value);
};

class WorkItem
{
  friend class WorkItemBuiltins;

public:
  enum State
  {
    READY,
    BARRIER,
    FINISHED
  };

public:
  WorkItem(const KernelInvocation* kernelInvocation, WorkGroup* workGroup,
           Size3 lid);
  virtual ~WorkItem();

  void clearBarrier();
  void dispatch(const llvm::Instruction* instruction, TypedValue& result);
  void execute(const llvm::Instruction* instruction);
  const std::stack<const llvm::Instruction*>& getCallStack() const;
  const llvm::BasicBlock* getCurrentBlock() const;
  const llvm::Instruction* getCurrentInstruction() const;
  Size3 getGlobalID() const;
  size_t getGlobalIndex() const;
  Size3 getLocalID() const;
  TypedValue getOperand(const llvm::Value* operand) const;
  const llvm::BasicBlock* getPreviousBlock() const;
  Memory* getPrivateMemory() const;
  State getState() const;
  const unsigned char* getValueData(const llvm::Value* value) const;
  const WorkGroup* getWorkGroup() const;
  void printExpression(std::string expr) const;
  bool printValue(const llvm::Value* value) const;
  State step();

  // SPIR instructions
private:
#define INSTRUCTION(name)                                                      \
  void name(const llvm::Instruction* instruction, TypedValue& result)
  INSTRUCTION(add);
  INSTRUCTION(addrspacecast);
  INSTRUCTION(alloc);
  INSTRUCTION(ashr);
  INSTRUCTION(bitcast);
  INSTRUCTION(br);
  INSTRUCTION(bwand);
  INSTRUCTION(bwor);
  INSTRUCTION(bwxor);
  INSTRUCTION(call);
  INSTRUCTION(extractelem);
  INSTRUCTION(extractval);
  INSTRUCTION(fadd);
  INSTRUCTION(fcmp);
  INSTRUCTION(fdiv);
  INSTRUCTION(fmul);
  INSTRUCTION(fneg);
  INSTRUCTION(fpext);
  INSTRUCTION(fptosi);
  INSTRUCTION(fptoui);
  INSTRUCTION(fptrunc);
  INSTRUCTION(frem);
  INSTRUCTION(fsub);
  INSTRUCTION(gep);
  INSTRUCTION(icmp);
  INSTRUCTION(insertelem);
  INSTRUCTION(insertval);
  INSTRUCTION(inttoptr);
  INSTRUCTION(itrunc);
  INSTRUCTION(load);
  INSTRUCTION(lshr);
  INSTRUCTION(mul);
  INSTRUCTION(phi);
  INSTRUCTION(ptrtoint);
  INSTRUCTION(ret);
  INSTRUCTION(sdiv);
  INSTRUCTION(select);
  INSTRUCTION(sext);
  INSTRUCTION(shl);
  INSTRUCTION(shuffle);
  INSTRUCTION(sitofp);
  INSTRUCTION(srem);
  INSTRUCTION(store);
  INSTRUCTION(sub);
  INSTRUCTION(swtch);
  INSTRUCTION(udiv);
  INSTRUCTION(uitofp);
  INSTRUCTION(urem);
  INSTRUCTION(zext);
  INSTRUCTION(freeze);
#undef INSTRUCTION

private:
  typedef std::map<std::string,
                   std::pair<const llvm::Value*, const llvm::DILocalVariable*>>
    VariableMap;

  size_t m_globalIndex;
  Size3 m_globalID;
  Size3 m_localID;
  TypedValueMap m_phiTemps;
  VariableMap m_variables;
  const Context* m_context;
  const KernelInvocation* m_kernelInvocation;
  Memory* m_privateMemory;
  WorkGroup* m_workGroup;
  mutable MemoryPool m_pool;

  State m_state;
  struct Position;
  Position* m_position;

  Memory* getMemory(unsigned int addrSpace) const;

  // Store for instruction results and other operand values
  std::vector<TypedValue> m_values;
  TypedValue getValue(const llvm::Value* key) const;
  bool hasValue(const llvm::Value* key) const;
  void setValue(const llvm::Value* key, TypedValue value);

  const InterpreterCache* m_cache;
};
} // namespace oclgrind
