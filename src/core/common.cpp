// common.cpp (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

#if defined(_WIN32) && !defined(__MINGW32__)
#include <time.h>
#else
#include <sys/time.h>
#endif

#include "llvm/Instructions.h"
#include "llvm/Operator.h"
#include "llvm/Support/raw_os_ostream.h"

using namespace oclgrind;
using namespace std;

namespace oclgrind
{
  _Size3_::_Size3_()
  {
    x = y = z = 0;
  }

  _Size3_::_Size3_(size_t _x, size_t _y, size_t _z)
  {
    x = _x;
    y = _y;
    z = _z;
  }

  _Size3_::_Size3_(size_t linear, _Size3_ dimensions)
  {
    x = linear % dimensions.x;
    y = (linear / dimensions.x) % dimensions.y;
    z = (linear / (dimensions.x * dimensions.y));
  }

  size_t& Size3::operator[](size_t i)
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
    }
  }

  const size_t& Size3::operator[](size_t i) const
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
    }
  }

  bool Size3::operator==(const Size3& rhs) const
  {
    return x == rhs.x && y == rhs.y && z == rhs.z;
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
      FATAL_ERROR("Unsupported float size: %lu bytes", size);
    }
  }

  size_t TypedValue::getPointer(unsigned index) const
  {
    if (size != sizeof(size_t))
    {
      FATAL_ERROR("Unsupported pointer size: %lu bytes", size);
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
      FATAL_ERROR("Unsupported signed int size: %lu bytes", size);
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
      FATAL_ERROR("Unsupported unsigned int size: %lu bytes", size);
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
      FATAL_ERROR("Unsupported float size: %lu bytes", size);
    }
  }

  void TypedValue::setPointer(size_t value, unsigned index)
  {
    if (size != sizeof(size_t))
    {
      FATAL_ERROR("Unsupported pointer size: %lu bytes", size);
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
      FATAL_ERROR("Unsupported signed int size: %lu bytes", size);
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
      FATAL_ERROR("Unsupported unsigned int size: %lu bytes", size);
    }
  }

  TypedValue TypedValue::clone() const
  {
    TypedValue result;
    result.size = size;
    result.num  = num;
    result.data = new unsigned char[size*num];
    memcpy(result.data, data, size*num);
    return result;
  }

  void dumpInstruction(ostream& out, const llvm::Instruction *instruction)
  {
    llvm::raw_os_ostream stream(out);
    instruction->print(stream);
  }

  void getConstantData(unsigned char *data, const llvm::Constant *constant)
  {
    if (constant->getValueID() == llvm::Value::UndefValueVal)
    {
      memset(data, -1, getTypeSize(constant->getType()));
      return;
    }

    const llvm::Type *type = constant->getType();
    size_t size = getTypeSize(type);
    switch (type->getTypeID())
    {
    case llvm::Type::IntegerTyID:
      memcpy(data,
             ((llvm::ConstantInt*)constant)->getValue().getRawData(),
             size);
      break;
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
      int num = type->getVectorNumElements();
      const llvm::Type *elemType = type->getVectorElementType();
      size_t elemSize = getTypeSize(elemType);
      for (int i = 0; i < num; i++)
      {
        getConstantData(data + i*elemSize, constant->getAggregateElement(i));
      }
      break;
    }
    case llvm::Type::ArrayTyID:
    {
      int num = type->getArrayNumElements();
      const llvm::Type *elemType = type->getArrayElementType();
      size_t elemSize = getTypeSize(elemType);
      for (int i = 0; i < num; i++)
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
      int num = type->getStructNumElements();
      for (int i = 0; i < num; i++)
      {
        size_t offset = getStructMemberOffset((const llvm::StructType*)type, i);
        getConstantData(data + offset, constant->getAggregateElement(i));
      }
      break;
    }
    default:
      FATAL_ERROR("Unsupported constant type: %d", type->getTypeID());
    }
  }

  const llvm::Instruction* getConstExprAsInstruction(
    const llvm::ConstantExpr *expr)
  {
    // Get operands
    int numOperands = expr->getNumOperands();
    llvm::Value **valueOperands = new llvm::Value*[numOperands];
    for (int i = 0; i < numOperands; i++)
    {
      valueOperands[i] = expr->getOperand(i);
    }
    llvm::ArrayRef<llvm::Value*> operands(valueOperands, numOperands);

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
        return llvm::GetElementPtrInst::Create(operands[0], operands.slice(1));
      }
    case llvm::Instruction::ICmp:
    case llvm::Instruction::FCmp:
      return llvm::CmpInst::Create((llvm::Instruction::OtherOps)opcode,
                                   expr->getPredicate(),
                                   operands[0], operands[1]);
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

  size_t getStructMemberOffset(const llvm::StructType *type, size_t index)
  {
    bool packed = ((llvm::StructType*)type)->isPacked();

    size_t offset = 0;
    for (int i = 0; i <= index; i++)
    {
      // Get member size and alignment
      const llvm::Type *elemType = type->getStructElementType(i);
      size_t size = getTypeSize(elemType);
      size_t align = getTypeAlignment(elemType);

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
    assert(false);
  }

  size_t getTypeSize(const llvm::Type *type)
  {
    if (type->isArrayTy())
    {
      size_t num = type->getArrayNumElements();
      size_t sz = getTypeSize(type->getArrayElementType());
      return num*sz;
    }
    else if (type->isStructTy())
    {
      bool packed = ((llvm::StructType*)type)->isPacked();

      size_t size = 0;
      size_t alignment = 1;
      for (int i = 0; i < type->getStructNumElements(); i++)
      {
        // Get member size and alignment
        const llvm::Type *elemType = type->getStructElementType(i);
        size_t sz    = getTypeSize(elemType);
        size_t align = getTypeAlignment(elemType);

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
      size_t num = type->getVectorNumElements();
      size_t sz = getTypeSize(type->getVectorElementType());
      if (num == 3) num = 4; // Hack for 3-element vectors
      return num*sz;
    }
    else if (type->isPointerTy())
    {
      return sizeof(size_t);
    }
    else
    {
      // For some reason, getScalarSizeInBits is not const
      llvm::Type* nonConstTy = const_cast<llvm::Type*>(type);

      // Round up for types that have a bit size not multiple of 8
      // like "bool".
      size_t ret = nonConstTy->getScalarSizeInBits() / 8;
      if (nonConstTy->getScalarSizeInBits() % 8)
        ret++;

      return ret;
    }
  }

  /// Returns the byte alignment of this type
  size_t getTypeAlignment(const llvm::Type* type) {
    using namespace llvm;
    // Array types are aligned to their element type
    if (const ArrayType* psAT = dyn_cast<ArrayType>(type)) {
      return getTypeAlignment(psAT->getElementType());
    }

    // Struct alignment is the size of its largest contained type
    if (const StructType* structT = dyn_cast<StructType>(type)) {
      if (structT->isPacked()) return 1;
      StructType* nonConstTy = const_cast<StructType*>(structT);
      uint32_t uAlign = 0, uMaxAlign = 1;
      uint32_t uCount = structT->getNumElements();
      for (uint32_t i = 0; i < uCount; i++) {
          const Type* psElemType = nonConstTy->getTypeAtIndex(i);
          uAlign = getTypeAlignment(psElemType);

          if (uAlign > uMaxAlign)
              uMaxAlign = uAlign;
      }

      return uMaxAlign;
    }

    return getTypeSize(type);
  }

  pair<size_t,size_t> getValueSize(const llvm::Value *value)
  {
    size_t bits, numElements;
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

    size_t elemSize = bits >> 3;

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

    return pair<size_t,size_t>(elemSize,numElements);
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
    size_t size = getTypeSize(type);
    switch (type->getTypeID())
    {
    case llvm::Type::FloatTyID:
      cout << *(float*)data;
      break;
    case llvm::Type::DoubleTyID:
      cout << *(double*)data;
      break;
    case llvm::Type::IntegerTyID:
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
      for (int i = 0; i < type->getVectorNumElements(); i++)
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
    default:
      cout << "(raw) 0x" << hex << uppercase << setfill('0');
      for (int i = 0; i < size; i++)
      {
        cout << setw(2) << (int)data[i];
      }
    }
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
}
