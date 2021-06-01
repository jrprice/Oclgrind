// Program.cpp (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"
#include "config.h"

#include <fstream>

#if defined(_WIN32) && !defined(__MINGW32__)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "Context.h"
#include "Kernel.h"
#include "Memory.h"
#include "Program.h"
#include "WorkItem.h"

#define ENV_DUMP_SPIR "OCLGRIND_DUMP_SPIR"
#define CL_DUMP_NAME "/tmp/oclgrind_%lX.cl"
#define IR_DUMP_NAME "/tmp/oclgrind_%lX.s"
#define BC_DUMP_NAME "/tmp/oclgrind_%lX.bc"

#if defined(_WIN32)
#define REMAP_DIR "Z:/remapped/"
#else
#define REMAP_DIR "/remapped/"
#endif

#define REMAP_INPUT "input.cl"
#define OPENCL_C_H_PATH REMAP_DIR "opencl-c.h"
extern const char OPENCL_C_H_DATA[];

const char* EXTENSIONS[] = {
  "cl_khr_fp64",
  "cl_khr_3d_image_writes",
  "cl_khr_global_int32_base_atomics",
  "cl_khr_global_int32_extended_atomics",
  "cl_khr_local_int32_base_atomics",
  "cl_khr_local_int32_extended_atomics",
  "cl_khr_int64_base_atomics",
  "cl_khr_int64_extended_atomics",
  "cl_khr_byte_addressable_store",
};

using namespace oclgrind;
using namespace std;

Program::Program(const Context* context, llvm::Module* module)
    : m_module(module), m_context(context)
{
  m_buildLog = "";
  m_buildOptions = "";
  m_buildStatus = CL_BUILD_SUCCESS;
  m_uid = generateUID();
  m_totalProgramScopeVarSize = 0;

  allocateProgramScopeVars();
}

Program::Program(const Context* context, const string& source)
    : m_context(context)
{
  m_source = source;
  m_buildLog = "";
  m_buildOptions = "";
  m_buildStatus = CL_BUILD_NONE;
  m_uid = 0;
  m_totalProgramScopeVarSize = 0;

  // Split source into individual lines
  m_sourceLines.clear();
  if (!source.empty())
  {
    std::stringstream ss(source);
    std::string line;
    while (std::getline(ss, line, '\n'))
    {
      m_sourceLines.push_back(line);
    }
  }
}

Program::~Program()
{
  clearInterpreterCache();
  deallocateProgramScopeVars();
}

void Program::allocateProgramScopeVars()
{
  deallocateProgramScopeVars();

  Memory* globalMemory = m_context->getGlobalMemory();

  // Create the pointer values for each global variable
  llvm::Module::const_global_iterator itr;
  for (itr = m_module->global_begin(); itr != m_module->global_end(); itr++)
  {
    unsigned addrspace = itr->getType()->getPointerAddressSpace();
    if (addrspace != AddrSpaceGlobal && addrspace != AddrSpaceConstant)
      continue;

    // Allocate global variable
    const llvm::Type* type = itr->getType()->getPointerElementType();
    size_t size = getTypeSize(type);
    size_t ptr = globalMemory->allocateBuffer(size);
    m_totalProgramScopeVarSize += size;

    // Create pointer value
    TypedValue ptrValue = {sizeof(size_t), 1, new uint8_t[sizeof(size_t)]};
    ptrValue.setPointer(ptr);
    m_programScopeVars[&*itr] = ptrValue;
  }

  try
  {
    // Initialize global variables
    for (auto itr = m_programScopeVars.begin(); itr != m_programScopeVars.end();
         itr++)
    {
      auto var = llvm::cast<llvm::GlobalVariable>(itr->first);
      const llvm::Constant* initializer = var->getInitializer();
      if (!initializer)
        continue;

      size_t varptr = itr->second.getPointer();
      if (initializer->getType()->getTypeID() == llvm::Type::PointerTyID)
      {
        size_t ptr = resolveConstantPointer(initializer, m_programScopeVars);
        globalMemory->store((uint8_t*)&ptr, varptr, sizeof(size_t));
      }
      else
      {
        size_t size = getTypeSize(initializer->getType());
        uint8_t* data = new uint8_t[size];
        getConstantData((uint8_t*)data, (const llvm::Constant*)initializer);
        globalMemory->store(data, varptr, size);
        delete[] data;
      }
    }
  }
  catch (FatalError& err)
  {
    cerr << endl
         << "OCLGRIND FATAL ERROR "
         << "(" << err.getFile() << ":" << err.getLine() << ")" << endl
         << err.what() << endl
         << "When initializing program scope global variables" << endl;
  }
}

// Utility to split a string up to the next unquoted space
// After this returns, input will point to the start of the next string (no
// leading spaces), and next will point to where the next string will start.
// Modifies the content of input in place.
void split_token(char* input, char** next)
{
  char* output = input;

  // Strip leading spaces
  while (*input == ' ')
    input++;

  // Loop until end of string
  bool quoted = false;
  while (*input != '\0')
  {
    // Stop at space, unless we're in quotes
    if (*input == ' ' && !quoted)
      break;

    if (*input == '"')
    {
      // Enter/exit quoted region, don't emit quote
      quoted = !quoted;
    }
    else
    {
      // Check for escaped space
      if (*input == '\\' && *(input + 1) == ' ')
        input++;

      // Copy character to output string
      *output = *input;
      output++;
    }

    input++;
  }

  // Set *next to start of next potential string
  *next = input;
  if (**next != '\0')
    (*next)++;

  // Split token with null terminator
  *output = '\0';
}

bool Program::build(const char* options, list<Header> headers)
{
  m_buildStatus = CL_BUILD_IN_PROGRESS;
  m_buildOptions = options ? options : "";

  // Create build log
  m_buildLog = "";
  llvm::raw_string_ostream buildLog(m_buildLog);

  // Do nothing if program was created with binary
  if (m_source.empty() && m_module)
  {
    m_buildStatus = CL_BUILD_SUCCESS;

    allocateProgramScopeVars();

    return true;
  }

  if (m_module)
  {
    clearInterpreterCache();
    m_module.reset();
  }

  // Assign a new UID to this program
  m_uid = generateUID();

  // Set compiler arguments
  vector<const char*> args;
  args.push_back("-cl-kernel-arg-info");
  args.push_back("-D__IMAGE_SUPPORT__=1");
  args.push_back("-D__OPENCL_VERSION__=120");
  args.push_back("-fno-builtin");
  args.push_back("-fgnu89-inline");
  args.push_back("-debug-info-kind=standalone");
  args.push_back("-triple");
  if (sizeof(size_t) == 4)
    args.push_back("spir-unknown-unknown");
  else
    args.push_back("spir64-unknown-unknown");

#if !IS_BIG_ENDIAN
  args.push_back("-D__ENDIAN_LITTLE__=1");
#endif

  // Disable all extensions
  std::string cl_ext("-cl-ext=-all");
  // Explicitly enable supported extensions
  for (unsigned i = 0; i < sizeof(EXTENSIONS) / sizeof(const char*); i++)
  {
    cl_ext += ",+" + std::string(EXTENSIONS[i]);
  }
  args.push_back(cl_ext.c_str());

  bool defaultOptimization = true;
  const char* clstd = NULL;

  // Add OpenCL build options
  const char* mainOptions = options;
  const char* extraOptions = getenv("OCLGRIND_BUILD_OPTIONS");
  if (!mainOptions)
    mainOptions = "";
  if (!extraOptions)
    extraOptions = "";
  char* tmpOptions = new char[strlen(mainOptions) + strlen(extraOptions) + 2];
  sprintf(tmpOptions, "%s %s", mainOptions, extraOptions);
  char* opt = tmpOptions;
  char* next = NULL;
  while (strlen(opt) > 0)
  {
    // Split token up to next unquoted space
    if (next)
      opt = next;
    split_token(opt, &next);
    if (!strlen(opt))
      break;

    // Ignore options that break PCH
    if (strcmp(opt, "-cl-fast-relaxed-math") != 0 &&
        strcmp(opt, "-cl-finite-math-only") != 0 &&
        strcmp(opt, "-cl-single-precision-constant") &&
        strcmp(opt, "-cl-unsafe-math-optimizations") != 0)
    {
      // Check for optimization flags
      if (strncmp(opt, "-O", 2) == 0 || strcmp(opt, "-cl-opt-disable") == 0)
      {
        defaultOptimization = false;
      }

      // Clang no longer supports -cl-no-signed-zeros
      if (strcmp(opt, "-cl-no-signed-zeros") == 0)
        continue;

      // Handle -cl-denorms-are-zero
      if (strcmp(opt, "-cl-denorms-are-zero") == 0)
      {
        args.push_back("-fdenormal-fp-math=preserve-sign");
        continue;
      }

      // Check for -cl-std flag
      if (strncmp(opt, "-cl-std=", 8) == 0)
      {
        clstd = opt;
        continue;
      }

      args.push_back(opt);
    }
  }

  if (defaultOptimization)
  {
    // Disable optimizations by default if in interactive mode
    if (checkEnv("OCLGRIND_INTERACTIVE"))
      args.push_back("-O0");
    // Otherwise, default to optimizing for size
    else
      args.push_back("-Oz");
  }

  if (!clstd)
  {
    clstd = "-cl-std=CL1.2";
  }
  args.push_back(clstd);

  // Pre-compiled header
  char* pchdir = NULL;
  char* pch = NULL;
  if (!checkEnv("OCLGRIND_DISABLE_PCH") &&
      (!strcmp(clstd, "-cl-std=CL1.2") || !strcmp(clstd, "-cl-std=CL2.0")))
  {
    const char* pchdirOverride = getenv("OCLGRIND_PCH_DIR");
    if (pchdirOverride)
    {
      pchdir = strdup(pchdirOverride);
    }
    else
    {
      // Get directory containing library
#if defined(_WIN32) && !defined(__MINGW32__)
      char libpath[4096];
      HMODULE dll;
      if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                             (LPCSTR)&Program::createFromBitcode, &dll) &&
          GetModuleFileNameA(dll, libpath, sizeof(libpath)))
      {
#else
      Dl_info dlinfo;
      if (dladdr((const void*)Program::createFromBitcode, &dlinfo))
      {
        const char* libpath = dlinfo.dli_fname;
#endif

        // Construct path to PCH directory
        const char* dirend;
#if defined(_WIN32) && !defined(__MINGW32__)
        if ((dirend = strrchr(libpath, '\\')))
#else
        if ((dirend = strrchr(libpath, '/')))
#endif
        {
          const char* includes_relative = "/../include/oclgrind/";
          size_t length = dirend - libpath;
          pchdir = new char[length + strlen(includes_relative) + 1];
          strncpy(pchdir, libpath, length);
          strcpy(pchdir + length, includes_relative);
        }
      }
    }

    if (pchdir)
    {
      // Select precompiled header
      pch = new char[strlen(pchdir) + 24];
      sprintf(pch, "%s/opencl-c-%s-%d.pch", pchdir, clstd + 10,
              (sizeof(size_t) == 4 ? 32 : 64));

      // Check if precompiled header exists
      ifstream pchfile(pch);
      if (!pchfile.good())
      {
        buildLog << "WARNING: Unable to find precompiled header:\n"
                 << pch << "\n";
        delete[] pch;
        pch = NULL;
      }
      pchfile.close();
    }
    else
    {
      buildLog << "WARNING: Unable to determine precompiled header path\n";
    }
  }

  if (pch)
  {
    args.push_back("-isysroot");
    args.push_back(pchdir);

    args.push_back("-include-pch");
    args.push_back(pch);
    args.push_back("-fno-validate-pch");
  }
  else
  {
    // Fall back to embedded opencl-c.h
    args.push_back("-include");
    args.push_back(OPENCL_C_H_PATH);
  }

  // Append input file to arguments (remapped later)
  args.push_back(REMAP_INPUT);

  // Create diagnostics engine
  clang::DiagnosticOptions* diagOpts = new clang::DiagnosticOptions();
  llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> diagID(
    new clang::DiagnosticIDs());
  clang::TextDiagnosticPrinter* diagConsumer =
    new clang::TextDiagnosticPrinter(buildLog, diagOpts);
  clang::DiagnosticsEngine diags(diagID, diagOpts, diagConsumer);

  // Create compiler instance
  clang::CompilerInstance compiler;
  compiler.createDiagnostics(diagConsumer, false);

  // Create compiler invocation
  std::shared_ptr<clang::CompilerInvocation> invocation(
    new clang::CompilerInvocation);
  clang::CompilerInvocation::CreateFromArgs(*invocation, args,
                                            compiler.getDiagnostics());
  compiler.setInvocation(invocation);

  // Remap include files
  std::unique_ptr<llvm::MemoryBuffer> buffer;
  compiler.getHeaderSearchOpts().AddPath(REMAP_DIR, clang::frontend::Quoted,
                                         false, true);
  list<Header>::iterator itr;
  for (itr = headers.begin(); itr != headers.end(); itr++)
  {
    buffer = llvm::MemoryBuffer::getMemBuffer(itr->second->m_source, "", false);
    compiler.getPreprocessorOpts().addRemappedFile(REMAP_DIR + itr->first,
                                                   buffer.release());
  }

  // Remap opencl-c.h
  buffer = llvm::MemoryBuffer::getMemBuffer(OPENCL_C_H_DATA, "", false);
  compiler.getPreprocessorOpts().addRemappedFile(OPENCL_C_H_PATH,
                                                 buffer.release());

  // Remap input file
  buffer = llvm::MemoryBuffer::getMemBuffer(m_source, "", false);
  compiler.getPreprocessorOpts().addRemappedFile(REMAP_INPUT, buffer.release());

  // Compile
  clang::EmitLLVMOnlyAction action(m_context->getLLVMContext());
  if (compiler.ExecuteAction(action))
  {
    // Retrieve module
    m_module = action.takeModule();

    // Strip debug intrinsics if not in interactive mode
    if (!checkEnv("OCLGRIND_INTERACTIVE"))
    {
      stripDebugIntrinsics();
    }

    removeLValueLoads();

    allocateProgramScopeVars();

    m_buildStatus = CL_BUILD_SUCCESS;
  }
  else
  {
    m_buildStatus = CL_BUILD_ERROR;
  }

  // Dump temps if required
  if (checkEnv(ENV_DUMP_SPIR))
  {
    // Temporary directory
#if defined(_WIN32)
    const char* tmpdir = getenv("TEMP");
#else
    const char* tmpdir = "/tmp";
#endif

    // Construct unique output filenames
    size_t sz = snprintf(NULL, 0, "%s/oclgrind_%lX.XX", tmpdir, m_uid) + 1;
    char* tempCL = new char[sz];
    char* tempIR = new char[sz];
    char* tempBC = new char[sz];
    sprintf(tempCL, "%s/oclgrind_%lX.cl", tmpdir, m_uid);
    sprintf(tempIR, "%s/oclgrind_%lX.ll", tmpdir, m_uid);
    sprintf(tempBC, "%s/oclgrind_%lX.bc", tmpdir, m_uid);

    // Dump source
    ofstream cl;
    cl.open(tempCL);
    cl << m_source;
    cl.close();

    if (m_buildStatus == CL_BUILD_SUCCESS)
    {
      // Dump IR
      std::error_code err;
      llvm::raw_fd_ostream ir(tempIR, err, llvm::sys::fs::F_None);
      llvm::AssemblyAnnotationWriter asmWriter;
      m_module->print(ir, &asmWriter);
      ir.close();

      // Dump bitcode
      llvm::raw_fd_ostream bc(tempBC, err, llvm::sys::fs::F_None);
      llvm::WriteBitcodeToFile(*m_module, bc);
      bc.close();
    }

    delete[] tempCL;
    delete[] tempIR;
    delete[] tempBC;
  }

  delete[] tmpOptions;
  delete[] pchdir;
  delete[] pch;

  return m_buildStatus == CL_BUILD_SUCCESS;
}

void Program::clearInterpreterCache()
{
  InterpreterCacheMap::iterator itr;
  for (itr = m_interpreterCache.begin(); itr != m_interpreterCache.end(); itr++)
  {
    delete itr->second;
  }
  m_interpreterCache.clear();
}

Program* Program::createFromBitcode(const Context* context,
                                    const unsigned char* bitcode, size_t length)
{
  // Load bitcode from file
  llvm::StringRef data((const char*)bitcode, length);
  unique_ptr<llvm::MemoryBuffer> buffer =
    llvm::MemoryBuffer::getMemBuffer(data, "", false);
  if (!buffer)
  {
    return NULL;
  }

  std::string tag = std::to_string(rand());
#if defined(_WIN32)
  std::string tmpdir = getenv("TEMP");
#else
  std::string tmpdir = "/tmp";
#endif

  if (checkEnv(ENV_DUMP_SPIR)) {
    std::string tempBC = tmpdir + "/oclgrind_" + tag + ".bc";

    // Dump bitcode
    ofstream bc_dump;
    bc_dump.open(tempBC);
    bc_dump << std::string(buffer->getMemBufferRef().getBufferStart(), buffer->getMemBufferRef().getBufferSize());
    bc_dump.close();
  }

  // Parse bitcode into IR module
  llvm::Expected<unique_ptr<llvm::Module>> module =
    parseBitcodeFile(buffer->getMemBufferRef(), *context->getLLVMContext());
  if (!module)
  {
    return NULL;
  }

  if (checkEnv(ENV_DUMP_SPIR)) {
    // Dump LLVM IR
    std::string tempIR = tmpdir + "/oclgrind_" + tag + ".ll";
    std::error_code err;
    llvm::raw_fd_ostream ir(tempIR, err, llvm::sys::fs::F_None);
    llvm::AssemblyAnnotationWriter asmWriter;
    (*module)->print(ir, &asmWriter);
    ir.close();
  }

  return new Program(context, module.get().release());
}

Program* Program::createFromBitcodeFile(const Context* context,
                                        const string filename)
{
  // Load bitcode from file
  llvm::ErrorOr<unique_ptr<llvm::MemoryBuffer>> buffer =
    llvm::MemoryBuffer::getFile(filename);
  if (!buffer)
  {
    return NULL;
  }

  // Parse bitcode into IR module
  llvm::Expected<unique_ptr<llvm::Module>> module = parseBitcodeFile(
    buffer->get()->getMemBufferRef(), *context->getLLVMContext());
  if (!module)
  {
    return NULL;
  }

  return new Program(context, module.get().release());
}

Program* Program::createFromPrograms(const Context* context,
                                     list<const Program*> programs)
{
  llvm::Module* module =
    new llvm::Module("oclgrind_linked", *context->getLLVMContext());
  llvm::Linker linker(*module);

  // Link modules
  list<const Program*>::iterator itr;
  for (itr = programs.begin(); itr != programs.end(); itr++)
  {
    unique_ptr<llvm::Module> m = llvm::CloneModule(*(*itr)->m_module);
    if (linker.linkInModule(std::move(m)))
    {
      return NULL;
    }
  }

  return new Program(context, module);
}

Kernel* Program::createKernel(const string name)
{
  if (!m_module)
    return NULL;

  // Iterate over functions in module to find kernel
  llvm::Function* function = NULL;

  for (auto F = m_module->begin(); F != m_module->end(); F++)
  {
    if (F->getCallingConv() == llvm::CallingConv::SPIR_KERNEL &&
        F->getName() == name)
    {
      function = &*F;
      break;
    }
  }

  if (function == NULL)
  {
    return NULL;
  }

  try
  {
    // Create cache if none already
    InterpreterCacheMap::iterator itr = m_interpreterCache.find(function);
    if (itr == m_interpreterCache.end())
    {
      m_interpreterCache[function] = new InterpreterCache(function);
    }

    return new Kernel(this, function, m_module.get());
  }
  catch (FatalError& err)
  {
    cerr << endl
         << "OCLGRIND FATAL ERROR "
         << "(" << err.getFile() << ":" << err.getLine() << ")" << endl
         << err.what() << endl
         << "When creating kernel '" << name << "'" << endl;
    return NULL;
  }
}

void Program::deallocateProgramScopeVars()
{
  for (auto psv = m_programScopeVars.begin(); psv != m_programScopeVars.end();
       psv++)
  {
    m_context->getGlobalMemory()->deallocateBuffer(psv->second.getPointer());
    delete[] psv->second.data;
  }
  m_programScopeVars.clear();
  m_totalProgramScopeVarSize = 0;
}

void Program::getBinary(unsigned char* binary) const
{
  if (!m_module)
    return;

  std::string str;
  llvm::raw_string_ostream stream(str);
  llvm::WriteBitcodeToFile(*m_module, stream);
  stream.str();

  memcpy(binary, str.c_str(), str.length());
}

size_t Program::getBinarySize() const
{
  if (!m_module)
  {
    return 0;
  }

  std::string str;
  llvm::raw_string_ostream stream(str);
  llvm::WriteBitcodeToFile(*m_module, stream);
  stream.str();
  return str.length();
}

const string& Program::getBuildLog() const
{
  return m_buildLog;
}

const string& Program::getBuildOptions() const
{
  return m_buildOptions;
}

unsigned int Program::getBuildStatus() const
{
  return m_buildStatus;
}

const Context* Program::getContext() const
{
  return m_context;
}

unsigned long Program::generateUID() const
{
  srand(now());
  return rand();
}

const InterpreterCache*
Program::getInterpreterCache(const llvm::Function* kernel) const
{
  return m_interpreterCache[kernel];
}

list<string> Program::getKernelNames() const
{
  list<string> names;
  for (auto F = m_module->begin(); F != m_module->end(); F++)
  {
    if (F->getCallingConv() == llvm::CallingConv::SPIR_KERNEL)
    {
      names.push_back(F->getName().str());
    }
  }
  return names;
}

llvm::LLVMContext& Program::getLLVMContext() const
{
  return m_module->getContext();
}

unsigned int Program::getNumKernels() const
{
  assert(m_module);

  unsigned int num = 0;
  for (auto F = m_module->begin(); F != m_module->end(); F++)
  {
    if (F->getCallingConv() == llvm::CallingConv::SPIR_KERNEL)
    {
      num++;
    }
  }
  return num;
}

const TypedValue& Program::getProgramScopeVar(const llvm::Value* variable) const
{
  return m_programScopeVars.at(variable);
}

const string& Program::getSource() const
{
  return m_source;
}

const char* Program::getSourceLine(size_t lineNumber) const
{
  if (!lineNumber || (lineNumber - 1) >= m_sourceLines.size())
    return NULL;

  return m_sourceLines[lineNumber - 1].c_str();
}

size_t Program::getNumSourceLines() const
{
  return m_sourceLines.size();
}

size_t Program::getTotalProgramScopeVarSize() const
{
  return m_totalProgramScopeVarSize;
}

unsigned long Program::getUID() const
{
  return m_uid;
}

void Program::pruneDeadCode(llvm::Instruction* instruction)
{
  // Remove instructions that have no uses
  if (instruction->getNumUses() == 0)
  {
    // Get list of operands
    set<llvm::Value*> operands;
    {
      llvm::Instruction::op_iterator op;
      for (op = instruction->op_begin(); op != instruction->op_end(); op++)
      {
        operands.insert(*op);
      }
    }

    // Remove instruction
    instruction->eraseFromParent();

    // Prune operands
    set<llvm::Value*>::iterator op;
    for (op = operands.begin(); op != operands.end(); op++)
    {
      if (auto inst = llvm::dyn_cast<llvm::Instruction>(*op))
        pruneDeadCode(inst);
    }
  }
}

void Program::removeLValueLoads()
{
  // Get list of aggregate store instructions
  set<llvm::StoreInst*> aggStores;
  for (llvm::Module::iterator F = m_module->begin(); F != m_module->end(); F++)
  {
    llvm::Function* f = &*F;
    for (llvm::inst_iterator I = inst_begin(f), E = inst_end(f); I != E; I++)
    {
      if (auto store = llvm::dyn_cast<llvm::StoreInst>(&*I))
        aggStores.insert(store);
    }
  }

  // Replace aggregate modify-write sequences with direct scalar writes
  set<llvm::StoreInst*>::iterator itr;
  for (itr = aggStores.begin(); itr != aggStores.end(); itr++)
  {
    scalarizeAggregateStore(*itr);
  }
}

void Program::scalarizeAggregateStore(llvm::StoreInst* store)
{
  llvm::IntegerType* gepIndexType =
    (sizeof(size_t) == 8)
      ? llvm::Type::getInt64Ty(m_module.get()->getContext())
      : llvm::Type::getInt32Ty(m_module.get()->getContext());

  llvm::Value* storeValue = store->getValueOperand();
  llvm::Value* vectorPtr = store->getPointerOperand();

  if (auto insert = llvm::dyn_cast<llvm::InsertElementInst>(storeValue))
  {
    llvm::Value* vector = insert->getOperand(0);
    llvm::Value* value = insert->getOperand(1);
    llvm::Value* index = insert->getOperand(2);

    // Create GEP for scalar value
    llvm::GetElementPtrInst* scalarPtr = NULL;
    if (auto gep = llvm::dyn_cast<llvm::GetElementPtrInst>(vectorPtr))
    {
      // Create GEP from existing GEP
      std::vector<llvm::Value*> indices;
      for (auto idx = gep->idx_begin(); idx != gep->idx_end(); idx++)
      {
        indices.push_back(*idx);
      }
      indices.push_back(index);
      scalarPtr = llvm::GetElementPtrInst::Create(
        gep->getPointerOperandType()->getPointerElementType(),
        gep->getPointerOperand(), indices);
    }
    else
    {
      // Create GEP from non-GEP pointer
      std::vector<llvm::Value*> indices;
      indices.push_back(llvm::ConstantInt::getSigned(gepIndexType, 0));
      indices.push_back(index);
      scalarPtr = llvm::GetElementPtrInst::Create(
        vectorPtr->getType()->getPointerElementType(), vectorPtr, indices);
    }
    scalarPtr->setDebugLoc(store->getDebugLoc());
    scalarPtr->insertAfter(store);

    // Create direct scalar store
    llvm::StoreInst* scalarStore =
      new llvm::StoreInst(value, scalarPtr, store->isVolatile(),
                          llvm::Align(getTypeAlignment(value->getType())));
    scalarStore->setDebugLoc(store->getDebugLoc());
    scalarStore->insertAfter(scalarPtr);

    // Check if the input to the insertelement instruction came from something
    // other than a load to the same address as the store
    llvm::LoadInst* load = llvm::dyn_cast<llvm::LoadInst>(vector);
    if (!(load && load->getPointerOperand() == store->getPointerOperand()))
    {
      // Replace value in store with the input to the insertelement instruction
      llvm::StoreInst* _store = new llvm::StoreInst(
        vector, store->getPointerOperand(), store->isVolatile(),
        llvm::Align(store->getAlignment()));
      _store->setDebugLoc(store->getDebugLoc());
      _store->insertAfter(store);

      // Repeat process with new store
      if (_store)
        scalarizeAggregateStore(_store);
    }

    // Remove vector store and any dead code
    store->eraseFromParent();
    pruneDeadCode(insert);
  }
  else if (auto shuffle = llvm::dyn_cast<llvm::ShuffleVectorInst>(storeValue))
  {
    llvm::Value* v1 = shuffle->getOperand(0);
    llvm::Value* v2 = shuffle->getOperand(1);
    unsigned maskSize = shuffle->getShuffleMask().size();
    unsigned v1num =
      llvm::cast<llvm::FixedVectorType>(v1->getType())->getNumElements();

    // Check if shuffle sources came from a load with same address as the store
    llvm::LoadInst* load;
    bool v1SourceIsDest = false, v2SourceIsDest = false;
    if ((load = llvm::dyn_cast<llvm::LoadInst>(v1)) &&
        load->getPointerOperand() == vectorPtr)
      v1SourceIsDest = true;
    if ((load = llvm::dyn_cast<llvm::LoadInst>(v2)) &&
        load->getPointerOperand() == vectorPtr)
      v2SourceIsDest = true;

    // Get mask indices that don't correspond to the destination vector
    stack<unsigned> indices;
    for (unsigned i = 0; i < maskSize; i++)
    {
      int idx = shuffle->getMaskValue(i);

      // Skip undef indices
      if (idx == -1)
        continue;

      // Check if source is the store destination
      bool sourceIsDest =
        ((unsigned)idx < v1num ? v1SourceIsDest : v2SourceIsDest);

      // If destination is used in non-identity position, leave shuffle as is
      if (sourceIsDest && (unsigned)idx != i)
        return;

      // Add non-destination index
      if (!sourceIsDest)
        indices.push(i);
    }

    // Check if destination is actually used as a source in the mask
    if (indices.size() == maskSize)
    {
      // Check for any unused loads with the same address as the store
      // These would usually be caught by DCE, but if optimisations are
      // disabled we need to prune these manually
      list<llvm::LoadInst*> lvalueloads;
      for (auto user = vectorPtr->user_begin(); user != vectorPtr->user_end();
           user++)
      {
        if (auto load = llvm::dyn_cast<llvm::LoadInst>(*user))
        {
          if (load->getNumUses() == 0)
            lvalueloads.push_back(load);
        }
      }
      for (auto load = lvalueloads.begin(); load != lvalueloads.end(); load++)
      {
        (*load)->eraseFromParent();
      }

      return;
    }

    // Create a scalar store for each shuffle index
    while (!indices.empty())
    {
      unsigned index = indices.top();
      indices.pop();

      // Create GEP for scalar value
      llvm::GetElementPtrInst* scalarPtr = NULL;
      if (auto gep = llvm::dyn_cast<llvm::GetElementPtrInst>(vectorPtr))
      {
        // Create GEP from existing GEP
        std::vector<llvm::Value*> gepIndices;
        for (auto idx = gep->idx_begin(); idx != gep->idx_end(); idx++)
        {
          gepIndices.push_back(*idx);
        }
        gepIndices.push_back(llvm::ConstantInt::getSigned(gepIndexType, index));
        scalarPtr = llvm::GetElementPtrInst::Create(
          gep->getPointerOperandType()->getPointerElementType(),
          gep->getPointerOperand(), gepIndices);
      }
      else
      {
        // Create GEP from non-GEP pointer
        std::vector<llvm::Value*> gepIndices;
        gepIndices.push_back(llvm::ConstantInt::getSigned(gepIndexType, 0));
        gepIndices.push_back(llvm::ConstantInt::getSigned(gepIndexType, index));
        scalarPtr = llvm::GetElementPtrInst::Create(
          vectorPtr->getType()->getPointerElementType(), vectorPtr, gepIndices);
      }
      scalarPtr->setDebugLoc(store->getDebugLoc());
      scalarPtr->insertAfter(store);

      // Get source vector and index
      unsigned idx = shuffle->getMaskValue(index);
      llvm::Value* src = v1;
      if (idx >= v1num)
      {
        idx -= v1num;
        src = v2;
      }

      // Create direct scalar store
      if (auto cnst = llvm::dyn_cast<llvm::ConstantVector>(src))
      {
        // If source is a constant, extract scalar constant
        src = cnst->getAggregateElement(idx);

        llvm::StoreInst* scalarStore =
          new llvm::StoreInst(src, scalarPtr, store->isVolatile(),
                              llvm::Align(getTypeAlignment(src->getType())));
        scalarStore->setDebugLoc(store->getDebugLoc());
        scalarStore->insertAfter(scalarPtr);
      }
      else
      {
        // If extracting from a shuffle, trace back to last non-shuffle
        while (auto shfl = llvm::dyn_cast<llvm::ShuffleVectorInst>(src))
        {
          llvm::Value* v1 = shfl->getOperand(0);
          llvm::Value* v2 = shfl->getOperand(1);
          unsigned v1num =
            llvm::cast<llvm::FixedVectorType>(v1->getType())->getNumElements();

          // Get source vector and index
          idx = shfl->getMaskValue(idx);
          src = v1;
          if (idx >= v1num)
          {
            idx -= v1num;
            src = v2;
          }
        }

        llvm::ExtractElementInst* extract = llvm::ExtractElementInst::Create(
          src, llvm::ConstantInt::getSigned(gepIndexType, idx));
        extract->setDebugLoc(shuffle->getDebugLoc());
        extract->insertAfter(scalarPtr);

        llvm::StoreInst* scalarStore = new llvm::StoreInst(
          extract, scalarPtr, store->isVolatile(),
          llvm::Align(getTypeAlignment(extract->getType())));
        scalarStore->setDebugLoc(store->getDebugLoc());
        scalarStore->insertAfter(extract);
      }
    }

    // Prune old store and dead any code
    store->eraseFromParent();
    pruneDeadCode(shuffle);
  }
}

void Program::stripDebugIntrinsics()
{
  // Get list of llvm.dbg intrinsics
  set<llvm::Instruction*> intrinsics;
  for (llvm::Module::iterator F = m_module->begin(); F != m_module->end(); F++)
  {
    llvm::Function* f = &*F;
    for (llvm::inst_iterator I = inst_begin(f), E = inst_end(f); I != E; I++)
    {
      if (I->getOpcode() == llvm::Instruction::Call)
      {
        llvm::CallInst* call = (llvm::CallInst*)&*I;
        llvm::Function* function =
          (llvm::Function*)call->getCalledFunction()->stripPointerCasts();
        if (function->getName().startswith("llvm.dbg"))
        {
          intrinsics.insert(&*I);
        }
      }
    }
  }

  // Remove instructions
  set<llvm::Instruction*>::iterator itr;
  for (itr = intrinsics.begin(); itr != intrinsics.end(); itr++)
  {
    (*itr)->eraseFromParent();
  }
}
