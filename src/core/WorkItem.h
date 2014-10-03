// WorkItem.h (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
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

#include "llvm/Function.h"

namespace llvm
{
  class DbgValueInst;
  class CallInst;
}

namespace oclgrind
{
  class Device;
  class Kernel;
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
  extern std::MAP<std::string,BuiltinFunction> workItemBuiltins;
  extern std::list< std::pair<std::string,
                              BuiltinFunction> > workItemPrefixBuiltins;

  // Return address for a function call
  typedef std::pair<llvm::Function::const_iterator,
                    llvm::BasicBlock::const_iterator> ReturnAddress;

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
    } m_pool;

  public:
    // Per-program cache for various interpreter state information
    class InterpreterCache
    {
    public:
      typedef struct
      {
        BuiltinFunction function;
        std::string name, overload;
      } Builtin;

    public:
      static void clear(unsigned long uid);
      static InterpreterCache* get(unsigned long uid);

      std::MAP<const llvm::Value*, size_t> valueIDs;
      std::MAP<const llvm::Function*, Builtin> builtins;
      TypedValue getConstant(const llvm::Value *operand);

    private:
      static std::MAP<unsigned long, InterpreterCache*> m_cache;

      InterpreterCache();
      ~InterpreterCache();
      std::MAP<const llvm::Value*, TypedValue> m_constants;
    };

  public:
    WorkItem(Device *device, WorkGroup& workGroup, const Kernel& kernel,
             size_t lid_x, size_t lid_y, size_t lid_z);
    virtual ~WorkItem();

    void clearBarrier();
    static void clearInstructionCounts();
    void dispatch(const llvm::Instruction *instruction, TypedValue& result);
    void execute(const llvm::Instruction *instruction);
    const std::stack<ReturnAddress>& getCallStack() const;
    static std::string getCountedOpcodeName(unsigned opcode);
    const llvm::Instruction* getCurrentInstruction() const;
    Device* getDevice() {return m_device;}
    const size_t* getGlobalID() const;
    size_t getGlobalIndex() const;
    static std::vector<size_t> getInstructionCounts();
    const size_t* getLocalID() const;
    TypedValue getOperand(const llvm::Value *operand);
    Memory* getPrivateMemory() const;
    State getState() const;
    const unsigned char* getValueData(const llvm::Value *value) const;
    const llvm::Value* getVariable(std::string name) const;
    bool printValue(const llvm::Value *value);
    bool printVariable(std::string name);
    TypedValue resolveConstExpr(const llvm::ConstantExpr *expr);
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
    size_t m_globalIndex;
    size_t m_globalID[3];
    size_t m_localID[3];
    TypedValueMap m_phiTemps;
    std::map<std::string, const llvm::Value*> m_variables;
    Device *m_device;
    const Kernel& m_kernel;
    Memory *m_privateMemory;
    WorkGroup& m_workGroup;

    State m_state;
    llvm::Function::const_iterator m_prevBlock;
    llvm::Function::const_iterator m_currBlock;
    llvm::Function::const_iterator m_nextBlock;
    llvm::BasicBlock::const_iterator m_currInst;
    std::stack<ReturnAddress> m_callStack;

    Memory* getMemory(unsigned int addrSpace);

    // Store for instruction results and other operand values
    std::vector<TypedValue> m_values;
    TypedValue get(const llvm::Value *key) const;
    bool has(const llvm::Value *key) const;
    void set(const llvm::Value *key, TypedValue value);

    InterpreterCache *m_cache;

    void countInstruction(const llvm::Instruction *instruction);
    static std::vector<size_t> m_instructionCounts;
    static std::vector<size_t> m_memopBytes;
    static std::vector<const llvm::Function*> m_countedFunctions;
  };
}
