// WorkItem.h (oclgrind)
// Copyright (C) 2013 James Price
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.

#include "common.h"

#include "llvm/Function.h"

namespace llvm
{
  class DbgValueInst;
  class CallInst;
}

namespace spirsim
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
                 std::string, std::string, TypedValue&, void*);
    void *op;
    _BuiltinFunction(){};
    _BuiltinFunction(void (*f)(WorkItem*, const llvm::CallInst*,
                     std::string, std::string, TypedValue&, void*),
                     void *o) : func(f), op(o) {};
  } BuiltinFunction;
  extern std::map<std::string,BuiltinFunction> workItemBuiltins;
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

  public:
    WorkItem(Device *device, WorkGroup& workGroup, const Kernel& kernel,
             size_t lid_x, size_t lid_y, size_t lid_z);
    virtual ~WorkItem();

    void clearBarrier();
    void dispatch(const llvm::Instruction& instruction, TypedValue& result);
    void dumpPrivateMemory();
    void execute(const llvm::Instruction& instruction);
    const std::stack<ReturnAddress>& getCallStack() const;
    const llvm::Instruction* getCurrentInstruction() const;
    const size_t* getGlobalID() const;
    double getFloatValue(const llvm::Value *operand,
                         unsigned int index = 0);
    const size_t* getLocalID() const;
    Memory* getPrivateMemory() const;
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
    void trap();

    // SPIR instructions
    void add(const llvm::Instruction& instruction, TypedValue& result);
    void alloc(const llvm::Instruction& instruction, TypedValue& result);
    void ashr(const llvm::Instruction& instruction, TypedValue& result);
    void bitcast(const llvm::Instruction& instruction, TypedValue& result);
    void br(const llvm::Instruction& instruction);
    void bwand(const llvm::Instruction& instruction, TypedValue& result);
    void bwor(const llvm::Instruction& instruction, TypedValue& result);
    void bwxor(const llvm::Instruction& instruction, TypedValue& result);
    void call(const llvm::Instruction& instruction, TypedValue& result);
    void extract(const llvm::Instruction& instruction, TypedValue& result);
    void fadd(const llvm::Instruction& instruction, TypedValue& result);
    void fcmp(const llvm::Instruction& instruction, TypedValue& result);
    void fdiv(const llvm::Instruction& instruction, TypedValue& result);
    void fmul(const llvm::Instruction& instruction, TypedValue& result);
    void fpext(const llvm::Instruction& instruction, TypedValue& result);
    void fptosi(const llvm::Instruction& instruction, TypedValue& result);
    void frem(const llvm::Instruction& instruction, TypedValue& result);
    void fsub(const llvm::Instruction& instruction, TypedValue& result);
    void gep(const llvm::Instruction& instruction, TypedValue& result);
    void icmp(const llvm::Instruction& instruction, TypedValue& result);
    void insert(const llvm::Instruction& instruction, TypedValue& result);
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
    size_t m_globalID[3];
    size_t m_localID[3];
    TypedValueMap m_instResults;
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
  };
}
