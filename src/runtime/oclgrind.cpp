// oclgrind.cpp (Oclgrind)
// Copyright (c) 2013-2016, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "config.h"

#include <iostream>

#include <windows.h>

using namespace std;

#define MAX_DLL_PATH 4096
#define MAX_CMD 32768

static char cmd[MAX_CMD];

static void checkWow64(HANDLE parent, HANDLE child);
static void die(const char *op);
static void getDllPath(char path[MAX_DLL_PATH]);
static bool parseArguments(int argc, char *argv[]);
static void printUsage();
static void setEnvironment(const char *name, const char *value);

int main(int argc, char *argv[])
{
  // Parse arguments
  if (!parseArguments(argc, argv))
  {
    return 1;
  }

  // Get full path to oclgrind-rt.dll
  char dllpath[MAX_DLL_PATH];
  getDllPath(dllpath);


  PROCESS_INFORMATION pinfo = { 0 };
  STARTUPINFOA sinfo = { 0 };
  sinfo.cb = sizeof(sinfo);

  // Create child process in suspended state
  if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_SUSPENDED,
                      NULL, NULL, &sinfo, &pinfo))
    die("creating child process");

  // Check that we are running as 64-bit if and only if we need to be
  checkWow64(GetCurrentProcess(), pinfo.hProcess);

  // Allocate memory for DLL path
  void *childPath = VirtualAllocEx(pinfo.hProcess, NULL, strlen(dllpath)+1,
                                   MEM_COMMIT, PAGE_READWRITE);
  if (!childPath)
    die("allocating child memory");

  // Write DLL path to child
  if (!WriteProcessMemory(pinfo.hProcess, childPath,
                          (void*)dllpath, strlen(dllpath)+1, NULL))
    die("writing child memory");

  // Create thread to load DLL in child process
  HANDLE childThread =
    CreateRemoteThread(pinfo.hProcess, NULL, 0,
                       (LPTHREAD_START_ROUTINE)GetProcAddress(
                         GetModuleHandleA("kernel32.dll"), "LoadLibraryA"),
                       childPath, 0, NULL);
  if (!childThread)
    die("loading DLL in child thread");

  // Wait for child thread to complete
  if (WaitForSingleObject(childThread, INFINITE) != WAIT_OBJECT_0)
    die("waiting for child thread");

  CloseHandle(childThread);
  VirtualFreeEx(pinfo.hProcess, childPath, sizeof(dllpath), MEM_RELEASE);


  // Load DLL in this process as well to get function pointers
  HMODULE dll = LoadLibraryA(dllpath);
  if (!dll)
    die("loading DLL");

  // Get handle to initOclgrind function in DLL
  HANDLE initFunction = GetProcAddress(dll, "initOclgrind");
  if (!initFunction)
    die("getting init function address");

  // Launch init function in child process
  childThread = CreateRemoteThread(pinfo.hProcess, NULL, 0,
                                   (LPTHREAD_START_ROUTINE)initFunction,
                                   NULL, 0, NULL);
  if (!childThread)
    die("launching init in child thread");

  // Wait for init to finish
  if (WaitForSingleObject(childThread, INFINITE) != WAIT_OBJECT_0)
    die("wait failed");


  // Check return value
  DWORD retval = 0;
  if (!GetExitCodeThread(childThread, &retval))
    die("getting init exit code");
  if (!retval)
  {
    std::cout << "[Oclgrind] initialization failed: " << retval << std::endl;
    exit(retval);
  }

  CloseHandle(childThread);

  // Resume child process
  if (ResumeThread(pinfo.hThread) == -1)
    die("resuming thread");

  return 0;
}

static bool parseArguments(int argc, char *argv[])
{
  cmd[0] = '\0';

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "--build-options"))
    {
      if (++i >= argc)
      {
        cerr << "Missing argument to --build-options" << endl;
        return false;
      }
      setEnvironment("OCLGRIND_BUILD_OPTIONS", argv[i]);
    }
    else if (!strcmp(argv[i], "--check-api"))
    {
      setEnvironment("OCLGRIND_CHECK_API", "1");
    }
    else if (!strcmp(argv[i], "--data-races"))
    {
      setEnvironment("OCLGRIND_DATA_RACES", "1");
    }
    else if (!strcmp(argv[i], "--disable-pch"))
    {
      setEnvironment("OCLGRIND_DISABLE_PCH", "1");
    }
    else if (!strcmp(argv[i], "--dump-spir"))
    {
      setEnvironment("OCLGRIND_DUMP_SPIR", "1");
    }
    else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
    {
      printUsage();
      exit(0);
    }
    else if (!strcmp(argv[i], "--inst-counts"))
    {
      setEnvironment("OCLGRIND_INST_COUNTS", "1");
    }
    else if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--interactive"))
    {
      setEnvironment("OCLGRIND_INTERACTIVE", "1");
    }
    else if (!strcmp(argv[i], "--log"))
    {
      if (++i >= argc)
      {
        cerr << "Missing argument to --log" << endl;
        return false;
      }
      setEnvironment("OCLGRIND_LOG", argv[i]);
    }
    else if (!strcmp(argv[i], "--max-errors"))
    {
      if (++i >= argc)
      {
        cerr << "Missing argument to --max-errors" << endl;
        return false;
      }
      setEnvironment("OCLGRIND_MAX_ERRORS", argv[i]);
    }
    else if (!strcmp(argv[i], "--num-threads"))
    {
      if (++i >= argc)
      {
        cerr << "Missing argument to --num-threads" << endl;
        return false;
      }
      setEnvironment("OCLGRIND_NUM_THREADS", argv[i]);
    }
    else if (!strcmp(argv[i], "--pch-dir"))
    {
      if (++i >= argc)
      {
        cerr << "Missing argument to --pch-dir" << endl;
        return false;
      }
      setEnvironment("OCLGRIND_PCH_DIR", argv[i]);
    }
    else if (!strcmp(argv[i], "--plugins"))
    {
      if (++i >= argc)
      {
        cerr << "Missing argument to --plugins" << endl;
        return false;
      }
      setEnvironment("OCLGRIND_PLUGINS", argv[i]);
    }
    else if (!strcmp(argv[i], "-q") || !strcmp(argv[i], "--quick"))
    {
      setEnvironment("OCLGRIND_QUICK", "1");
    }
    else if (!strcmp(argv[i], "--uniform-writes"))
    {
      setEnvironment("OCLGRIND_UNIFORM_WRITES", "1");
    }
    else if (!strcmp(argv[i], "--uninitialized"))
    {
      setEnvironment("OCLGRIND_UNINITIALIZED", "1");
    }
    else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version"))
    {
      cout << endl;
      cout << "Oclgrind " PACKAGE_VERSION << endl;
      cout << endl;
      cout << "Copyright (c) 2013-2016" << endl;
      cout << "James Price and Simon McIntosh-Smith, University of Bristol"
        << endl;
      cout << "https://github.com/jrprice/Oclgrind" << endl;
      cout << endl;
      exit(0);
    }
    else if (argv[i][0] == '-')
    {
      cerr << "Unrecognised option '" << argv[i] << "'" << endl;
      return false;
    }
    else
    {
      for (; i < argc; i++)
      {
        strcat_s(cmd, MAX_CMD, argv[i]);
        strcat_s(cmd, MAX_CMD, " ");
      }
      break;
    }
  }

  if (strlen(cmd) == 0)
  {
    printUsage();
    return false;
  }

  return true;
}

void checkWow64(HANDLE parent, HANDLE child)
{
  BOOL parentWow64, childWow64;
  IsWow64Process(parent, &parentWow64);
  IsWow64Process(child, &childWow64);
  if (parentWow64 != childWow64)
  {
    const char *bits = childWow64 ? "32" : "64";
    std::cout << "[Oclgrind] ";
    std::cout << "target application is " << bits << "-bit" << std::endl;
    std::cout << "Use the " << bits << "-bit version of oclgrind.exe" << std::endl;

    exit(1);
  }
}

void die(const char *op)
{
  DWORD err = GetLastError();
  char buffer[1024];
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    buffer, 1024, NULL);
  std::cout << "[Oclgrind] Error while '" << op << "':" << std::endl
    << buffer << std::endl;
  exit(1);
}

static void getDllPath(char path[MAX_DLL_PATH])
{
  // Get full path to exeuctable
  GetModuleFileNameA(GetModuleHandle(NULL), path, MAX_PATH);

  // Remove executable filename
  char *dirend;
  if ((dirend = strrchr(path, '\\')))
    *dirend = '\0';

  // Append relative path to DLL
  strcat_s(path, 4096, "\\..\\lib\\oclgrind-rt.dll");
}

static void printUsage()
{
  cout
    << "Usage: oclgrind [OPTIONS] COMMAND" << endl
    << "       oclgrind [--help | --version]" << endl
    << endl
    << "Options:" << endl
    << "     --build-options  OPTIONS  "
             "Additional options to pass to the OpenCL compiler" << endl
    << "     --check-api               "
             "Reports errors on API calls" << endl
    << "     --data-races              "
             "Enable data-race detection" << endl
    << "     --disable-pch             "
             "Don't use precompiled headers" << endl
    << "     --dump-spir               "
             "Dump SPIR to /tmp/oclgrind_*.{ll,bc}" << endl
    << "  -h --help                    "
             "Display usage information" << endl
    << "     --inst-counts             "
             "Output histograms of instructions executed" << endl
    << "  -i --interactive             "
             "Enable interactive mode" << endl
    << "     --log            LOGFILE  "
             "Redirect log/error messages to a file" << endl
    << "     --max-errors     NUM      "
             "Limit the number of error/warning messages" << endl
    << "     --num-threads    NUM      "
             "Set the number of worker threads to use" << endl
    << "     --pch-dir        DIR      "
             "Override directory containing precompiled headers" << endl
    << "     --plugins        PLUGINS  "
             "Load colon separated list of plugin libraries" << endl
    << "  -q --quick                   "
             "Only run first and last work-group" << endl
    << "     --uniform-writes          "
             "Don't suppress uniform write-write data-races" << endl
    << "     --uninitialized           "
             "Report usage of uninitialized values" << endl
    << "  -v --version                 "
             "Display version information" << endl
    << endl
    << "For more information, please visit the Oclgrind wiki page:" << endl
    << "-> https://github.com/jrprice/Oclgrind/wiki" << endl
    << endl;
}

static void setEnvironment(const char *name, const char *value)
{
  _putenv_s(name, value);
}
