#include "localstore.h"

#include <fstream>
#include <stdexcept>

using namespace std;

LocalStore::LocalStore(const string &filename): filename(filename) {
  ifstream in(filename);
  if(!in.good()) {
    Json::Reader().parse("{}", data);
  } else {
    if(!(Json::Reader().parse(in, data))) throw runtime_error("local store could not be parsed");
  }
}

LocalStore::~LocalStore() {
  {
    ofstream out(filename + ".new");
    out << Json::StyledWriter().write(data);
  }
  rename((filename + ".new").c_str(), filename.c_str());
}
