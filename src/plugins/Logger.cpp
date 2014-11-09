// Logger.cpp (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/common.h"

#include "Logger.h"

using namespace oclgrind;
using namespace std;

Logger::Logger(const Context *context)
 : Plugin(context)
{
}

void Logger::log(MessageType type, const char *message)
{
  cout << endl << message << endl;
}
