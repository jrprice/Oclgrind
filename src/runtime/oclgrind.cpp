// oclgrind.cpp (Oclgrind)
// Copyright (c) 2013-2019, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "config.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#if defined(_WIN32) && !defined(__MINGW32__)
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

using namespace std;

#if defined(_WIN32) && !defined(__MINGW32__)

static string appCmd;
static void checkWow64(HANDLE parent, HANDLE child);
static void die(const char* op);

#else // not Windows

static char** appArgs = NULL;
#ifdef __APPLE__
#define LIB_EXTENSION "dylib"
#define LD_LIBRARY_PATH_ENV "DYLD_LIBRARY_PATH"
#define LD_PRELOAD_ENV "DYLD_INSERT_LIBRARIES"
#else
#define LIB_EXTENSION "so"
#define LD_LIBRARY_PATH_ENV "LD_LIBRARY_PATH"
#define LD_PRELOAD_ENV "LD_PRELOAD"
#endif

#endif

static string getLibDirPath();
static bool parseArguments(int argc, char* argv[]);
static void printUsage();
static void setEnvironment(const char* name, const char* value);

int main(int argc, char* argv[])
{
  // Parse arguments
  if (!parseArguments(argc, argv))
  {
    return 1;
  }

#if defined(_WIN32) && !defined(__MINGW32__)

  // Get full path to oclgrind-rt.dll
  string dllpath = getLibDirPath();
  dllpath += "\\oclgrind-rt.dll";

  PROCESS_INFORMATION pinfo = {0};
  STARTUPINFOA sinfo = {0};
  sinfo.cb = sizeof(sinfo);

  // Create child process in suspended state
  if (!CreateProcessA(NULL, (LPSTR)appCmd.c_str(), NULL, NULL, FALSE,
                      CREATE_SUSPENDED, NULL, NULL, &sinfo, &pinfo))
    die("creating child process");

  // Check that we are running as 64-bit if and only if we need to be
  checkWow64(GetCurrentProcess(), pinfo.hProcess);

  // Allocate memory for DLL path
  void* childPath = VirtualAllocEx(pinfo.hProcess, NULL, dllpath.size() + 1,
                                   MEM_COMMIT, PAGE_READWRITE);
  if (!childPath)
    die("allocating child memory");

  // Write DLL path to child
  if (!WriteProcessMemory(pinfo.hProcess, childPath, (void*)dllpath.c_str(),
                          dllpath.size() + 1, NULL))
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
    die("waiting for load thread");

  CloseHandle(childThread);
  VirtualFreeEx(pinfo.hProcess, childPath, dllpath.size() + 1, MEM_RELEASE);

  // Load DLL in this process as well to get function pointers
  HMODULE dll = LoadLibraryA(dllpath.c_str());
  if (!dll)
    die("loading DLL");

  // Get handle to initOclgrind function in DLL
  HANDLE initFunction = GetProcAddress(dll, "initOclgrind");
  if (!initFunction)
    die("getting init function address");

  // Launch init function in child process
  childThread =
    CreateRemoteThread(pinfo.hProcess, NULL, 0,
                       (LPTHREAD_START_ROUTINE)initFunction, NULL, 0, NULL);
  if (!childThread)
    die("launching init in child thread");

  // Wait for init to finish
  if (WaitForSingleObject(childThread, INFINITE) != WAIT_OBJECT_0)
    die("waiting for init thread");

  // Check return value
  DWORD retval = 0;
  if (!GetExitCodeThread(childThread, &retval))
    die("getting init exit code");
  if (!retval)
  {
    cerr << "[Oclgrind] initialization failed: " << retval << endl;
    exit(retval);
  }

  CloseHandle(childThread);

  // Resume child process
  if (ResumeThread(pinfo.hThread) == -1)
    die("resuming thread");

  // Wait for child process to finish
  if (WaitForSingleObject(pinfo.hProcess, INFINITE) != WAIT_OBJECT_0)
    die("waiting for child process failed");

  // Get return code and forward it
  if (!GetExitCodeProcess(pinfo.hProcess, &retval))
    die("getting child process exit code");

  return retval;

#else // not Windows

  // Get path to Oclgrind library directory
  string libdir = getLibDirPath();

  // Construct new LD_LIBRARY_PATH
  string ldLibraryPath = libdir;
  const char* oldLdLibraryPath = getenv(LD_LIBRARY_PATH_ENV);
  if (oldLdLibraryPath)
  {
    ldLibraryPath += ":";
    ldLibraryPath += oldLdLibraryPath;
  }

  // Add oclgrind-rt library to LD_PRELOAD
  string ldPreload = libdir;
  ldPreload += "/liboclgrind-rt.";
  ldPreload += LIB_EXTENSION;
  const char* oldLdPreload = getenv(LD_PRELOAD_ENV);
  if (oldLdPreload)
  {
    ldPreload += ":";
    ldPreload += oldLdPreload;
  }

  setEnvironment(LD_LIBRARY_PATH_ENV, ldLibraryPath.c_str());
  setEnvironment(LD_PRELOAD_ENV, ldPreload.c_str());
#ifdef __APPLE__
  setEnvironment("DYLD_FORCE_FLAT_NAMESPACE", "1");
#endif

  // Launch target application
  if (execvp(appArgs[0], appArgs) == -1)
  {
    cerr << "[Oclgrind] Failed to launch target application" << endl;
    exit(1);
  }

#endif
}

static bool parseArguments(int argc, char* argv[])
{
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
    else if (!strcmp(argv[i], "--compute-units"))
    {
      if (++i >= argc)
      {
        cerr << "Missing argument to --compute-units" << endl;
        return false;
      }
      setEnvironment("OCLGRIND_COMPUTE_UNITS", argv[i]);
    }
    else if (!strcmp(argv[i], "--constant-mem-size"))
    {
      if (++i >= argc)
      {
        cerr << "Missing argument to --constant-mem-size" << endl;
        return false;
      }
      setEnvironment("OCLGRIND_CONSTANT_MEM_SIZE", argv[i]);
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
    else if (!strcmp(argv[i], "--global-mem-size"))
    {
      if (++i >= argc)
      {
        cerr << "Missing argument to --global-mem-size" << endl;
        return false;
      }
      setEnvironment("OCLGRIND_GLOBAL_MEM_SIZE", argv[i]);
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
    else if (!strcmp(argv[i], "--local-mem-size"))
    {
      if (++i >= argc)
      {
        cerr << "Missing argument to --local-mem-size" << endl;
        return false;
      }
      setEnvironment("OCLGRIND_LOCAL_MEM_SIZE", argv[i]);
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
    else if (!strcmp(argv[i], "--max-wgsize"))
    {
      if (++i >= argc)
      {
        cerr << "Missing argument to --max-wgsize" << endl;
        return false;
      }
      setEnvironment("OCLGRIND_MAX_WGSIZE", argv[i]);
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
      cout << "Copyright (c) 2013-2019" << endl;
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
#if defined(_WIN32) && !defined(__MINGW32__)
      // Build command-line for target application
      for (; i < argc; i++)
      {
        appCmd += argv[i];
        appCmd += " ";
      }
#else // not Windows
      appArgs = (char**)malloc((argc - i + 1) * sizeof(char*));
      int offset = i;
      for (; i < argc; i++)
      {
        appArgs[i - offset] = argv[i];
      }
      appArgs[argc - offset] = NULL;
#endif
      break;
    }
  }

#if defined(_WIN32) && !defined(__MINGW32__)
  if (appCmd.size() == 0)
#else
  if (!appArgs)
#endif
  {
    printUsage();
    return false;
  }

  return true;
}

static void stripLastComponent(string& path)
{
  size_t slash;
#if defined(_WIN32) && !defined(__MINGW32__)
  if ((slash = path.find_last_of('\\')) == string::npos)
#else
  if ((slash = path.find_last_of('/')) == string::npos)
#endif
  {
    cerr << "[Oclgrind] Failed to get path to library directory" << endl;
    exit(1);
  }

  path.resize(slash);
}

static string getLibDirPath()
{
  string libdir;

  // Get full path to executable
#if defined(_WIN32) && !defined(__MINGW32__)
  char path[MAX_PATH];
  GetModuleFileNameA(GetModuleHandle(NULL), path, MAX_PATH);
  if (GetLastError() != ERROR_SUCCESS)
    die("getting path to Oclgrind installation");
  libdir = path;
#else
  char exepath[PATH_MAX];
  char path[PATH_MAX];
  // Get path to executable
#if defined(__APPLE__)
  uint32_t sz = PATH_MAX;
  if (_NSGetExecutablePath(exepath, &sz))
#else // not apple
  if (readlink("/proc/self/exe", exepath, PATH_MAX) == -1)
#endif
  {
    cerr << "[Oclgrind] Unable to get path to Oclgrind installation" << endl;
    exit(1);
  }
  // Resolve symbolic links and normalise path
  realpath(exepath, path);
  libdir = path;
#endif

  // Remove executable filename
  stripLastComponent(libdir);

  const char* testing = getenv("OCLGRIND_TESTING");
  if (!testing)
  {
    // Remove containing directory and append library directory
    stripLastComponent(libdir);
    libdir += "/lib" LIBDIR_SUFFIX;
  }

  return libdir;
}

static void printUsage()
{
  cout << "Usage: oclgrind [OPTIONS] COMMAND" << endl
       << "       oclgrind [--help | --version]" << endl
       << endl
       << "Options:" << endl
       << "  --build-options     OPTIONS  "
          "Additional options to pass to the OpenCL compiler"
       << endl
       << "  --check-api                  "
          "Report errors on API calls"
       << endl
       << "  --compute-units     UNITS    "
          "Change the number of compute units reported"
       << endl
       << "  --constant-mem-size BYTES    "
          "Change the constant memory size of the device"
       << endl
       << "  --data-races                 "
          "Enable data-race detection"
       << endl
       << "  --disable-pch                "
          "Don't use precompiled headers"
       << endl
       << "  --dump-spir                  "
          "Dump SPIR to /tmp/oclgrind_*.{ll,bc}"
       << endl
       << "  --global-mem-size   BYTES    "
          "Change the global memory size of the device"
       << endl
       << "  --help [-h]                  "
          "Display usage information"
       << endl
       << "  --inst-counts                "
          "Output histograms of instructions executed"
       << endl
       << "  --interactive [-i]           "
          "Enable interactive mode"
       << endl
       << "  --local-mem-size    BYTES    "
          "Change the local memory size of the device"
       << endl
       << "  --log               LOGFILE  "
          "Redirect log/error messages to a file"
       << endl
       << "  --max-errors        NUM      "
          "Limit the number of error/warning messages"
       << endl
       << "  --max-wgsize        WGSIZE   "
          "Change the maximum work-group size of the device"
       << endl
       << "  --num-threads       NUM      "
          "Set the number of worker threads to use"
       << endl
       << "  --pch-dir           DIR      "
          "Override directory containing precompiled headers"
       << endl
       << "  --plugins           PLUGINS  "
          "Load colon separated list of plugin libraries"
       << endl
       << "  --quick [-q]                 "
          "Only run first and last work-group"
       << endl
       << "  --uniform-writes             "
          "Don't suppress uniform write-write data-races"
       << endl
       << "  --uninitialized              "
          "Report usage of uninitialized values"
       << endl
       << "  --version [-v]               "
          "Display version information"
       << endl
       << endl
       << "For more information, please visit the Oclgrind wiki page:" << endl
       << "-> https://github.com/jrprice/Oclgrind/wiki" << endl
       << endl;
}

static void setEnvironment(const char* name, const char* value)
{
#if defined(_WIN32) && !defined(__MINGW32__)
  _putenv_s(name, value);
#else
  setenv(name, value, 1);
#endif
}

#if defined(_WIN32) && !defined(__MINGW32__)

void checkWow64(HANDLE parent, HANDLE child)
{
  BOOL parentWow64, childWow64;
  IsWow64Process(parent, &parentWow64);
  IsWow64Process(child, &childWow64);
  if (parentWow64 != childWow64)
  {
    const char* bits = childWow64 ? "32" : "64";
    cerr << "[Oclgrind] target application is " << bits << "-bit" << endl
         << "Use the " << bits << "-bit version of oclgrind.exe" << endl;
    exit(1);
  }
}

void die(const char* op)
{
  DWORD err = GetLastError();
  char buffer[1024];
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, 1024, NULL);
  cerr << "[Oclgrind] Error while '" << op << "':" << endl << buffer << endl;
  exit(1);
}

#endif
