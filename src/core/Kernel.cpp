// Kernel.cpp (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "config.h"
#include "common.h"

#include <sstream>

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_os_ostream.h"

#include "Kernel.h"
#include "Program.h"

using namespace oclgrind;
using namespace std;

Kernel::Kernel(const Program *program,
               const llvm::Function *function, const llvm::Module *module)
 : m_program(program), m_function(function), m_name(function->getName())
{
  // Set-up global variables
  llvm::Module::const_global_iterator itr;
  for (itr = module->global_begin(); itr != module->global_end(); itr++)
  {
    llvm::PointerType *type = itr->getType();
    switch (type->getPointerAddressSpace())
    {
    case AddrSpacePrivate:
    {
      // Get initializer data
      const llvm::Constant *init = itr->getInitializer();
      unsigned size = getTypeSize(init->getType());
      TypedValue value = {size, 1, new uint8_t[size]};
      getConstantData(value.data, init);
      m_values[&*itr] = value;

      break;
    }
    case AddrSpaceGlobal:
    case AddrSpaceConstant:
      m_values[&*itr] = program->getProgramScopeVar(&*itr).clone();
      break;
    case AddrSpaceLocal:
    {
      // Check that local memory variable belongs to this kernel
      if (!itr->getName().startswith(m_name))
        continue;

      // Get size of allocation
      TypedValue allocSize = {
        getTypeSize(itr->getInitializer()->getType()), 1, NULL
      };
      m_values[&*itr] = allocSize;

      break;
    }
    default:
      FATAL_ERROR("Unsupported GlobalVariable address space: %d",
                  type->getPointerAddressSpace());
    }
  }

  // Check whether the kernel requires uniform work-groups
  m_requiresUniformWorkGroups = false;
  for (auto &AS : m_function->getAttributes())
  {
    if (AS.hasAttribute("uniform-work-group-size"))
    {
      const llvm::Attribute &A = AS.getAttribute("uniform-work-group-size");
      if (A.getValueAsString().equals("true"))
        m_requiresUniformWorkGroups = true;
      break;
    }
  }

  // Get metadata node containing kernel arg info
  m_metadata = NULL;
  llvm::NamedMDNode *md = module->getNamedMetadata("opencl.kernels");
  if (md)
  {
    for (unsigned i = 0; i < md->getNumOperands(); i++)
    {
      llvm::MDNode *node = md->getOperand(i);

      llvm::ConstantAsMetadata *cam =
        llvm::dyn_cast<llvm::ConstantAsMetadata>(node->getOperand(0).get());
      if (!cam)
        continue;

      llvm::Function *function = ((llvm::Function*)cam->getValue());
      if (function->getName() == m_name)
      {
        m_metadata = node;
        break;
      }
    }
  }
}

Kernel::Kernel(const Kernel& kernel)
 : m_program(kernel.m_program)
{
  m_function = kernel.m_function;
  m_name = kernel.m_name;
  m_metadata = kernel.m_metadata;
  m_requiresUniformWorkGroups = kernel.m_requiresUniformWorkGroups;

  for (auto itr = kernel.m_values.begin(); itr != kernel.m_values.end(); itr++)
  {
    m_values[itr->first] = itr->second.clone();
  }
}

Kernel::~Kernel()
{
  TypedValueMap::iterator itr;
  for (itr = m_values.begin(); itr != m_values.end(); itr++)
  {
    delete[] itr->second.data;
  }
}

bool Kernel::allArgumentsSet() const
{
  llvm::Function::const_arg_iterator itr;
  for (itr = m_function->arg_begin(); itr != m_function->arg_end(); itr++)
  {
    if (!m_values.count(&*itr))
    {
      return false;
    }
  }
  return true;
}

const llvm::Argument* Kernel::getArgument(unsigned int index) const
{
  assert(index < getNumArguments());

  llvm::Function::const_arg_iterator argItr = m_function->arg_begin();
  for (unsigned i = 0; i < index; i++)
  {
    argItr++;
  }
  return &*argItr;
}

unsigned int Kernel::getArgumentAccessQualifier(unsigned int index) const
{
  assert(index < getNumArguments());

  // Get metadata
  const llvm::Metadata *md =
    getArgumentMetadata("kernel_arg_access_qual", index);
  if (!md)
  {
    return -1;
  }

  // Get qualifier string
  const llvm::MDString *str = llvm::dyn_cast<llvm::MDString>(md);
  string access = str->getString();
  if (access == "read_only")
  {
    return CL_KERNEL_ARG_ACCESS_READ_ONLY;
  }
  else if (access == "write_only")
  {
    return CL_KERNEL_ARG_ACCESS_WRITE_ONLY;
  }
  else if (access == "read_write")
  {
    return CL_KERNEL_ARG_ACCESS_READ_WRITE;
  }
  return CL_KERNEL_ARG_ACCESS_NONE;
}

unsigned int Kernel::getArgumentAddressQualifier(unsigned int index) const
{
  assert(index < getNumArguments());

  // Get metadata
  const llvm::Metadata *md =
    getArgumentMetadata("kernel_arg_addr_space", index);
  if (!md)
  {
    return -1;
  }

  switch(getMDAsConstInt(md)->getZExtValue())
  {
    case AddrSpacePrivate:
      return CL_KERNEL_ARG_ADDRESS_PRIVATE;
    case AddrSpaceGlobal:
      return CL_KERNEL_ARG_ADDRESS_GLOBAL;
    case AddrSpaceConstant:
      return CL_KERNEL_ARG_ADDRESS_CONSTANT;
    case AddrSpaceLocal:
      return CL_KERNEL_ARG_ADDRESS_LOCAL;
    default:
      return -1;
  }
}

const llvm::Metadata* Kernel::getArgumentMetadata(string name,
                                                  unsigned int index) const
{
  llvm::MDNode *node = m_function->getMetadata(name);
  if (node)
    return node->getOperand(index);

  if (!m_metadata)
  {
    return NULL;
  }

  // Loop over all metadata nodes for this kernel
  for (unsigned i = 0; i < m_metadata->getNumOperands(); i++)
  {
    const llvm::MDOperand& op = m_metadata->getOperand(i);
    if (llvm::MDNode *node = llvm::dyn_cast<llvm::MDNode>(op.get()))
    {
      // Check if node matches target name
      if (node->getNumOperands() > 0 &&
          ((llvm::MDString*)(node->getOperand(0).get()))->getString() == name)
      {
        return node->getOperand(index+1).get();
      }
    }
  }
  return NULL;
}

const llvm::StringRef Kernel::getArgumentName(unsigned int index) const
{
  return getArgument(index)->getName();
}

const llvm::StringRef Kernel::getArgumentTypeName(unsigned int index) const
{
  assert(index < getNumArguments());

  // Get metadata
  const llvm::Metadata *md = getArgumentMetadata("kernel_arg_type", index);
  if (!md)
  {
    return "";
  }

  llvm::StringRef name = llvm::dyn_cast<llvm::MDString>(md)->getString();
  size_t imgStart = name.find(" image");
  if (imgStart != llvm::StringRef::npos)
  {
    name = name.substr(imgStart+1);
  }
  return name;
}

unsigned int Kernel::getArgumentTypeQualifier(unsigned int index) const
{
  assert(index < getNumArguments());

  // Get metadata
  const llvm::Metadata *md = getArgumentMetadata("kernel_arg_type_qual", index);
  if (!md)
  {
    return -1;
  }

  // Ignore type qualifiers for non-pointer arguments
  const llvm::Argument *arg = getArgument(index);
  if (!arg->getType()->isPointerTy() || arg->hasByValAttr())
    return CL_KERNEL_ARG_TYPE_NONE;

  // Get qualifiers
  const llvm::MDString *str = llvm::dyn_cast<llvm::MDString>(md);
  istringstream iss(str->getString().str());

  unsigned int result = CL_KERNEL_ARG_TYPE_NONE;
  while (!iss.eof())
  {
    string tok;
    iss >> tok;
    if (tok == "const")
    {
      result |= CL_KERNEL_ARG_TYPE_CONST;
    }
    else if (tok == "restrict")
    {
      result |= CL_KERNEL_ARG_TYPE_RESTRICT;
    }
    else if (tok == "volatile")
    {
      result |= CL_KERNEL_ARG_TYPE_VOLATILE;
    }
  }

  return result;
}

size_t Kernel::getArgumentSize(unsigned int index) const
{
  const llvm::Argument *argument = getArgument(index);
  const llvm::Type *type = argument->getType();

  // Check if pointer argument
  if (type->isPointerTy() && argument->hasByValAttr())
  {
    return getTypeSize(type->getPointerElementType());
  }

  return getTypeSize(type);
}

string Kernel::getAttributes() const
{
  ostringstream attributes("");
  llvm::MDNode *node;

  node = m_function->getMetadata("reqd_work_group_size");
  if (node)
  {
    attributes << "reqd_work_group_size("
               <<        getMDAsConstInt(node->getOperand(0))->getZExtValue()
               << "," << getMDAsConstInt(node->getOperand(1))->getZExtValue()
               << "," << getMDAsConstInt(node->getOperand(2))->getZExtValue()
               << ") ";
  }

  node = m_function->getMetadata("work_group_size_hint");
  if (node)
  {
    attributes << "work_group_size_hint("
               <<        getMDAsConstInt(node->getOperand(0))->getZExtValue()
               << "," << getMDAsConstInt(node->getOperand(1))->getZExtValue()
               << "," << getMDAsConstInt(node->getOperand(2))->getZExtValue()
               << ") ";
  }

  node = m_function->getMetadata("vec_type_hint");
  if (node)
  {
    // Get type hint
    size_t n = 1;
    llvm::Metadata *md = node->getOperand(0).get();
    llvm::ValueAsMetadata *vam = llvm::dyn_cast<llvm::ValueAsMetadata>(md);
    const llvm::Type *type = vam->getType();
    if (type->isVectorTy())
    {
      n = type->getVectorNumElements();
      type = type->getVectorElementType();
    }

    // Generate attribute string
    attributes << "vec_type_hint(" << flush;
    llvm::raw_os_ostream out(attributes);
    type->print(out);
    out.flush();
    attributes << n << ") ";
  }

  return attributes.str();
}

const llvm::Function* Kernel::getFunction() const
{
  return m_function;
}

size_t Kernel::getLocalMemorySize() const
{
  size_t sz = 0;
  for (auto value = m_values.begin(); value != m_values.end(); value++)
  {
    const llvm::Type *type = value->first->getType();
    if (type->isPointerTy() && type->getPointerAddressSpace() == AddrSpaceLocal)
    {
      sz += value->second.size;
    }
  }
  return sz;
}

const std::string& Kernel::getName() const
{
  return m_name;
}

unsigned int Kernel::getNumArguments() const
{
  return m_function->arg_size();
}

const Program* Kernel::getProgram() const
{
  return m_program;
}

void Kernel::getRequiredWorkGroupSize(size_t reqdWorkGroupSize[3]) const
{
  memset(reqdWorkGroupSize, 0, 3*sizeof(size_t));
  for (int j = 0; j < 3; j++)
  {
    const llvm::Metadata *md = getArgumentMetadata("reqd_work_group_size", j);
    if (md)
      reqdWorkGroupSize[j] = getMDAsConstInt(md)->getZExtValue();
  }
}

bool Kernel::requiresUniformWorkGroups() const
{
  return m_requiresUniformWorkGroups;
}

void Kernel::setArgument(unsigned int index, TypedValue value)
{
  assert(index < m_function->arg_size());

  const llvm::Value *argument = getArgument(index);

  // Deallocate existing argument
  if (m_values.count(argument))
  {
    delete[] m_values[argument].data;
  }

  if (getArgumentTypeName(index).str() == "sampler_t")
  {
    // Get an llvm::ConstantInt that represents the sampler value
    llvm::Type *i32 = llvm::Type::getInt32Ty(m_program->getLLVMContext());
    llvm::Constant *samplerValue = llvm::ConstantInt::get(i32, value.getSInt());

    // A sampler argument is a pointer to the llvm::ConstantInt value
    TypedValue sampler;
    sampler.size = sizeof(size_t);
    sampler.num = 1;
    sampler.data = new unsigned char[sizeof(size_t)];
    sampler.setPointer((size_t)samplerValue);

    m_values[argument] = sampler;
  }
  else
  {
    m_values[argument] = value.clone();
  }
}

TypedValueMap::const_iterator Kernel::values_begin() const
{
  return m_values.begin();
}

TypedValueMap::const_iterator Kernel::values_end() const
{
  return m_values.end();
}
