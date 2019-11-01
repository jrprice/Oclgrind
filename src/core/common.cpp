// common.cpp (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "config.h"
#include "common.h"

#if defined(_WIN32) && !defined(__MINGW32__)
#include <time.h>
#else
#include <sys/time.h>
#endif

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/raw_os_ostream.h"

using namespace oclgrind;
using namespace std;

namespace oclgrind
{
  Size3::Size3()
  {
    x = y = z = 0;
  }

  Size3::Size3(size_t _x, size_t _y, size_t _z)
  {
    x = _x;
    y = _y;
    z = _z;
  }

  Size3::Size3(size_t linear, Size3 dimensions)
  {
    x = linear % dimensions.x;
    y = (linear / dimensions.x) % dimensions.y;
    z = (linear / (dimensions.x * dimensions.y));
  }

  size_t& Size3::operator[](unsigned i)
  {
    switch (i)
    {
    case 0:
      return x;
    case 1:
      return y;
    case 2:
      return z;
    default:
      assert(false && "Size3 index out of range");
      abort();
    }
  }

  const size_t& Size3::operator[](unsigned i) const
  {
    switch (i)
    {
    case 0:
      return x;
    case 1:
      return y;
    case 2:
      return z;
    default:
      assert(false && "Size3 index out of range");
      abort();
    }
  }

  bool Size3::operator==(const Size3& rhs) const
  {
    return x == rhs.x && y == rhs.y && z == rhs.z;
  }

  bool Size3::operator!=(const Size3& rhs) const
  {
    return x != rhs.x || y != rhs.y || z != rhs.z;
  }

  ostream& operator<<(ostream& stream, const Size3& size)
  {
    stream << dec    << "("
           << size.x << ","
           << size.y << ","
           << size.z << ")";
    return stream;
  }

  double TypedValue::getFloat(unsigned index) const
  {
    switch (size)
    {
    case 4:
      return ((float*)data)[index];
    case 8:
      return ((double*)data)[index];
    default:
      FATAL_ERROR("Unsupported float size: %u bytes", size);
    }
  }

  size_t TypedValue::getPointer(unsigned index) const
  {
    if (size != sizeof(size_t))
    {
      FATAL_ERROR("Unsupported pointer size: %u bytes", size);
    }
    return ((size_t*)data)[index];
  }

  int64_t TypedValue::getSInt(unsigned index) const
  {
    switch (size)
    {
    case 1:
      return ((int8_t*)data)[index];
    case 2:
      return ((int16_t*)data)[index];
    case 4:
      return ((int32_t*)data)[index];
    case 8:
      return ((int64_t*)data)[index];
    default:
      FATAL_ERROR("Unsupported signed int size: %u bytes", size);
    }
  }

  uint64_t TypedValue::getUInt(unsigned index) const
  {
    switch (size)
    {
    case 1:
      return ((uint8_t*)data)[index];
    case 2:
      return ((uint16_t*)data)[index];
    case 4:
      return ((uint32_t*)data)[index];
    case 8:
      return ((uint64_t*)data)[index];
    default:
      FATAL_ERROR("Unsupported unsigned int size: %u bytes", size);
    }
  }

  void TypedValue::setFloat(double value, unsigned index)
  {
    switch (size)
    {
    case 4:
      ((float*)data)[index] = value;
      break;
    case 8:
      ((double*)data)[index] = value;
      break;
    default:
      FATAL_ERROR("Unsupported float size: %u bytes", size);
    }
  }

  void TypedValue::setPointer(size_t value, unsigned index)
  {
    if (size != sizeof(size_t))
    {
      FATAL_ERROR("Unsupported pointer size: %u bytes", size);
    }
    ((size_t*)data)[index] = value;
  }

  void TypedValue::setSInt(int64_t value, unsigned index)
  {
    switch (size)
    {
    case 1:
      ((int8_t*)data)[index] = value;
      break;
    case 2:
      ((int16_t*)data)[index] = value;
      break;
    case 4:
      ((int32_t*)data)[index] = value;
      break;
    case 8:
      ((int64_t*)data)[index] = value;
      break;
    default:
      FATAL_ERROR("Unsupported signed int size: %u bytes", size);
    }
  }

  void TypedValue::setUInt(uint64_t value, unsigned index)
  {
    switch (size)
    {
    case 1:
      ((uint8_t*)data)[index] = value;
      break;
    case 2:
      ((uint16_t*)data)[index] = value;
      break;
    case 4:
      ((uint32_t*)data)[index] = value;
      break;
    case 8:
      ((uint64_t*)data)[index] = value;
      break;
    default:
      FATAL_ERROR("Unsupported unsigned int size: %u bytes", size);
    }
  }

  ostream& operator<<(ostream& stream, const TypedValue& tv)
  {
    if(tv.data)
    {
      if(tv.num > 1)
      {
        stream << "(";
      }

      for(unsigned n = 0; n < tv.num; ++n)
      {
        for(int i = tv.size - 1; i >= 0; --i)
        {
          stream << hex << uppercase << setw(2) << setfill('0')
                 << (int)*(tv.data + tv.size * n + i);
        }

        if(n != tv.num - 1)
        {
          stream << ",";
        }
      }

      if(tv.num > 1)
      {
        stream << ")";
      }
    }
    else
    {
      stream << "NULL";
    }

    return stream;
  }

  bool TypedValue::operator==(const TypedValue& rhs) const
  {
    return (size == rhs.size) && (num == rhs.num) &&
           (memcmp(data, rhs.data, size*num) == 0);
  }

  bool TypedValue::operator!=(const TypedValue& rhs) const
  {
    return (size != rhs.size) || (num != rhs.num) ||
           (memcmp(data, rhs.data, size*num) != 0);
  }

  TypedValue TypedValue::clone() const
  {
    TypedValue result;
    result.size = size;
    result.num  = num;
    if (data)
    {
      result.data = new unsigned char[size*num];
      memcpy(result.data, data, size*num);
    }
    else
    {
      result.data = NULL;
    }
    return result;
  }

  bool checkEnv(const char *var)
  {
    const char *value = getenv(var);
    return (value && !strcmp(value, "1"));
  }

  unsigned getEnvInt(const char *var, int def, bool allowZero)
  {
    const char *value = getenv(var);
    if (!value)
      return def;

    char *next;
    uint64_t result = strtoul(value, &next, 10);
    if (strlen(next) || result == ULONG_MAX || (!allowZero && !result))
    {
      cerr << endl << "Oclgrind: Invalid value for " << var << endl;
      abort();
    }

    return result;
  }

  void dumpInstruction(ostream& out, const llvm::Instruction *instruction)
  {
    llvm::raw_os_ostream stream(out);
    instruction->print(stream);
  }

  const char* getAddressSpaceName(unsigned addrSpace)
  {
    switch (addrSpace)
    {
    case AddrSpacePrivate:
      return "private";
    case AddrSpaceGlobal:
      return "global";
    case AddrSpaceConstant:
      return "constant";
    case AddrSpaceLocal:
      return "local";
    default:
      return "(unknown)";
    }
  }

  void getConstantData(unsigned char *data, const llvm::Constant *constant)
  {
    if (constant->getValueID() == llvm::Value::UndefValueVal)
    {
      return;
    }

    const llvm::Type *type = constant->getType();
    unsigned size = getTypeSize(type);
    switch (type->getTypeID())
    {
    case llvm::Type::IntegerTyID:
    {
      uint64_t ui = ((llvm::ConstantInt*)constant)->getZExtValue();
      switch (size)
      {
      case 1:
        *((uint8_t*)data) = ui;
        break;
      case 2:
        *((uint16_t*)data) = ui;
        break;
      case 4:
        *((uint32_t*)data) = ui;
        break;
      case 8:
        *((uint64_t*)data) = ui;
        break;
      default:
        FATAL_ERROR("Unsupported constant int size: %u bytes", size);
      }
      break;
    }
    case llvm::Type::FloatTyID:
    {
      *(float*)data =
        ((llvm::ConstantFP*)constant)->getValueAPF().convertToFloat();
      break;
    }
    case llvm::Type::DoubleTyID:
    {
      *(double*)data =
        ((llvm::ConstantFP*)constant)->getValueAPF().convertToDouble();
      break;
    }
    case llvm::Type::VectorTyID:
    {
      unsigned num = type->getVectorNumElements();
      const llvm::Type *elemType = type->getVectorElementType();
      unsigned elemSize = getTypeSize(elemType);
      for (unsigned i = 0; i < num; i++)
      {
        getConstantData(data + i*elemSize, constant->getAggregateElement(i));
      }
      break;
    }
    case llvm::Type::ArrayTyID:
    {
      unsigned num = type->getArrayNumElements();
      const llvm::Type *elemType = type->getArrayElementType();
      unsigned elemSize = getTypeSize(elemType);
      for (unsigned i = 0; i < num; i++)
      {
        getConstantData(data + i*elemSize, constant->getAggregateElement(i));
      }
      break;
    }
    case llvm::Type::PointerTyID:
    {
      if (constant->getValueID() != llvm::Value::ConstantPointerNullVal)
      {
        FATAL_ERROR("Unsupported constant pointer value: %d",
                    constant->getValueID());
      }
      *(size_t*)data = 0;
      break;
    }
    case llvm::Type::StructTyID:
    {
      unsigned num = type->getStructNumElements();
      for (unsigned i = 0; i < num; i++)
      {
        unsigned offset =
          getStructMemberOffset((const llvm::StructType*)type, i);
        getConstantData(data + offset, constant->getAggregateElement(i));
      }
      break;
    }
    default:
      FATAL_ERROR("Unsupported constant type: %d", type->getTypeID());
    }
  }

  llvm::Instruction* getConstExprAsInstruction(const llvm::ConstantExpr *expr)
  {
    // Get operands
    vector<llvm::Value*> valueOperands(expr->op_begin(), expr->op_end());
    llvm::ArrayRef<llvm::Value*> operands(valueOperands);

    // Create instruction
    unsigned opcode = expr->getOpcode();
    switch (opcode)
    {
    case llvm::Instruction::Trunc:
    case llvm::Instruction::ZExt:
    case llvm::Instruction::SExt:
    case llvm::Instruction::FPTrunc:
    case llvm::Instruction::FPExt:
    case llvm::Instruction::UIToFP:
    case llvm::Instruction::SIToFP:
    case llvm::Instruction::FPToUI:
    case llvm::Instruction::FPToSI:
    case llvm::Instruction::PtrToInt:
    case llvm::Instruction::IntToPtr:
    case llvm::Instruction::BitCast:
      return llvm::CastInst::Create((llvm::Instruction::CastOps)opcode,
                                    operands[0], expr->getType());
    case llvm::Instruction::Select:
      return llvm::SelectInst::Create(operands[0], operands[1], operands[2]);
    case llvm::Instruction::InsertElement:
      return llvm::InsertElementInst::Create(operands[0], operands[1],
                                             operands[2]);
    case llvm::Instruction::ExtractElement:
      return llvm::ExtractElementInst::Create(operands[0], operands[1]);
    case llvm::Instruction::InsertValue:
      return llvm::InsertValueInst::Create(operands[0], operands[1],
                                           expr->getIndices());
    case llvm::Instruction::ExtractValue:
      return llvm::ExtractValueInst::Create(operands[0], expr->getIndices());
    case llvm::Instruction::ShuffleVector:
      return new llvm::ShuffleVectorInst(operands[0], operands[1],
                                         operands[2]);
    case llvm::Instruction::GetElementPtr:
      if (((const llvm::GEPOperator*)expr)->isInBounds())
      {
        return llvm::GetElementPtrInst::CreateInBounds(operands[0],
                                                       operands.slice(1));
      }
      else
      {
        return llvm::GetElementPtrInst::Create(nullptr,
                                               operands[0], operands.slice(1));
      }
    case llvm::Instruction::ICmp:
    case llvm::Instruction::FCmp:
      return llvm::CmpInst::Create(
        (llvm::Instruction::OtherOps)opcode,
        (llvm::CmpInst::Predicate)expr->getPredicate(),
        operands[0], operands[1]);
    case llvm::Instruction::AddrSpaceCast:
      FATAL_ERROR("Unsupported constant expression: addrspacecast");
    default:
      assert(expr->getNumOperands() == 2 && "Must be binary operator?");

      llvm::BinaryOperator *binaryOp =
        llvm::BinaryOperator::Create((llvm::Instruction::BinaryOps)opcode,
                                     operands[0], operands[1]);

      // Check for overflowing operator
      if (opcode == llvm::Instruction::Add ||
          opcode == llvm::Instruction::Mul ||
          opcode == llvm::Instruction::Shl ||
          opcode == llvm::Instruction::Sub)
      {
        binaryOp->setHasNoUnsignedWrap(
          expr->getRawSubclassOptionalData() &
          llvm::OverflowingBinaryOperator::NoUnsignedWrap);
        binaryOp->setHasNoSignedWrap(
          expr->getRawSubclassOptionalData() &
          llvm::OverflowingBinaryOperator::NoSignedWrap);
      }

      // Check for possibly exact operator
      if (opcode == llvm::Instruction::AShr ||
          opcode == llvm::Instruction::LShr ||
          opcode == llvm::Instruction::SDiv ||
          opcode == llvm::Instruction::UDiv)
      {
        binaryOp->setIsExact(expr->getRawSubclassOptionalData() &
                             llvm::PossiblyExactOperator::IsExact);
      }

      return binaryOp;
    }
  }

  const llvm::ConstantInt* getMDAsConstInt(const llvm::Metadata *md)
  {
    const llvm::ConstantAsMetadata *cam =
      llvm::dyn_cast<llvm::ConstantAsMetadata>(md);
    if (!cam)
      return NULL;
    return llvm::dyn_cast<llvm::ConstantInt>(cam->getValue());
  }

  unsigned getStructMemberOffset(const llvm::StructType *type, unsigned index)
  {
    bool packed = ((llvm::StructType*)type)->isPacked();

    unsigned offset = 0;
    for (unsigned i = 0; i <= index; i++)
    {
      // Get member size and alignment
      const llvm::Type *elemType = type->getStructElementType(i);
      unsigned size = getTypeSize(elemType);
      unsigned align = getTypeAlignment(elemType);

      // Add padding if necessary
      if (!packed && offset % align)
      {
        offset += (align - (offset%align));
      }

      if (i == index)
      {
        return offset;
      }
      offset += size;
    }

    // Unreachable
    abort();
  }

  unsigned getTypeSize(const llvm::Type *type)
  {
    if (type->isArrayTy())
    {
      unsigned num = type->getArrayNumElements();
      unsigned sz = getTypeSize(type->getArrayElementType());
      return num*sz;
    }
    else if (type->isStructTy())
    {
      bool packed = ((llvm::StructType*)type)->isPacked();

      unsigned size = 0;
      unsigned alignment = 1;
      for (unsigned i = 0; i < type->getStructNumElements(); i++)
      {
        // Get member size and alignment
        const llvm::Type *elemType = type->getStructElementType(i);
        unsigned sz    = getTypeSize(elemType);
        unsigned align = getTypeAlignment(elemType);

        // Add padding if necessary
        if (!packed && size % align)
        {
          size += (align - (size%align));
        }

        size += sz;

        alignment = max(alignment, align);
      }

      // Alignment of struct should match member with largest alignment
      if (!packed && size % alignment)
      {
        size += (alignment - (size%alignment));
      }

      return size;
    }
    else if (type->isVectorTy())
    {
      unsigned num = type->getVectorNumElements();
      unsigned sz = getTypeSize(type->getVectorElementType());
      if (num == 3) num = 4; // Hack for 3-element vectors
      return num*sz;
    }
    else if (type->isPointerTy())
    {
      return sizeof(size_t);
    }
    else
    {
      // Round up for types that have a bit size not multiple of 8
      // like "bool".
      return (type->getScalarSizeInBits() + 7) >> 3;
    }
  }

  /// Returns the byte alignment of this type
  unsigned getTypeAlignment(const llvm::Type* type)
  {
    using namespace llvm;
    // Array types are aligned to their element type
    if (const ArrayType* psAT = dyn_cast<ArrayType>(type))
    {
      return getTypeAlignment(psAT->getElementType());
    }

    // Struct alignment is the size of its largest contained type
    if (const StructType* structT = dyn_cast<StructType>(type))
    {
      if (structT->isPacked())
        return 1;
      StructType* nonConstTy = const_cast<StructType*>(structT);
      unsigned uAlign = 0, uMaxAlign = 1;
      unsigned uCount = structT->getNumElements();
      for (unsigned i = 0; i < uCount; i++)
      {
          const Type* psElemType = nonConstTy->getTypeAtIndex(i);
          uAlign = getTypeAlignment(psElemType);

          if (uAlign > uMaxAlign)
            uMaxAlign = uAlign;
      }

      return uMaxAlign;
    }

    return getTypeSize(type);
  }

  pair<unsigned,unsigned> getValueSize(const llvm::Value *value)
  {
    unsigned bits, numElements;
    const llvm::Type *type = value->getType();

    if (type->isVectorTy())
    {
      bits = type->getVectorElementType()->getPrimitiveSizeInBits();
      numElements = type->getVectorNumElements();
    }
    else if (type->isAggregateType())
    {
      bits = getTypeSize(type)<<3;
      numElements = 1;
    }
    else
    {
      bits = type->getPrimitiveSizeInBits();
      numElements = 1;
    }

    unsigned elemSize = (bits+7) >> 3;

    // Special case for pointer types
    if (type->isPointerTy())
    {
      elemSize = sizeof(size_t);
    }

    // Special case for boolean results
    if (bits == 1)
    {
      elemSize = sizeof(bool);
    }

    return pair<unsigned,unsigned>(elemSize,numElements);
  }

  bool isConstantOperand(const llvm::Value *operand)
  {
    unsigned id = operand->getValueID();
    return (id >= llvm::Value::ConstantFirstVal &&
            id <= llvm::Value::ConstantLastVal);
  }

  bool isVector3(const llvm::Value *value)
  {
    return (value->getType()->isVectorTy() &&
            value->getType()->getVectorNumElements() == 3);
  }

  double now()
  {
#if defined(_WIN32) && !defined(__MINGW32__)
    return time(NULL)*1e9;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec*1e3 + tv.tv_sec*1e9;
#endif
  }

  void printTypedData(const llvm::Type *type, const unsigned char *data)
  {
    // TODO: Interpret other types (array, struct)
    unsigned size = getTypeSize(type);
    switch (type->getTypeID())
    {
    case llvm::Type::FloatTyID:
      cout << *(float*)data;
      break;
    case llvm::Type::DoubleTyID:
      cout << *(double*)data;
      break;
    case llvm::Type::IntegerTyID:
      cout << dec;
      switch (size)
      {
      case 1:
        cout << (int)*(char*)data;
        break;
      case 2:
        cout << *(short*)data;
        break;
      case 4:
        cout << *(int*)data;
        break;
      case 8:
        cout << *(long*)data;
        break;
      default:
        cout << "(invalid integer size)";
        break;
      }
      break;
    case llvm::Type::VectorTyID:
    {
      const llvm::Type *elemType = type->getVectorElementType();
      cout << "(";
      for (unsigned i = 0; i < type->getVectorNumElements(); i++)
      {
        if (i > 0)
        {
          cout << ",";
        }
        printTypedData(elemType, data+i*getTypeSize(elemType));
      }
      cout << ")";
      break;
    }
    case llvm::Type::PointerTyID:
      cout << "0x" << hex << *(size_t*)data;
      break;
    case llvm::Type::ArrayTyID:
    {
      const llvm::Type *elemType = type->getArrayElementType();
      unsigned elemSize = getTypeSize(elemType);
      cout << "{";
      for (unsigned i = 0; i < type->getArrayNumElements(); i++)
      {
        if (i > 0)
          cout << ",";
        printTypedData(elemType, data+i*elemSize);
      }
      cout << "}";
      break;
    }
    default:
      cout << "(raw) 0x" << hex << uppercase << setfill('0');
      for (unsigned i = 0; i < size; i++)
      {
        cout << setw(2) << (int)data[i];
      }
    }
  }

  size_t resolveConstantPointer(const llvm::Value *ptr, TypedValueMap& values)
  {
    if (values.count(ptr))
    {
      // In the value map - just return the pointer
      return values.at(ptr).getPointer();
    }
    else if (auto gep = llvm::dyn_cast<llvm::GEPOperator>(ptr))
    {
      // Get base address
      size_t base = resolveConstantPointer(gep->getPointerOperand(), values);
      const llvm::Type *ptrType = gep->getPointerOperandType();

      // Get indices
      std::vector<int64_t> offsets;
      llvm::User::const_op_iterator opItr;
      for (opItr = gep->idx_begin(); opItr != gep->idx_end(); opItr++)
      {
        auto idx = (llvm::ConstantInt*)(opItr->get());
        offsets.push_back(idx->getSExtValue());
      }

      return resolveGEP(base, ptrType, offsets);
    }
    else if (auto bc = llvm::dyn_cast<llvm::BitCastOperator>(ptr))
    {
      // bitcast - no change to the source pointer
      return resolveConstantPointer(bc->getOperand(0), values);
    }
    else if (ptr->getValueID() == llvm::Value::ConstantPointerNullVal)
    {
      return 0;
    }
    else
    {
      FATAL_ERROR("Unsupported constant pointer type: %d", ptr->getValueID());
    }

    return 0;
  }

  size_t resolveGEP(size_t base, const llvm::Type *ptrType,
                    std::vector<int64_t>& offsets)
  {
    size_t address = base;

    // Iterate over indices
    for (int i = 0; i < offsets.size(); i++)
    {
      int64_t offset = offsets[i];

      if (ptrType->isPointerTy())
      {
        // Get pointer element size
        const llvm::Type *elemType = ptrType->getPointerElementType();
        address += offset*getTypeSize(elemType);
        ptrType = elemType;
      }
      else if (ptrType->isArrayTy())
      {
        // Get array element size
        const llvm::Type *elemType = ptrType->getArrayElementType();
        address += offset*getTypeSize(elemType);
        ptrType = elemType;
      }
      else if (ptrType->isVectorTy())
      {
        // Get vector element size
        const llvm::Type *elemType = ptrType->getVectorElementType();
        address += offset*getTypeSize(elemType);
        ptrType = elemType;
      }
      else if (ptrType->isStructTy())
      {
        address +=
          getStructMemberOffset((const llvm::StructType*)ptrType, offset);
        ptrType = ptrType->getStructElementType(offset);
      }
      else
      {
        FATAL_ERROR("Unsupported GEP base type: %d", ptrType->getTypeID());
      }
    }

    return address;
  }

  FatalError::FatalError(const string& msg, const string& file, size_t line)
    : std::runtime_error(msg)
  {
    m_file = file;
    m_line = line;
  }

  FatalError::~FatalError() throw()
  {
  }

  const string& FatalError::getFile() const
  {
    return m_file;
  }

  size_t FatalError::getLine() const
  {
    return m_line;
  }

  const char* FatalError::what() const throw()
  {
    return runtime_error::what();
  }

  MemoryPool::MemoryPool(size_t blockSize) : m_blockSize(blockSize)
  {
    // Force first allocation to create new block
    m_offset = m_blockSize;
  }

  MemoryPool::~MemoryPool()
  {
    for (auto itr = m_blocks.begin(); itr != m_blocks.end(); itr++)
    {
      delete[] *itr;
    }
  }

  uint8_t* MemoryPool::alloc(size_t size)
  {
    if (size == 0)
      return NULL;

    // Check if requested size larger than block size
    if (size > m_blockSize)
    {
      // Oversized buffers allocated separately from main pool
      unsigned char *buffer = new unsigned char[size];
      m_blocks.push_back(buffer);
      return buffer;
    }

    // Round up size to nearest power of two for alignment
    // Taken from here:
    //   http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    unsigned align = size;
    align--;
    align |= align >> 1;
    align |= align >> 2;
    align |= align >> 4;
    align |= align >> 8;
    align |= align >> 16;
    align++;

    // Align offset to size of requested allocation
    if (m_offset & (align-1))
      m_offset += (align - (m_offset & (align-1)));

    // Check if enough space in current block
    if (m_offset + size > m_blockSize)
    {
      // Allocate new block
      m_blocks.push_front(new unsigned char[m_blockSize]);
      m_offset = 0;
    }
    uint8_t *buffer = m_blocks.front() + m_offset;
    m_offset += size;
    return buffer;
  }

  TypedValue MemoryPool::clone(const TypedValue& source)
  {
    TypedValue dest;
    dest.size = source.size;
    dest.num = source.num;
    dest.data = alloc(dest.size*dest.num);
    memcpy(dest.data, source.data, dest.size*dest.num);
    return dest;
  }
}
