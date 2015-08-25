// Program.cpp (Oclgrind)
// Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"
#include <fstream>

#if defined(_WIN32) && !defined(__MINGW32__)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"

#include "Kernel.h"
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
#define CLC_H_PATH REMAP_DIR"clc.h"
extern const char CLC_H_DATA[];

const char *EXTENSIONS[] =
{
  "cl_khr_fp64",
  "cl_khr_3d_image_writes",
  "cl_khr_global_int32_base_atomics",
  "cl_khr_global_int32_extended_atomics",
  "cl_khr_local_int32_base_atomics",
  "cl_khr_local_int32_extended_atomics",
  "cl_khr_byte_addressable_store",
};

using namespace oclgrind;
using namespace std;

Program::Program(const Context *context, llvm::Module *module)
  : m_module(module), m_context(context)
{
  m_buildLog = "";
  m_buildOptions = "";
  m_buildStatus = CL_BUILD_SUCCESS;
  m_uid = generateUID();
}

Program::Program(const Context *context, const string& source)
  : m_context(context)
{
  m_source = source;
  m_buildLog = "";
  m_buildOptions = "";
  m_buildStatus = CL_BUILD_NONE;
  m_uid = 0;

  // Split source into individual lines
  m_sourceLines.clear();
  if (!source.empty())
  {
    std::stringstream ss(source);
    std::string line;
    while(std::getline(ss, line, '\n'))
    {
      m_sourceLines.push_back(line);
    }
  }
}

Program::~Program()
{
  clearInterpreterCache();
}

bool Program::build(const char *options, list<Header> headers)
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
  args.push_back("-cl-std=CL1.2");
  args.push_back("-cl-kernel-arg-info");
  args.push_back("-fno-builtin");
  args.push_back("-g");
  args.push_back("-triple");
  if (sizeof(size_t) == 4)
    args.push_back("spir-unknown-unknown");
  else
    args.push_back("spir64-unknown-unknown");

  // Define extensions
  for (unsigned i = 0; i < sizeof(EXTENSIONS)/sizeof(const char*); i++)
  {
    args.push_back("-D");
    args.push_back(EXTENSIONS[i]);
  }

  // Disable Clang's optimizations.
  // We will manually run optimization passes and legalize the IR later.
  args.push_back("-O0");

  bool optimize = true;
  bool cl12     = true;

  // Add OpenCL build options
  const char *mainOptions = options;
  const char *extraOptions = getenv("OCLGRIND_BUILD_OPTIONS");
  if (!mainOptions)
    mainOptions = "";
  if (!extraOptions)
    extraOptions = "";
  char *tmpOptions = new char[strlen(mainOptions) + strlen(extraOptions) + 2];
  sprintf(tmpOptions, "%s %s", mainOptions, extraOptions);
  for (char *opt = strtok(tmpOptions, " "); opt; opt = strtok(NULL, " "))
  {
    // Ignore options that break PCH
    if (strcmp(opt, "-cl-fast-relaxed-math") != 0 &&
        strcmp(opt, "-cl-finite-math-only") != 0 &&
        strcmp(opt, "-cl-single-precision-constant") != 0)
    {
      // Check for optimization flags
      if (strcmp(opt, "-O0") == 0 || strcmp(opt, "-cl-opt-disable") == 0)
      {
        optimize = false;
        continue;
      }
      else if (strncmp(opt, "-O", 2) == 0)
      {
        optimize = true;
        continue;
      }

#if LLVM_VERSION >= 37
      // Clang no longer supports -cl-no-signed-zeros
      if (strcmp(opt, "-cl-no-signed-zeros") == 0)
        continue;
#endif

      // Check for -cl-std flag
      if (strncmp(opt, "-cl-std=", 8) == 0)
      {
        if (strcmp(opt+8, "CL1.2") != 0)
        {
          cl12 = false;
          args.push_back(opt);
        }
        continue;
      }

      args.push_back(opt);
    }
  }

  if (cl12)
  {
    args.push_back("-cl-std=CL1.2");
  }

  // Pre-compiled header
  char *pchdir = NULL;
  char *pch    = NULL;
  if (!checkEnv("OCLGRIND_DISABLE_PCH") && cl12)
  {
    const char *pchdirOverride = getenv("OCLGRIND_PCH_DIR");
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
      if (GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&Program::createFromBitcode, &dll) &&
          GetModuleFileName(dll, libpath, sizeof(libpath)))
      {
#else
      Dl_info dlinfo;
      if (dladdr((const void*)Program::createFromBitcode, &dlinfo))
      {
        const char *libpath = dlinfo.dli_fname;
#endif

        // Construct path to PCH directory
        const char *dirend;
#if defined(_WIN32) && !defined(__MINGW32__)
        if ((dirend = strrchr(libpath, '\\')))
#else
        if ((dirend = strrchr(libpath, '/')))
#endif
        {
          const char *includes_relative = "/../include/oclgrind/";
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
      pch = new char[strlen(pchdir) + 20];
      sprintf(pch, "%s/clc%d.pch", pchdir, (sizeof(size_t) == 4 ? 32 : 64));

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
  }
  else
  {
    // Fall back to embedded clc.h
    args.push_back("-include");
    args.push_back(CLC_H_PATH);
  }

  // Append input file to arguments (remapped later)
  args.push_back(REMAP_INPUT);

  // Create diagnostics engine
  clang::DiagnosticOptions *diagOpts = new clang::DiagnosticOptions();
  llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> diagID(
    new clang::DiagnosticIDs());
  clang::TextDiagnosticPrinter *diagConsumer =
    new clang::TextDiagnosticPrinter(buildLog, diagOpts);
  clang::DiagnosticsEngine diags(diagID, diagOpts, diagConsumer);

  // Create compiler instance
  clang::CompilerInstance compiler;
  compiler.createDiagnostics(diagConsumer, false);

  // Create compiler invocation
  clang::CompilerInvocation *invocation = new clang::CompilerInvocation;
  clang::CompilerInvocation::CreateFromArgs(*invocation,
                                            &args[0], &args[0] + args.size(),
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

  // Remap clc.h
  buffer = llvm::MemoryBuffer::getMemBuffer(CLC_H_DATA, "", false);
  compiler.getPreprocessorOpts().addRemappedFile(CLC_H_PATH, buffer.release());

  // Remap input file
  buffer = llvm::MemoryBuffer::getMemBuffer(m_source, "", false);
  compiler.getPreprocessorOpts().addRemappedFile(REMAP_INPUT, buffer.release());

  // Compile
  llvm::LLVMContext& context = llvm::getGlobalContext();
  clang::EmitLLVMOnlyAction action(&context);
  if (compiler.ExecuteAction(action))
  {
    // Retrieve module
    m_module = action.takeModule();

    // Strip debug intrinsics if not in interactive mode
    if (!checkEnv("OCLGRIND_INTERACTIVE"))
    {
      stripDebugIntrinsics();
    }

    // Run optimizations on module
    if (optimize)
    {
      // Initialize pass managers
      llvm::legacy::PassManager modulePasses;
      llvm::legacy::FunctionPassManager functionPasses(m_module.get());
#if LLVM_VERSION < 37
      modulePasses.add(new llvm::DataLayoutPass());
      functionPasses.add(new llvm::DataLayoutPass());
#endif

      // Populate pass managers with -Oz
      llvm::PassManagerBuilder builder;
      builder.OptLevel = 2;
      builder.SizeLevel = 2;
      builder.populateModulePassManager(modulePasses);
      builder.populateFunctionPassManager(functionPasses);

      // Run passes
      functionPasses.doInitialization();
      llvm::Module::iterator fItr;
      for (fItr = m_module->begin(); fItr != m_module->end(); fItr++)
        functionPasses.run(*fItr);
      functionPasses.doFinalization();
      modulePasses.run(*m_module);
    }

    removeLValueLoads();

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
    const char *tmpdir = getenv("TEMP");
#else
    const char *tmpdir = "/tmp";
#endif

    // Construct unique output filenames
    size_t sz = snprintf(NULL, 0, "%s/oclgrind_%lX.XX", tmpdir, m_uid) + 1;
    char *tempCL = new char[sz];
    char *tempIR = new char[sz];
    char *tempBC = new char[sz];
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
      llvm::WriteBitcodeToFile(m_module.get(), bc);
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

Program* Program::createFromBitcode(const Context *context,
                                    const unsigned char *bitcode,
                                    size_t length)
{
  // Load bitcode from file
  llvm::StringRef data((const char*)bitcode, length);
  unique_ptr<llvm::MemoryBuffer> buffer =
    llvm::MemoryBuffer::getMemBuffer(data, "", false);
  if (!buffer)
  {
    return NULL;
  }

  // Parse bitcode into IR module
#if LLVM_VERSION < 37
  llvm::ErrorOr<llvm::Module*> module =
#else
  llvm::ErrorOr<unique_ptr<llvm::Module>> module =
#endif
    parseBitcodeFile(buffer->getMemBufferRef(), llvm::getGlobalContext());
  if (!module)
  {
    return NULL;
  }

#if LLVM_VERSION < 37
  return new Program(context, module.get());
#else
  return new Program(context, module.get().release());
#endif
}

Program* Program::createFromBitcodeFile(const Context *context,
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
#if LLVM_VERSION < 37
  llvm::ErrorOr<llvm::Module*> module =
#else
  llvm::ErrorOr<unique_ptr<llvm::Module>> module =
#endif
    parseBitcodeFile(buffer->get()->getMemBufferRef(),
                     llvm::getGlobalContext());
  if (!module)
  {
    return NULL;
  }

#if LLVM_VERSION < 37
  return new Program(context, module.get());
#else
  return new Program(context, module.get().release());
#endif
}

Program* Program::createFromPrograms(const Context *context,
                                     list<const Program*> programs)
{
  llvm::Module *module = new llvm::Module("oclgrind_linked",
                                          llvm::getGlobalContext());
  llvm::Linker linker(module);

  // Link modules
  list<const Program*>::iterator itr;
  for (itr = programs.begin(); itr != programs.end(); itr++)
  {
    if (linker.linkInModule(CloneModule((*itr)->m_module.get())))
    {
      return NULL;
    }
  }

  return new Program(context, linker.getModule());
}

Kernel* Program::createKernel(const string name)
{
  if (!m_module)
    return NULL;

  // Iterate over functions in module to find kernel
  llvm::Function *function = NULL;

  // Query the SPIR kernel list
  llvm::NamedMDNode* tuple = m_module->getNamedMetadata("opencl.kernels");
  // No kernels in module
  if (!tuple)
    return NULL;

  for (unsigned i = 0; i < tuple->getNumOperands(); ++i)
  {
    llvm::MDNode* kernel = tuple->getOperand(i);

    llvm::ConstantAsMetadata *cam =
      llvm::dyn_cast<llvm::ConstantAsMetadata>(kernel->getOperand(0).get());
    if (!cam)
      continue;

    llvm::Function *kernelFunction =
      llvm::dyn_cast<llvm::Function>(cam->getValue());

    // Shouldn't really happen - this would mean an invalid Module as input
    if (!kernelFunction)
      continue;

    // Is this the kernel we want?
    if (kernelFunction->getName() == name)
    {
      function = kernelFunction;
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
    cerr << endl << "OCLGRIND FATAL ERROR "
         << "(" << err.getFile() << ":" << err.getLine() << ")"
         << endl << err.what()
         << endl << "When creating kernel '" << name << "'"
         << endl;
    return NULL;
  }
}

void Program::getBinary(unsigned char *binary) const
{
  if (!m_module)
    return;

  std::string str;
  llvm::raw_string_ostream stream(str);
  llvm::WriteBitcodeToFile(m_module.get(), stream);
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
  llvm::WriteBitcodeToFile(m_module.get(), stream);
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

const InterpreterCache* Program::getInterpreterCache(
  const llvm::Function *kernel) const
{
  return m_interpreterCache[kernel];
}

list<string> Program::getKernelNames() const
{
  list<string> names;

  // Query the SPIR kernel list
  llvm::NamedMDNode* tuple = m_module->getNamedMetadata("opencl.kernels");

  if (tuple)
  {
    for (unsigned i = 0; i < tuple->getNumOperands(); ++i)
    {
      llvm::MDNode* kernel = tuple->getOperand(i);

      llvm::ConstantAsMetadata *cam =
      llvm::dyn_cast<llvm::ConstantAsMetadata>(kernel->getOperand(0).get());
      if (!cam)
        continue;

      llvm::Function *kernelFunction =
        llvm::dyn_cast<llvm::Function>(cam->getValue());

      // Shouldn't really happen - this would mean an invalid Module as input
      if (!kernelFunction)
        continue;

      names.push_back(kernelFunction->getName());
    }
  }

  return names;
}

unsigned int Program::getNumKernels() const
{
  assert(m_module);

  // Extract kernels from metadata
  llvm::NamedMDNode* tuple = m_module->getNamedMetadata("opencl.kernels");

  // No kernels in module
  if (!tuple)
    return 0;

  return tuple->getNumOperands();
}

const string& Program::getSource() const
{
  return m_source;
}

const char* Program::getSourceLine(size_t lineNumber) const
{
  if (!lineNumber || (lineNumber-1) >= m_sourceLines.size())
    return NULL;

  return m_sourceLines[lineNumber-1].c_str();
}

size_t Program::getNumSourceLines() const
{
  return m_sourceLines.size();
}

unsigned long Program::getUID() const
{
  return m_uid;
}

void Program::pruneDeadCode(llvm::Instruction *instruction)
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
    for (llvm::inst_iterator I = inst_begin(F), E = inst_end(F); I != E; I++)
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

void Program::scalarizeAggregateStore(llvm::StoreInst *store)
{
  llvm::IntegerType *gepIndexType = (sizeof(size_t)==8) ?
      llvm::Type::getInt64Ty(m_module.get()->getContext()) :
      llvm::Type::getInt32Ty(m_module.get()->getContext());

  llvm::Value *storeValue = store->getValueOperand();
  llvm::Value *vectorPtr  = store->getPointerOperand();

  if (auto insert = llvm::dyn_cast<llvm::InsertElementInst>(storeValue))
  {
    llvm::Value *vector = insert->getOperand(0);
    llvm::Value *value  = insert->getOperand(1);
    llvm::Value *index  = insert->getOperand(2);

    // Create GEP for scalar value
    llvm::GetElementPtrInst *scalarPtr = NULL;
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
#if LLVM_VERSION > 36
        gep->getPointerOperandType()->getPointerElementType(),
#endif
        gep->getPointerOperand(), indices);
    }
    else
    {
      // Create GEP from non-GEP pointer
      std::vector<llvm::Value*> indices;
      indices.push_back(llvm::ConstantInt::getSigned(gepIndexType, 0));
      indices.push_back(index);
      scalarPtr = llvm::GetElementPtrInst::Create(
#if LLVM_VERSION > 36
        vectorPtr->getType()->getPointerElementType(),
#endif
        vectorPtr, indices);
    }
    scalarPtr->setDebugLoc(store->getDebugLoc());
    scalarPtr->insertAfter(store);

    // Create direct scalar store
    llvm::StoreInst *scalarStore = new llvm::StoreInst(
      value, scalarPtr, store->isVolatile(),
      getTypeAlignment(value->getType()));
    scalarStore->setDebugLoc(store->getDebugLoc());
    scalarStore->insertAfter(scalarPtr);

    // Check if the input to the insertelement instruction came from something
    // other than a load to the same address as the store
    llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(vector);
    if (!(load && load->getPointerOperand() == store->getPointerOperand()))
    {
      // Replace value in store with the input to the insertelement instruction
      llvm::StoreInst *_store = new llvm::StoreInst(
        vector, store->getPointerOperand(),
        store->isVolatile(), store->getAlignment());
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
    llvm::Value *v1      = shuffle->getOperand(0);
    llvm::Value *v2      = shuffle->getOperand(1);
    llvm::Constant *mask = shuffle->getMask();
    unsigned maskSize    = mask->getType()->getVectorNumElements();

    // Check if shuffle sources came from a load with same address as the store
    llvm::LoadInst *load;
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
        ((unsigned)idx < v1->getType()->getVectorNumElements() ?
          v1SourceIsDest : v2SourceIsDest);

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
      for (auto user  = vectorPtr->user_begin();
                user != vectorPtr->user_end() ;
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
      llvm::GetElementPtrInst *scalarPtr = NULL;
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
#if LLVM_VERSION > 36
          gep->getPointerOperandType()->getPointerElementType(),
#endif
          gep->getPointerOperand(), gepIndices);
      }
      else
      {
        // Create GEP from non-GEP pointer
        std::vector<llvm::Value*> gepIndices;
        gepIndices.push_back(llvm::ConstantInt::getSigned(gepIndexType, 0));
        gepIndices.push_back(llvm::ConstantInt::getSigned(gepIndexType, index));
        scalarPtr = llvm::GetElementPtrInst::Create(
#if LLVM_VERSION > 36
          vectorPtr->getType()->getPointerElementType(),
#endif
          vectorPtr, gepIndices);
      }
      scalarPtr->setDebugLoc(store->getDebugLoc());
      scalarPtr->insertAfter(store);

      // Get source vector and index
      unsigned idx   = shuffle->getMaskValue(index);
      unsigned v1num = v1->getType()->getVectorNumElements();
      llvm::Value *src = v1;
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

        llvm::StoreInst *scalarStore = new llvm::StoreInst(
          src, scalarPtr, store->isVolatile(),
          getTypeAlignment(src->getType()));
        scalarStore->setDebugLoc(store->getDebugLoc());
        scalarStore->insertAfter(scalarPtr);
      }
      else
      {
        // If extracting from a shuffle, trace back to last non-shuffle
        while (auto shfl = llvm::dyn_cast<llvm::ShuffleVectorInst>(src))
        {
          llvm::Value *v1 = shfl->getOperand(0);
          llvm::Value *v2 = shfl->getOperand(1);
          unsigned v1num  = v1->getType()->getVectorNumElements();

          // Get source vector and index
          idx = shfl->getMaskValue(idx);
          src = v1;
          if (idx >= v1num)
          {
            idx -= v1num;
            src = v2;
          }
        }

        llvm::ExtractElementInst *extract = llvm::ExtractElementInst::Create(
          src, llvm::ConstantInt::getSigned(gepIndexType, idx));
        extract->setDebugLoc(shuffle->getDebugLoc());
        extract->insertAfter(scalarPtr);

        llvm::StoreInst *scalarStore = new llvm::StoreInst(
          extract, scalarPtr, store->isVolatile(),
          getTypeAlignment(extract->getType()));
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
    for (llvm::inst_iterator I = inst_begin(F), E = inst_end(F); I != E; I++)
    {
      if (I->getOpcode() == llvm::Instruction::Call)
      {
        llvm::CallInst *call = (llvm::CallInst*)&*I;
        llvm::Function *function =
          (llvm::Function*)call->getCalledValue()->stripPointerCasts();
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
