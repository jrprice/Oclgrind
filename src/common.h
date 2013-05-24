#ifndef __common_h_
#define __common_h_

#include "config.h"
#include <iomanip>
#include <iostream>
#include <map>
#include <vector>

namespace llvm
{
  class Value;
}

// Structure for a value with a size/type
typedef struct
{
  size_t size;
  unsigned char *data;
} TypedValue;

// Private memory map type
typedef std::map<const llvm::Value*,TypedValue> TypedValueMap;

// Clone a TypedValue
static TypedValue clone(TypedValue source)
{
  TypedValue dest;
  dest.size = source.size;
  dest.data = new unsigned char[dest.size];
  memcpy(dest.data, source.data, dest.size);
  return dest;
}

#endif // __common_h_
