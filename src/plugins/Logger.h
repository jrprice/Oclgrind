// Logger.h (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/Plugin.h"

namespace oclgrind
{
  class Logger : public Plugin
  {
  public:
    Logger(const Context *context);
    virtual ~Logger();

    virtual void log(MessageType type, const char *message) override;

  private:
    std::ostream *m_log;

    unsigned m_maxErrors;
    static unsigned m_numErrors;
  };
}
