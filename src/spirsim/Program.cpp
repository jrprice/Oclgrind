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

#include "Kernel.h"
#include "Program.h"

using namespace spirsim;
using namespace std;

Program::Program(llvm::Module *module)
  : m_module(module)
{
  m_source = NULL;
  m_action = NULL;
}

Program::Program(const char *source)
{
  size_t length = strlen(source);
  m_source = new char[length + 1];
  strncpy(m_source, source, length);
  m_source[length] = '\0';

  m_module = NULL;
  m_action = NULL;
}

Program::~Program()
{
  if (m_module)
  {
    delete m_module;
  }

  if (m_source)
  {
    delete[] m_source;
  }

  if (m_action)
  {
    delete m_action;
  }
}

bool Program::build(const char *options)
{
  // Do nothing if program was created with binary
  if (!m_source)
  {
    return true;
  }

  // Dump source to temporary file
  // TODO: Build from memory
  ofstream temp;
  temp.open("/tmp/oclgrind_temp.cl");
  temp << "#include \"clc.h\"" << endl; // TODO: Do this properly
  temp << m_source;
  temp.close();

  // Set compiler arguments
  // TODO: Need to auto-include clc.h
  vector<const char*> args;
  args.push_back("-cc1");
  args.push_back("-g");
  args.push_back("-emit-llvm-bc");
  args.push_back("-cl-kernel-arg-info");
  args.push_back("-triple");
  args.push_back("-spir64-unknown-unknown");
  args.push_back("/tmp/oclgrind_temp.cl");

  // Create diagnostics engine
  // TODO: Build log needs to be saved as a string, not output
  clang::DiagnosticOptions *diagOpts = new clang::DiagnosticOptions();
  llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> diagID(new clang::DiagnosticIDs());
  clang::DiagnosticsEngine diags(diagID, diagOpts);

  // Create compiler invocation
  llvm::OwningPtr<clang::CompilerInvocation> invocation(new clang::CompilerInvocation);
  clang::CompilerInvocation::CreateFromArgs(*invocation, &args[0], &args[0] + args.size(), diags);

  // Create compiler instance
  clang::CompilerInstance compiler;
  compiler.setInvocation(invocation.take());

  // Prepare diagnostics
  compiler.createDiagnostics(args.size(), &args[0]);
  if (!compiler.hasDiagnostics())
    return false;

  // Compile
  clang::CodeGenAction *action = new clang::EmitLLVMOnlyAction();
  if (!compiler.ExecuteAction(*action))
    return false;

  // Grab the module built by the EmitLLVMOnlyAction
  m_action = new llvm::OwningPtr<clang::CodeGenAction>(action);
  m_module = action->takeModule();

  // Dump compiled bitcode for debug purposes
  string err;
  llvm::raw_fd_ostream output("/tmp/oclgrind_temp.bc", err);
  llvm::WriteBitcodeToFile(m_module, output);

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

Program* Program::createFromBitcodeFile(const std::string filename)
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

Kernel* Program::createKernel(const std::string name)
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
