// Plugin.cpp (Oclgrind)
// Copyright (c) 2013-2015, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "Plugin.h"

using namespace oclgrind;

Plugin::Plugin(const Context *context)
  : m_context(context)
{
}

Plugin::~Plugin()
{
}

bool Plugin::isThreadSafe() const
{
  return true;
}
