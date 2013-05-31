#include "common.h"
#include <fstream>

#include "Simulator.h"

using namespace std;

static unsigned char outputMask = 0;
static const char *simfile = NULL;

static bool parseArguments(int argc, char *argv[]);
static void printUsage();

int main(int argc, char *argv[])
{
  // Parse arguments
  if (!parseArguments(argc, argv))
  {
    return 1;
  }

  // Attempt to open simulator file
  ifstream input;
  input.open(simfile);
  if (input.fail())
  {
    cout << "Unable to open simulator file." << endl;
    return 1;
  }

  // Initialise simulator
  Simulator simulator;
  bool ret = simulator.init(input);
  input.close();
  if (!ret)
  {
    return 1;
  }

  // Run simulator
  simulator.setOutputMask(outputMask);
  simulator.run();
}

static bool parseArguments(int argc, char *argv[])
{
  for (int i = 1; i < argc; i++)
  {
    if (argv[i][0] == '-')
    {
      char *opt = argv[i] + 1;
      while (*opt != '\0')
      {
        switch (*opt)
        {
        case 'g':
          outputMask |= Simulator::OUTPUT_GLOBAL_MEM;
          break;
        case 'l':
          outputMask |= Simulator::OUTPUT_LOCAL_MEM;
          break;
        case 'p':
          outputMask |= Simulator::OUTPUT_PRIVATE_MEM;
          break;
        case 'i':
          outputMask |= Simulator::OUTPUT_INSTRUCTIONS;
          break;
        default:
          cout << "Unrecognised option '" << argv[i] << "'" << endl;
          return false;
        }
        opt++;
      }
    }
    else
    {
      if (simfile == NULL)
      {
        simfile = argv[i];
      }
      else
      {
        cout << "Unexpected positional argument '" << argv[i] << "'" << endl;
        return false;
      }
    }
  }

  if (simfile == NULL)
  {
    printUsage();
    return false;
  }

  return true;
}

static void printUsage()
{
  cout << "Usage: oclgrind [-g] [-p] [-i] simfile" << endl;
}
