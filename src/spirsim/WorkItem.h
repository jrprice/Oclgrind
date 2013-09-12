#include "common.h"

namespace llvm
{
  class DbgValueInst;
  class CallInst;
}

namespace spirsim
{
  class Kernel;
  class Memory;
  class WorkGroup;

  class WorkItem
  {
  public:
    enum State {READY, BARRIER, WAIT_EVENT, FINISHED};

  public:
    WorkItem(WorkGroup& workGroup,
             const Kernel& kernel, Memory &globalMem,
             size_t lid_x, size_t lid_y, size_t lid_z);
    virtual ~WorkItem();

    void clearBarrier();
    void dispatch(const llvm::Instruction& instruction, TypedValue& result);
    void dumpPrivateMemory() const;
    void enableDebugOutput(bool enable);
    void execute(const llvm::Instruction& instruction);
    const size_t* getGlobalID() const;
    double getFloatValue(const llvm::Value *operand,
                         unsigned int index = 0) const;
    int64_t getSignedInt(const llvm::Value *operand,
                         unsigned int index = 0) const;
    State getState() const;
    uint64_t getUnsignedInt(const llvm::Value *operand,
                            unsigned int index = 0) const;
    void outputMemoryError(const llvm::Instruction& instruction,
                           const std::string& msg,
                           unsigned addressSpace,
                           size_t address, size_t size) const;
    TypedValue resolveConstExpr(const llvm::ConstantExpr *expr);
    void setFloatResult(TypedValue& result, double val,
                        unsigned int index = 0) const;
    void setIntResult(TypedValue& result, int64_t val,
                      unsigned int index = 0) const;
    void setIntResult(TypedValue& result, uint64_t val,
                      unsigned int index = 0) const;
    State step(bool debugOutput = false);
    void trap();
    void updateVariable(const llvm::DbgValueInst *instruction);

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

#define DECLARE_BUILTIN(name)      void name(const llvm::CallInst *callInst, \
                                             std::string name,               \
                                             std::string overload,           \
                                             TypedValue& result);
    // Async Copy and Prefetch Functions
    DECLARE_BUILTIN(async_work_group_copy);
    DECLARE_BUILTIN(wait_group_events);
    DECLARE_BUILTIN(prefetch);

    // Common Functions
    DECLARE_BUILTIN(clamp);
    DECLARE_BUILTIN(max);
    DECLARE_BUILTIN(min);

    // Geometric Functions
    DECLARE_BUILTIN(cross);
    DECLARE_BUILTIN(dot);
    DECLARE_BUILTIN(distance);
    DECLARE_BUILTIN(length);
    DECLARE_BUILTIN(normalize);

    // Integer Functions
    DECLARE_BUILTIN(clz);
    DECLARE_BUILTIN(hadd);
    DECLARE_BUILTIN(rotate);

    // Math Functions
    DECLARE_BUILTIN(fract);
    DECLARE_BUILTIN(frexp_builtin);
    DECLARE_BUILTIN(ilogb_builtin);
    DECLARE_BUILTIN(ldexp_builtin);
    DECLARE_BUILTIN(lgamma_r);
    DECLARE_BUILTIN(modf_builtin);
    DECLARE_BUILTIN(nan_builtin);
    DECLARE_BUILTIN(pown);
    DECLARE_BUILTIN(remquo_builtin);
    DECLARE_BUILTIN(rootn);
    DECLARE_BUILTIN(sincos);

    // Relational Functions
    DECLARE_BUILTIN(all);
    DECLARE_BUILTIN(any);
    DECLARE_BUILTIN(bitselect);
    DECLARE_BUILTIN(select_builtin);

    // Synchronization Functions
    DECLARE_BUILTIN(barrier);
    DECLARE_BUILTIN(mem_fence);

    // Vector Data Load and Store Functions
    DECLARE_BUILTIN(vload);
    DECLARE_BUILTIN(vstore);

    // Work-Item Functions
    DECLARE_BUILTIN(get_global_id);
    DECLARE_BUILTIN(get_global_size);
    DECLARE_BUILTIN(get_global_offset);
    DECLARE_BUILTIN(get_group_id);
    DECLARE_BUILTIN(get_local_id);
    DECLARE_BUILTIN(get_local_size);
    DECLARE_BUILTIN(get_num_groups);
    DECLARE_BUILTIN(get_work_dim);

    // Other Functions
    DECLARE_BUILTIN(printf_builtin);

    // LLVM Intrinisics
    DECLARE_BUILTIN(llvm_dbg_declare);
    DECLARE_BUILTIN(llvm_dbg_value);
    DECLARE_BUILTIN(llvm_lifetime_start);
    DECLARE_BUILTIN(llvm_lifetime_end);
    DECLARE_BUILTIN(llvm_memcpy);
    DECLARE_BUILTIN(llvm_memset);
    DECLARE_BUILTIN(llvm_trap);

    void builtin_f1arg(const llvm::CallInst *callInst, TypedValue& result,
                       double (*func)(double));
    void builtin_f2arg(const llvm::CallInst *callInst, TypedValue& result,
                       double (*func)(double, double));
    void builtin_f3arg(const llvm::CallInst *callInst, TypedValue& result,
                       double (*func)(double, double, double));
    void builtin_u1arg(const llvm::CallInst *callInst, TypedValue& result,
                       uint64_t (*func)(uint64_t));
    void builtin_u2arg(const llvm::CallInst *callInst, TypedValue& result,
                       uint64_t (*func)(uint64_t, uint64_t));
    void builtin_u3arg(const llvm::CallInst *callInst, TypedValue& result,
                       uint64_t (*func)(uint64_t, uint64_t, uint64_t));
    void builtin_s1arg(const llvm::CallInst *callInst, TypedValue& result,
                       int64_t (*func)(int64_t));
    void builtin_s2arg(const llvm::CallInst *callInst, TypedValue& result,
                       int64_t (*func)(int64_t, int64_t));
    void builtin_s3arg(const llvm::CallInst *callInst, TypedValue& result,
                       int64_t (*func)(int64_t, int64_t, int64_t));
    void builtin_rel1arg(const llvm::CallInst *callInst, TypedValue& result,
                         int (*func)(double));
    void builtin_rel2arg(const llvm::CallInst *callInst, TypedValue& result,
                         int (*func)(double, double));

  private:
    size_t m_globalID[3];
    size_t m_localID[3];
    TypedValueMap m_privateMemory;
    TypedValueMap m_phiTemps;
    std::map<std::string,const llvm::Value*> m_variables;
    const Kernel& m_kernel;
    Memory *m_stack;
    Memory& m_globalMemory;
    WorkGroup& m_workGroup;

    typedef std::pair<llvm::Function::const_iterator,
              llvm::BasicBlock::const_iterator> ReturnAddress;

    State m_state;
    llvm::Function::const_iterator m_prevBlock;
    llvm::Function::const_iterator m_currBlock;
    llvm::Function::const_iterator m_nextBlock;
    llvm::BasicBlock::const_iterator m_currInst;
    std::stack<ReturnAddress> m_callStack;
    bool m_debugOutput;
  };
}

double degrees(double x);
double mix(double x, double y, double a);
double radians(double x);
double sign(double x);
double smoothstep(double edge0, double edge1, double x);
double step_builtin(double edge, double x);

uint64_t popcount(uint64_t x);

double acospi(double x);
double asinpi(double x);
double atanpi(double x);
double atan2pi(double x, double y);
double cospi(double x);
double exp10(double x);
double fdivide(double x, double y);
double frecip(double x);
double maxmag(double x, double y);
double minmag(double x, double y);
double rsqrt(double x);
double sinpi(double x);
double tanpi(double x);

int isequal_builtin(double x, double y);
int isnotequal_builtin(double x, double y);
int isgreater_builtin(double x, double y);
int isgreaterequal_builtin(double x, double y);
int isless_builtin(double x, double y);
int islessequal_builtin(double x, double y);
int islessgreater_builtin(double x, double y);
int isfinite_builtin(double x);
int isinf_builtin(double x);
int isnan_builtin(double x);
int isnormal_builtin(double x);
int isordered_builtin(double x, double y);
int isunordered_builtin(double x, double y);
int signbit_builtin(double x);
