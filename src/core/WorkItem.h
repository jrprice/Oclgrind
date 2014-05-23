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

  private:
    typedef struct
    {
      BuiltinFunction function;
      std::string name, overload;
    } CachedBuiltin;

    // Per-program interpreter state, shared between all work-items
    typedef struct
    {
      std::MAP<const llvm::Value*, size_t> valueIDs;
      std::map<const llvm::Function*, CachedBuiltin> builtins;
    } InterpreterState;

  public:
    // Per-program cache for various interpreter state information
    class InterpreterCache
    {
    public:
      static void clear(unsigned long uid);
      static InterpreterState* get(unsigned long uid);
    private:
      static std::MAP<unsigned long, InterpreterState*> m_cache;
    };

  public:
    WorkItem(Device *device, WorkGroup& workGroup, const Kernel& kernel,
             size_t lid_x, size_t lid_y, size_t lid_z);
    virtual ~WorkItem();

    void clearBarrier();
    static void clearInstructionCounts();
    void dispatch(const llvm::Instruction& instruction, TypedValue& result);
    void execute(const llvm::Instruction& instruction);
    const std::stack<ReturnAddress>& getCallStack() const;
    const llvm::Instruction* getCurrentInstruction() const;
    const size_t* getGlobalID() const;
    size_t getGlobalIndex() const;
    double getFloatValue(const llvm::Value *operand,
                         unsigned int index = 0);
    static std::vector<size_t> getInstructionCounts();
    const size_t* getLocalID() const;
    static std::string getCountedOpcodeName(unsigned opcode);
    Memory* getPrivateMemory() const;
    size_t getPointer(const llvm::Value *operand, unsigned int index = 0);
    int64_t getSignedInt(const llvm::Value *operand,
                         unsigned int index = 0);
    State getState() const;
    uint64_t getUnsignedInt(const llvm::Value *operand,
                            unsigned int index = 0);
    const unsigned char* getValueData(const llvm::Value *value) const;
    const llvm::Value* getVariable(std::string name) const;
    bool printValue(const llvm::Value *value);
    bool printVariable(std::string name);
    TypedValue resolveConstExpr(const llvm::ConstantExpr *expr);
    static void setFloatResult(TypedValue& result, double val,
                               unsigned int index = 0);
    static void setIntResult(TypedValue& result, int64_t val,
                             unsigned int index = 0);
    static void setIntResult(TypedValue& result, uint64_t val,
                             unsigned int index = 0);
    State step();

    // SPIR instructions
  private:
    void add(const llvm::Instruction& instruction, TypedValue& result);
    void alloc(const llvm::Instruction& instruction, TypedValue& result);
    void ashr(const llvm::Instruction& instruction, TypedValue& result);
    void bitcast(const llvm::Instruction& instruction, TypedValue& result);
    void br(const llvm::Instruction& instruction);
    void bwand(const llvm::Instruction& instruction, TypedValue& result);
    void bwor(const llvm::Instruction& instruction, TypedValue& result);
    void bwxor(const llvm::Instruction& instruction, TypedValue& result);
    void call(const llvm::Instruction& instruction, TypedValue& result);
    void extractelem(const llvm::Instruction& instruction, TypedValue& result);
    void extractval(const llvm::Instruction& instruction, TypedValue& result);
    void fadd(const llvm::Instruction& instruction, TypedValue& result);
    void fcmp(const llvm::Instruction& instruction, TypedValue& result);
    void fdiv(const llvm::Instruction& instruction, TypedValue& result);
    void fmul(const llvm::Instruction& instruction, TypedValue& result);
    void fpext(const llvm::Instruction& instruction, TypedValue& result);
    void fptosi(const llvm::Instruction& instruction, TypedValue& result);
    void fptoui(const llvm::Instruction& instruction, TypedValue& result);
    void fptrunc(const llvm::Instruction& instruction, TypedValue& result);
    void frem(const llvm::Instruction& instruction, TypedValue& result);
    void fsub(const llvm::Instruction& instruction, TypedValue& result);
    void gep(const llvm::Instruction& instruction, TypedValue& result);
    void icmp(const llvm::Instruction& instruction, TypedValue& result);
    void insertelem(const llvm::Instruction& instruction, TypedValue& result);
    void insertval(const llvm::Instruction& instruction, TypedValue& result);
    void inttoptr(const llvm::Instruction& instruction, TypedValue& result);
    void itrunc(const llvm::Instruction& instruction, TypedValue& result);
    void load(const llvm::Instruction& instruction, TypedValue& result);
    void lshr(const llvm::Instruction& instruction, TypedValue& result);
    void mul(const llvm::Instruction& instruction, TypedValue& result);
    void phi(const llvm::Instruction& instruction, TypedValue& result);
    void ptrtoint(const llvm::Instruction& instruction, TypedValue& result);
    void ret(const llvm::Instruction& instruction, TypedValue& result);
    void sdiv(const llvm::Instruction& instruction, TypedValue& result);
    void select(const llvm::Instruction& instruction, TypedValue& result);
    void sext(const llvm::Instruction& instruction, TypedValue& result);
    void shl(const llvm::Instruction& instruction, TypedValue& result);
    void shuffle(const llvm::Instruction& instruction, TypedValue& result);
    void sitofp(const llvm::Instruction& instruction, TypedValue& result);
    void srem(const llvm::Instruction& instruction, TypedValue& result);
    void store(const llvm::Instruction& instruction);
    void sub(const llvm::Instruction& instruction, TypedValue& result);
    void swtch(const llvm::Instruction& instruction);
    void udiv(const llvm::Instruction& instruction, TypedValue& result);
    void uitofp(const llvm::Instruction& instruction, TypedValue& result);
    void urem(const llvm::Instruction& instruction, TypedValue& result);
    void zext(const llvm::Instruction& instruction, TypedValue& result);

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

    InterpreterState *m_cache;

    void countInstruction(const llvm::Instruction& instruction);
    static std::vector<size_t> m_instructionCounts;
    static std::vector<size_t> m_memopBytes;
    static std::vector<const llvm::Function*> m_countedFunctions;
  };
}
