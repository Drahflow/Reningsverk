#include "reningsverk.h"
#include "terminalui.h"

#include <iostream>
#include <exception>

using namespace std;

int usage() {
  cerr << "usage: ./reningsverk <apikey> <user> <password>" << endl;
  return 1;
}

int main(int argc, char *argv[]) {
  try {
    if(argc != 4) {
      return usage();
    }

    TerminalUI(Reningsverk(argv[1], argv[2], argv[3])) ();

    return 0;
  } catch(std::exception &e) {
    cerr << e.what() << endl;
    return 2;
  }
}
