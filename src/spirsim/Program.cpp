// Program.cpp (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"
#include <fstream>

#include "llvm/Assembly/AssemblyAnnotationWriter.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Linker.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/system_error.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Scalar.h"
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>

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

using namespace spirsim;
using namespace std;

Program::Program(llvm::Module *module)
  : m_module(module)
{
  m_action = NULL;
  m_buildLog = "";
  m_buildOptions = "";
  m_buildStatus = CL_BUILD_SUCCESS;
  m_uid = generateUID();
}

Program::Program(const string& source)
{
  m_source = source;
  m_module = NULL;
  m_action = NULL;
  m_buildLog = "";
  m_buildOptions = "";
  m_buildStatus = CL_BUILD_NONE;
  m_uid = 0;
}

Program::~Program()
{
  WorkItem::InterpreterCache::clear(m_uid);

  if (m_module)
  {
    delete m_module;
  }

  if (m_action)
  {
    delete m_action;
  }
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
    delete m_module;
    m_module = NULL;

    WorkItem::InterpreterCache::clear(m_uid);
  }

  // Assign a new UID to this program
  m_uid = generateUID();

  // Set compiler arguments
  vector<const char*> args;
  args.push_back("-D cl_khr_fp64");
  args.push_back("-cl-kernel-arg-info");
  args.push_back("-triple");
  args.push_back("spir64-unknown-unknown");
  args.push_back("-g");

  // Disable optimizations by default due to bugs in Khronos SPIR generator
  bool optimize = false;
  args.push_back("-O0");

  // Add OpenCL build options
  if (options)
  {
    char *_options = strdup(options);
    char *opt = strtok(_options, " ");
    while (opt)
    {
      // Ignore options that break PCH
      if (strcmp(opt, "-cl-fast-relaxed-math") != 0 &&
          strcmp(opt, "-cl-single-precision-constant") != 0)
      {
        args.push_back(opt);

        // Check for optimization flags
        if (strncmp(opt, "-O", 2) == 0)
        {
          if (strcmp(opt, "-O0") == 0)
          {
            optimize = false;
          }
          else
          {
            optimize = true;
          }
        }
      }
      opt = strtok(NULL, " ");
    }
  }

  // Select precompiled header
  const char *pch = NULL;
  if (optimize)
  {
    pch = INSTALL_ROOT"/include/spirsim/clc.pch";
  }
  else
  {
    pch = INSTALL_ROOT"/include/spirsim/clc.noopt.pch";
  }

  // Use precompiled header if it exists, otherwise fall back to embedded clc.h
  ifstream pchfile(pch);
  if (pchfile.good())
  {
    args.push_back("-include-pch");
    args.push_back(pch);
  }
  else
  {
    args.push_back("-include");
    args.push_back(CLC_H_PATH);
    buildLog << "WARNING: Unable to find precompiled header.\n";
  }
  pchfile.close();

  // Append input file to arguments (remapped later)
  args.push_back(REMAP_INPUT);

  // Create diagnostics engine
  clang::DiagnosticOptions *diagOpts = new clang::DiagnosticOptions();
  llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> diagID(
    new clang::DiagnosticIDs());
  clang::TextDiagnosticPrinter diagConsumer(buildLog, diagOpts);
  clang::DiagnosticsEngine diags(diagID, diagOpts, &diagConsumer, false);

  // Create compiler invocation
  llvm::OwningPtr<clang::CompilerInvocation> invocation(
    new clang::CompilerInvocation);
  clang::CompilerInvocation::CreateFromArgs(*invocation,
                                            &args[0], &args[0] + args.size(),
                                            diags);

  // Create compiler instance
  clang::CompilerInstance compiler;
  compiler.setInvocation(invocation.take());

  // Remap include files
  llvm::MemoryBuffer *buffer;
  compiler.getHeaderSearchOpts().AddPath(REMAP_DIR, clang::frontend::Quoted,
                                         false, false, false);
  list<Header>::iterator itr;
  for (itr = headers.begin(); itr != headers.end(); itr++)
  {
    buffer = llvm::MemoryBuffer::getMemBuffer(itr->second->m_source, "", false);
    compiler.getPreprocessorOpts().addRemappedFile(REMAP_DIR + itr->first,
                                                   buffer);
  }

  // Remap clc.h
  buffer = llvm::MemoryBuffer::getMemBuffer(CLC_H_DATA, "", false);
  compiler.getPreprocessorOpts().addRemappedFile(CLC_H_PATH, buffer);

  // Remap input file
  buffer = llvm::MemoryBuffer::getMemBuffer(m_source, "", false);
  compiler.getPreprocessorOpts().addRemappedFile(REMAP_INPUT, buffer);

  // Prepare diagnostics
  compiler.createDiagnostics(args.size(), &args[0], &diagConsumer, false);
  if (!compiler.hasDiagnostics())
  {
    m_buildStatus = CL_BUILD_ERROR;
    return false;
  }

  // Compile
  llvm::LLVMContext& context = llvm::getGlobalContext();
  clang::CodeGenAction *action = new clang::EmitLLVMOnlyAction(&context);

  if (compiler.ExecuteAction(*action))
  {
    // Retrieve module
    m_action = new llvm::OwningPtr<clang::CodeGenAction>(action);
    m_module = action->takeModule();
    m_buildStatus = CL_BUILD_SUCCESS;
  }
  else
  {
    m_buildStatus = CL_BUILD_ERROR;
  }

  // Dump temps if required
  const char *dumpSpir = getenv(ENV_DUMP_SPIR);
  if (dumpSpir && strcmp(dumpSpir, "1") == 0)
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
      string err;
      llvm::raw_fd_ostream ir(tempIR, err);
      llvm::AssemblyAnnotationWriter asmWriter;
      m_module->print(ir, &asmWriter);
      ir.close();

      // Dump bitcode
      llvm::raw_fd_ostream bc(tempBC, err);
      llvm::WriteBitcodeToFile(m_module, bc);
      bc.close();
    }

    delete[] tempCL;
    delete[] tempIR;
    delete[] tempBC;
  }

  return m_buildStatus == CL_BUILD_SUCCESS;
}

Program* Program::createFromBitcode(const unsigned char *bitcode,
                                    size_t length)
{
  // Load bitcode from file
  llvm::MemoryBuffer *buffer;
  llvm::StringRef data((const char*)bitcode, length);
  buffer = llvm::MemoryBuffer::getMemBuffer(data, "", false);
  if (!buffer)
  {
    return NULL;
  }

  // Parse bitcode into IR module
  llvm::LLVMContext& context = llvm::getGlobalContext();
  llvm::Module *module = ParseBitcodeFile(buffer, context);
  if (!module)
  {
    return NULL;
  }

  return new Program(module);
}

Program* Program::createFromBitcodeFile(const string filename)
{
  // Load bitcode from file
  llvm::OwningPtr<llvm::MemoryBuffer> buffer;
  if (llvm::MemoryBuffer::getFile(filename, buffer))
  {
    return NULL;
  }

  // Parse bitcode into IR module
  llvm::LLVMContext& context = llvm::getGlobalContext();
  llvm::Module *module = ParseBitcodeFile(buffer.get(), context);
  if (!module)
  {
    return NULL;
  }

  return new Program(module);
}

Program* Program::createFromPrograms(list<const Program*> programs)
{
  llvm::LLVMContext& context = llvm::getGlobalContext();
  llvm::Module *module = new llvm::Module("oclgrind_linked", context);
  llvm::Linker linker("oclgrind", module);

  // Link modules
  list<const Program*>::iterator itr;
  for (itr = programs.begin(); itr != programs.end(); itr++)
  {
    if (linker.LinkInModule(CloneModule((*itr)->m_module)))
    {
      return NULL;
    }
  }

  return new Program(linker.releaseModule());
}

Kernel* Program::createKernel(const string name)
{
  // Iterate over functions in module to find kernel
  llvm::Function *function = NULL;
  llvm::Module::iterator funcItr;
  for(funcItr = m_module->begin(); funcItr != m_module->end(); funcItr++)
  {
    // Check kernel name
    if (funcItr->getName().str() != name)
      continue;

    function = funcItr;
    break;
  }
  if (function == NULL)
  {
    return NULL;
  }

  // Assign identifiers to unnamed temporaries
  llvm::FunctionPass *instNamer = llvm::createInstructionNamerPass();
  instNamer->runOnFunction(*((llvm::Function*)function));
  delete instNamer;

  try
  {
    return new Kernel(*this, function, m_module);
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

unsigned char* Program::getBinary() const
{
  if (!m_module)
  {
    return NULL;
  }

  std::string str;
  llvm::raw_string_ostream stream(str);
  llvm::WriteBitcodeToFile(m_module, stream);
  stream.str();
  unsigned char *bitcode = new unsigned char[str.length()];
  memcpy(bitcode, str.c_str(), str.length());
  return bitcode;
}

size_t Program::getBinarySize() const
{
  if (!m_module)
  {
    return 0;
  }

  std::string str;
  llvm::raw_string_ostream stream(str);
  llvm::WriteBitcodeToFile(m_module, stream);
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

unsigned long Program::generateUID() const
{
  srand(now());
  return rand();
}

list<string> Program::getKernelNames() const
{
  list<string> names;

  // Iterate over functions in module to find kernels
  unsigned int num = 0;
  llvm::Function *function = NULL;
  llvm::Module::iterator funcItr;
  for(funcItr = m_module->begin(); funcItr != m_module->end(); funcItr++)
  {
    if (funcItr->getCallingConv() == llvm::CallingConv::SPIR_KERNEL)
    {
      names.push_back(funcItr->getName().str());
    }
  }

  return names;
}

unsigned int Program::getNumKernels() const
{
  assert(m_module != NULL);

  // Iterate over functions in module to find kernels
  unsigned int num = 0;
  llvm::Function *function = NULL;
  llvm::Module::iterator funcItr;
  for(funcItr = m_module->begin(); funcItr != m_module->end(); funcItr++)
  {
    if (funcItr->getCallingConv() == llvm::CallingConv::SPIR_KERNEL)
    {
      num++;
    }
  }

  return num;
}

const string& Program::getSource() const
{
  return m_source;
}

unsigned long Program::getUID() const
{
  return m_uid;
}
