#include "common.h"
#include <fstream>

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include "llvm/ADT/OwningPtr.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/system_error.h"
#include "llvm/Transforms/Scalar.h"
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>

#include <CL/cl.h>
#include "Kernel.h"
#include "Program.h"

#define TEMP_CL_FILE "/tmp/oclgrind_temp.cl"
#define TEMP_BC_FILE "/tmp/oclgrind_temp.bc"

using namespace spirsim;
using namespace std;

Program::Program(llvm::Module *module)
  : m_module(module)
{
  m_action = NULL;
  m_buildLog = "";
  m_buildOptions = "";
  m_buildStatus = CL_BUILD_NONE;
}

Program::Program(const string& source)
{
  m_source = source;
  m_module = NULL;
  m_action = NULL;
  m_buildLog = "";
  m_buildOptions = "";
  m_buildStatus = CL_BUILD_NONE;
}

Program::~Program()
{
  if (m_module)
  {
    delete m_module;
  }

  if (m_action)
  {
    delete m_action;
  }
}

bool Program::build(const char *options)
{
  m_buildStatus = CL_BUILD_IN_PROGRESS;
  m_buildOptions = options ? options : "";

  // Do nothing if program was created with binary
  if (m_source.empty() && m_module)
  {
    m_buildStatus = CL_BUILD_SUCCESS;
    return true;
  }

  // Dump source to temporary file
  // TODO: Build from memory?
  ofstream temp;
  temp.open(TEMP_CL_FILE);
  temp << m_source;
  temp.close();

  // Set compiler arguments
  vector<const char*> args;
  args.push_back("-g");
  args.push_back("-cl-kernel-arg-info");
  args.push_back("-triple");
  args.push_back("-spir64-unknown-unknown");

  // Add OpenCL build options
  if (options)
  {
    char *_options = strdup(options);
    char *opt = strtok(_options, " ");
    while (opt)
    {
      args.push_back(opt);
      opt = strtok(NULL, " ");
    }
  }

  args.push_back(TEMP_CL_FILE);

  // Create diagnostics engine
  m_buildLog = "";
  llvm::raw_string_ostream buildLog(m_buildLog);
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

  // Auto-include OpenCL header
  char *includes = strdup(getenv("C_INCLUDE_PATH"));
  char *path = strtok(includes, ":");
  while (path)
  {
    compiler.getHeaderSearchOpts().AddPath(path, clang::frontend::Quoted,
                                           false, false, false);
    path = strtok(NULL, ":");
  }
  compiler.getPreprocessorOpts().Includes.push_back("clc.h");
  free(includes);

  // Prepare diagnostics
  compiler.createDiagnostics(args.size(), &args[0], &diagConsumer, false);
  if (!compiler.hasDiagnostics())
  {
    m_buildStatus = CL_BUILD_ERROR;
    return false;
  }

  // Compile
  clang::CodeGenAction *action = new clang::EmitLLVMOnlyAction();
  if (!compiler.ExecuteAction(*action))
  {
    m_buildStatus = CL_BUILD_ERROR;
    return false;
  }

  // Retrieve module
  m_action = new llvm::OwningPtr<clang::CodeGenAction>(action);
  m_module = action->takeModule();

  // Dump bitcode for debugging
  string err;
  llvm::raw_fd_ostream output(TEMP_BC_FILE, err);
  llvm::WriteBitcodeToFile(m_module, output);
  output.close();

  m_buildStatus = CL_BUILD_SUCCESS;
  return true;
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
    cerr << "Invalid bitcode buffer" << endl;
    return NULL;
  }

  // Parse bitcode into IR module
  llvm::LLVMContext& context = llvm::getGlobalContext();
  llvm::Module *module = ParseBitcodeFile(buffer, context);
  if (!module)
  {
    cerr << "Failed to load SPIR bitcode." << endl;
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
    cerr << "Failed to open bitcode file '" << filename << "'" << endl;
    return NULL;
  }

  // Parse bitcode into IR module
  llvm::LLVMContext& context = llvm::getGlobalContext();
  llvm::Module *module = ParseBitcodeFile(buffer.get(), context);
  if (!module)
  {
    cerr << "Failed to load SPIR bitcode." << endl;
    return NULL;
  }

  return new Program(module);
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
    cerr << "Failed to locate kernel." << endl;
    return NULL;
  }

  // Assign identifiers to unnamed temporaries
  llvm::FunctionPass *instNamer = llvm::createInstructionNamerPass();
  instNamer->runOnFunction(*((llvm::Function*)function));
  delete instNamer;

  return new Kernel(function);
}

unsigned char* Program::getBinary() const
{
  if (!m_module)
  {
    return NULL;
  }

  // Get bitcode
  ifstream input(TEMP_BC_FILE, ifstream::binary);
  input.seekg(0, ios_base::end);
  size_t size = input.tellg();
  unsigned char *ret = new unsigned char[size];
  input.seekg(0, ios_base::beg);
  input.read((char*)ret, size);
  input.close();

  return ret;
}

size_t Program::getBinarySize() const
{
  if (!m_module)
  {
    return 0;
  }

  // Get bitcode size
  ifstream input(TEMP_BC_FILE, ifstream::binary);
  input.seekg(0, ios_base::end);
  size_t size = input.tellg();
  input.close();
  return size;
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
