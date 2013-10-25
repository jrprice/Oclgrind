// Program.cpp (oclgrind)
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
#include <fstream>
#include <sys/time.h>

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

#include <CL/cl.h>
#include "Kernel.h"
#include "Program.h"

#define TEMP_CL_FILE "/tmp/oclgrind_%lX.cl"
#define TEMP_BC_FILE "/tmp/oclgrind_%lX.bc"

#define REMAP_DIR "/remapped/"

using namespace spirsim;
using namespace std;

Program::Program(llvm::Module *module)
  : m_module(module)
{
  m_action = NULL;
  m_buildLog = "";
  m_buildOptions = "";
  m_buildStatus = CL_BUILD_SUCCESS;
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

bool Program::build(const char *options, list<Header> headers)
{
  m_buildStatus = CL_BUILD_IN_PROGRESS;
  m_buildOptions = options ? options : "";

  // Do nothing if program was created with binary
  if (m_source.empty() && m_module)
  {
    m_buildStatus = CL_BUILD_SUCCESS;
    return true;
  }

  // Generate unique tag for temporary files
  struct timeval tv;
  gettimeofday(&tv, NULL);
  srand(tv.tv_usec + tv.tv_sec*1e6);
  unsigned long tag = rand();

  // Construct unique filenames
  char *tempCL = new char[strlen(TEMP_CL_FILE) + 17];
  char *tempBC = new char[strlen(TEMP_BC_FILE) + 17];
  sprintf(tempCL, TEMP_CL_FILE, tag);
  sprintf(tempBC, TEMP_BC_FILE, tag);

  // Dump source to temporary file
  // TODO: Build from memory (remap file)?
  ofstream temp;
  temp.open(tempCL);
  temp << m_source;
  temp.close();

  // Set compiler arguments
  vector<const char*> args;
  args.push_back("-D cl_khr_fp64");
  args.push_back("-cl-kernel-arg-info");
  args.push_back("-triple");
  args.push_back("spir64-unknown-unknown");
  args.push_back("-g");
  args.push_back("-O0");

  // Auto-include OpenCL C header (precompiled if available)
  const char *pch = getenv("OCLGRIND_PCH");
  if (pch && strlen(pch) > 0)
  {
    args.push_back("-include-pch");
    args.push_back(pch);
  }
  else
  {
    args.push_back("-include");
    args.push_back("clc.h");
  }

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
      }
      opt = strtok(NULL, " ");
    }
  }

  args.push_back(tempCL);

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

  // Add header directories from C_INCLUDE_PATH
  char *includes = strdup(getenv("C_INCLUDE_PATH"));
  char *path = strtok(includes, ":");
  while (path)
  {
    compiler.getHeaderSearchOpts().AddPath(path, clang::frontend::Quoted,
                                           false, false, false);
    path = strtok(NULL, ":");
  }
  free(includes);

  // Remap include files
  compiler.getHeaderSearchOpts().AddPath(REMAP_DIR, clang::frontend::Quoted,
                                         false, false, false);
  list<Header>::iterator itr;
  for (itr = headers.begin(); itr != headers.end(); itr++)
  {
    llvm::MemoryBuffer *buffer =
      llvm::MemoryBuffer::getMemBuffer(itr->second->m_source, "", false);
    compiler.getPreprocessorOpts().addRemappedFile(REMAP_DIR + itr->first,
                                                   buffer);
  }

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
  if (!compiler.ExecuteAction(*action))
  {
    m_buildStatus = CL_BUILD_ERROR;
    return false;
  }

  // Retrieve module
  m_action = new llvm::OwningPtr<clang::CodeGenAction>(action);
  m_module = action->takeModule();

  const char *keepTempsEnv = getenv("OCLGRIND_KEEP_TEMPS");
  if (keepTempsEnv && strcmp(keepTempsEnv, "1") == 0)
  {
    // Dump bitcode for debugging
    string err;
    llvm::raw_fd_ostream output(tempBC, err);
    llvm::WriteBitcodeToFile(m_module, output);
    output.close();
  }
  else
  {
    remove(tempCL);
  }

  delete[] tempCL;
  delete[] tempBC;

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
    cerr << "Failed to locate kernel." << endl;
    return NULL;
  }

  // Assign identifiers to unnamed temporaries
  llvm::FunctionPass *instNamer = llvm::createInstructionNamerPass();
  instNamer->runOnFunction(*((llvm::Function*)function));
  delete instNamer;

  return new Kernel(*this, function, m_module);
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
