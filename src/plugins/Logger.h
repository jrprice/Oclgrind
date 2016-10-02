// Logger.h (Oclgrind)
// Copyright (c) 2013-2016, James Price and Simon McIntosh-Smith,
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
    int m_stopErrors;
    std::map<MessageType, bool> m_diagnosticOptions;
    static unsigned m_numErrors;

    bool hasDiagnosticOption(MessageType optType);
    void enableDiagnosticOption(MessageType optType, bool status = true);
    void enableAllDiagnostics();
    void logBadDiagnosticOption(const char* opt, bool isInvalid = false);
    bool parseDiagnosticOptions(char *options);
};
}
