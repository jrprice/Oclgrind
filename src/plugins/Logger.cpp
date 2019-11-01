// Logger.cpp (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "core/common.h"

#include <fstream>
#include <mutex>

#include "Logger.h"

using namespace oclgrind;
using namespace std;

#define DEFAULT_MAX_ERRORS 1000

unsigned Logger::m_numErrors = 0;

static mutex logMutex;

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

  m_maxErrors = getEnvInt("OCLGRIND_MAX_ERRORS", DEFAULT_MAX_ERRORS);
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
  lock_guard<mutex> lock(logMutex);

  // Limit number of errors/warning printed
  if (type == ERROR || type == WARNING)
  {
    if (m_numErrors == m_maxErrors)
    {
      *m_log << endl << "Oclgrind: "
             << m_numErrors << " errors generated - suppressing further errors"
             << endl << endl;
    }
    if (m_numErrors++ >= m_maxErrors)
      return;
  }

  *m_log << endl << message << endl;
}
