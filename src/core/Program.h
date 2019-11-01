// Program.h (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

namespace llvm
{
  class Function;
  class LLVMContext;
  class Module;
  class StoreInst;
}

namespace oclgrind
{
  class Context;
  class InterpreterCache;
  class Kernel;

  class Program
  {
  public:
    typedef std::pair<std::string, const Program*> Header;

  public:
    Program(const Context *context, const std::string& source);
    virtual ~Program();

    static Program* createFromBitcode(const Context *context,
                                      const unsigned char *bitcode,
                                      size_t length);
    static Program* createFromBitcodeFile(const Context *context,
                                          const std::string filename);
    static Program* createFromPrograms(const Context *context,
                                       std::list<const Program*>);

    bool build(const char *options,
               std::list<Header> headers = std::list<Header>());
    Kernel* createKernel(const std::string name);
    const std::string& getBuildLog() const;
    const std::string& getBuildOptions() const;
    void getBinary(unsigned char *binary) const;
    size_t getBinarySize() const;
    unsigned int getBuildStatus() const;
    const Context *getContext() const;
    const InterpreterCache* getInterpreterCache(
      const llvm::Function *kernel) const;
    std::list<std::string> getKernelNames() const;
    llvm::LLVMContext& getLLVMContext() const;
    unsigned int getNumKernels() const;
    const std::string& getSource() const;
    const char* getSourceLine(size_t lineNumber) const;
    size_t getNumSourceLines() const;
    const TypedValue& getProgramScopeVar(const llvm::Value *var) const;
    size_t getTotalProgramScopeVarSize() const;
    unsigned long getUID() const;

  private:
    Program(const Context *context, llvm::Module *module);

    std::unique_ptr<llvm::Module> m_module;
    std::string m_source;
    std::string m_buildLog;
    std::string m_buildOptions;
    unsigned int m_buildStatus;
    const Context *m_context;
    std::vector<std::string> m_sourceLines;

    TypedValueMap m_programScopeVars;
    size_t m_totalProgramScopeVarSize;

    unsigned long m_uid;
    unsigned long generateUID() const;

    void allocateProgramScopeVars();
    void deallocateProgramScopeVars();
    void pruneDeadCode(llvm::Instruction*);
    void removeLValueLoads();
    void scalarizeAggregateStore(llvm::StoreInst *store);
    void stripDebugIntrinsics();

    typedef std::map<const llvm::Function*, InterpreterCache*>
      InterpreterCacheMap;
    mutable InterpreterCacheMap m_interpreterCache;
    void clearInterpreterCache();
  };
}
