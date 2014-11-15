// Logger.cpp (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/common.h"

#include <fstream>

#include "Logger.h"

using namespace oclgrind;
using namespace std;

Logger::Logger(const Context *context)
 : Plugin(context)
{
  m_log = &cerr;

  const char *logfile = getenv("OCLGRIND_LOG");
  if (logfile)
  {
    m_log = new ofstream(logfile);
    if (!m_log->good())
    {
      cerr << "Oclgrind: Unable to open log file '" << logfile << "'" << endl;
      m_log = &cerr;
    }
  }
}

Logger::~Logger()
{
  if (m_log != &cerr)
  {
    ((ofstream*)m_log)->close();
    delete m_log;
  }
}

void Logger::log(MessageType type, const char *message)
{
  *m_log << endl << message << endl;
}
