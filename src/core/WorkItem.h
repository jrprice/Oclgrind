// WorkItem.h (Oclgrind)
// Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

#if HAVE_CXX11
#include <unordered_map>
#define MAP unordered_map
#else
#define MAP map
#endif

namespace llvm
{
  class CallInst;
  class DbgValueInst;
  class Function;
  class Module;
}

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
  typedef struct _BuiltinFunction
  {
    void (*func)(WorkItem*, const llvm::CallInst*,
                 const std::string&, const std::string&, TypedValue&, void*);
    void *op;
    _BuiltinFunction(){};
    _BuiltinFunction(void (*f)(WorkItem*, const llvm::CallInst*,
                     const std::string&, const std::string&, TypedValue&,
                     void*),
                     void *o) : func(f), op(o) {};
  } BuiltinFunction;
  typedef std::MAP<std::string,BuiltinFunction> BuiltinFunctionMap;
  typedef std::list< std::pair<std::string, BuiltinFunction> >
    BuiltinFunctionPrefixList;

  extern BuiltinFunctionMap workItemBuiltins;
  extern BuiltinFunctionPrefixList workItemPrefixBuiltins;

  // Per-kernel cache for various interpreter state information
  class InterpreterCache
  {
  public:
    typedef struct
    {
      BuiltinFunction function;
      std::string name, overload;
    } Builtin;

    InterpreterCache(llvm::Function *kernel);
    ~InterpreterCache();

    void addBuiltin(const llvm::Function *function);
    Builtin getBuiltin(const llvm::Function *function) const;

    void addConstant(const llvm::Value *constant);
    TypedValue getConstant(const llvm::Value *operand) const;

    unsigned addValueID(const llvm::Value *value);
    unsigned getValueID(const llvm::Value *value) const;
    unsigned getNumValues() const;
    bool hasValue(const llvm::Value *value) const;

  private:
    typedef std::MAP<const llvm::Value*, unsigned> ValueMap;
    typedef std::MAP<const llvm::Function*, Builtin> BuiltinMap;
    typedef std::MAP<const llvm::Value*, TypedValue> ConstantMap;

    BuiltinMap m_builtins;
    ConstantMap m_constants;
    ValueMap m_valueIDs;

    void addOperand(const llvm::Value *value);
  };

  class WorkItem
  {
    friend class WorkItemBuiltins;

  public:
    enum State {READY, BARRIER, FINISHED};

  private:
    class MemoryPool
    {
    public:
      MemoryPool(size_t blockSize = 1024);
      ~MemoryPool();
      unsigned char* alloc(size_t size);
      TypedValue clone(const TypedValue& source);
    private:
      size_t m_blockSize;
      size_t m_offset;
      std::list<unsigned char *> m_blocks;
    } mutable m_pool;

  public:
    WorkItem(const KernelInvocation *kernelInvocation,
             WorkGroup *workGroup, Size3 lid);
    virtual ~WorkItem();

    void clearBarrier();
    void dispatch(const llvm::Instruction *instruction, TypedValue& result);
    void execute(const llvm::Instruction *instruction);
    const std::stack<const llvm::Instruction*>& getCallStack() const;
    const llvm::Instruction* getCurrentInstruction() const;
    Size3 getGlobalID() const;
    size_t getGlobalIndex() const;
    Size3 getLocalID() const;
    TypedValue getOperand(const llvm::Value *operand) const;
    Memory* getPrivateMemory() const;
    State getState() const;
    const unsigned char* getValueData(const llvm::Value *value) const;
    const llvm::Value* getVariable(std::string name) const;
    const WorkGroup* getWorkGroup() const;
    bool printValue(const llvm::Value *value) const;
    bool printVariable(std::string name) const;
    State step();

    // SPIR instructions
  private:
#define INSTRUCTION(name) \
  void name(const llvm::Instruction *instruction, TypedValue& result)
    INSTRUCTION(add);
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
#undef INSTRUCTION

  private:
    typedef std::map<std::string, const llvm::Value*> VariableMap;

    size_t m_globalIndex;
    Size3 m_globalID;
    Size3 m_localID;
    TypedValueMap m_phiTemps;
    VariableMap m_variables;
    const Context *m_context;
    const KernelInvocation *m_kernelInvocation;
    Memory *m_privateMemory;
    WorkGroup *m_workGroup;

    State m_state;
    struct Position;
    Position *m_position;

    Memory* getMemory(unsigned int addrSpace) const;

    // Store for instruction results and other operand values
    std::vector<TypedValue> m_values;
    TypedValue getValue(const llvm::Value *key) const;
    bool hasValue(const llvm::Value *key) const;
    void setValue(const llvm::Value *key, TypedValue value);

    const InterpreterCache *m_cache;
  };
}
