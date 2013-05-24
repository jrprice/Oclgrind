#include "config.h"
#include <fstream>
#include <iostream>

#include "Simulator.h"

using namespace std;

int main(int argc, char *argv[])
{
  // Two arguments expected
  if (argc != 2)
  {
    cout << "Usage: oclgrind filename" << endl;
    return 1;
  }

  // Attempt to open simulator file
  ifstream input;
  input.open(argv[1]);
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
  simulator.run();
}
