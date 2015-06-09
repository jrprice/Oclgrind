// Logger.cpp (Oclgrind)
// Copyright (c) 2013-2016, James Price and Simon McIntosh-Smith,
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
  m_maxErrors = DEFAULT_MAX_ERRORS;
  const char *maxErrors = getenv("OCLGRIND_MAX_ERRORS");
  if (maxErrors)
  {
    char *next;
    m_maxErrors = strtoul(maxErrors, &next, 10);
    if (strlen(next))
    {
      cerr << "Oclgrind: Invalid value for OCLGRIND_MAX_ERRORS" << endl;
    }
  }

  m_diagnosticOptions = std::map<MessageType, bool>();
  char *diagnosticOptions = getenv("OCLGRIND_DIAGNOSTIC_OPTIONS");
  if (diagnosticOptions)
  {
    parseDiagnosticOptions(diagnosticOptions);
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
  lock_guard<mutex> lock(logMutex);

  // Limit number of errors/warning printed
  if (!hasDiagnosticOption(type) && type != ERROR_FATAL)
  {
    return;
  }

  MessageType baseType = getMessageBaseType(type);
  if (baseType == ERROR || baseType == WARNING)
  {
    if (m_numErrors == m_maxErrors && hasDiagnosticOption(INFO))
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

bool Logger::hasDiagnosticOption(MessageType optType)
{
  return m_diagnosticOptions[optType];
}

void Logger::enableDiagnosticOption(MessageType optType, bool status)
{
  m_diagnosticOptions[optType] = status;
}

void Logger::enableAllDiagnostics()
{
  // Turn on generic groups
  enableDiagnosticOption(INFO);
  enableDiagnosticOption(DEBUG);
  enableDiagnosticOption(WARNING);
  enableDiagnosticOption(ERROR);
  // Turn on specific groups
  enableDiagnosticOption(WARNING_UNINITIALIZED);
  enableDiagnosticOption(ERROR_DIVERGENCE);
  enableDiagnosticOption(ERROR_INVALID_ACCESS);
  enableDiagnosticOption(ERROR_DATA_RACE);
  enableDiagnosticOption(ERROR_UNALIGNED);
  enableDiagnosticOption(ERROR_ARRAY_BOUNDS);
}

void Logger::logBadDiagnosticOption(const char* opt, bool isInvalid)
{
  std::stringstream msg;
  msg << (isInvalid ? "Invalid" : "Unknown")
      << " diagnostic option '" << opt << "'.";
  log(WARNING, msg.str().c_str());
}

bool Logger::parseDiagnosticOptions(char *options)
{
  if (options == NULL)
  {
    return true;
  }

  std::vector<const char*> invalidOptions;
  std::vector<const char*> unknownOptions;

  for (char *opt = strtok(options, " "); opt; opt = strtok(NULL, " "))
  {
    bool isPositive = strncmp(opt, "-Wno-", 5);
    char *opt_type = NULL;

    if (isPositive)
    {
      if (strlen(opt) <= 2)
      {
        invalidOptions.push_back(opt);
        continue;
      }
      else
      {
        opt_type = opt + 2;
      }
    }
    else
    {
      if (strlen(opt) <= 5)
      {
        invalidOptions.push_back(opt);
        continue;
      }
      else
      {
        opt_type = opt + 5;
      }
    }

    assert(opt_type);

    if (!strcmp(opt_type, "all") && isPositive)
    {
      enableAllDiagnostics();
    }
    else if (!strcmp(opt_type, "generic"))
    {
      enableDiagnosticOption(WARNING, isPositive);
      enableDiagnosticOption(ERROR, isPositive);
    }
    else if (!strcmp(opt_type, "info"))
    {
      enableDiagnosticOption(INFO, isPositive);
    }
    else if (!strcmp(opt_type, "debug"))
    {
      enableDiagnosticOption(DEBUG, isPositive);
    }
    else if (!strcmp(opt_type, "uninitialized"))
    {
      enableDiagnosticOption(WARNING_UNINITIALIZED, isPositive);
    }
    else if (!strcmp(opt_type, "divergence"))
    {
      enableDiagnosticOption(ERROR_DIVERGENCE, isPositive);
    }
    else if (!strcmp(opt_type, "invalid-access"))
    {
      enableDiagnosticOption(ERROR_INVALID_ACCESS, isPositive);
    }
    else if (!strcmp(opt_type, "data-race"))
    {
      enableDiagnosticOption(ERROR_DATA_RACE, isPositive);
    }
    else if (!strcmp(opt_type, "unaligned"))
    {
      enableDiagnosticOption(ERROR_UNALIGNED, isPositive);
    }
    else if (!strcmp(opt_type, "array-bounds"))
    {
      enableDiagnosticOption(ERROR_ARRAY_BOUNDS, isPositive);
    }
    else
    {
      unknownOptions.push_back(opt);
    }
  }

  for (int i = 0; i < invalidOptions.size(); ++i)
  {
    logBadDiagnosticOption(invalidOptions[i], true);
  }

  for (int i = 0; i < unknownOptions.size(); ++i)
  {
    logBadDiagnosticOption(unknownOptions[i]);
  }

  return (!invalidOptions.size() && !unknownOptions.size());
}
