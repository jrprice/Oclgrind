// Program.h (oclgrind)
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

namespace clang
{
  class CodeGenAction;
}

namespace llvm
{
  class Module;
  template<typename T> class OwningPtr;
}

namespace spirsim
{
  class Kernel;

  class Program
  {
  public:
    Program(const std::string& source);
    virtual ~Program();

    static Program* createFromBitcode(const unsigned char *bitcode,
                                      size_t length);
    static Program* createFromBitcodeFile(const std::string filename);

    bool build(const char *options);
    Kernel* createKernel(const std::string name);
    const std::string& getBuildLog() const;
    const std::string& getBuildOptions() const;
    unsigned char* getBinary() const;
    size_t getBinarySize() const;
    unsigned int getBuildStatus() const;
    std::list<std::string> getKernelNames() const;
    unsigned int getNumKernels() const;
    const std::string& getSource() const;

  private:
    Program(llvm::Module *module);

    llvm::OwningPtr<clang::CodeGenAction> *m_action;
    llvm::Module *m_module;
    std::string m_source;
    std::string m_buildLog;
    std::string m_buildOptions;
    unsigned int m_buildStatus;
  };
}
