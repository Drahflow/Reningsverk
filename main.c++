#include "reningsverk.h"
#include "terminalui.h"

#include <iostream>
#include <exception>

using namespace std;

int usage() {
  cerr << "usage: ./reningsverk <API-Key>" << endl;
  return 1;
}

int main(int argc, char *argv[]) {
  try {
    if(argc != 2) {
      return usage();
    }

    TerminalUI{argv[1]} ();

    return 0;
  } catch(std::exception &e) {
    cerr << e.what() << endl;
    return 2;
  }
}
