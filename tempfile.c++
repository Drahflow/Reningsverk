#include "tempfile.h"

#include <memory>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

using namespace std;

Tempfile::Tempfile() {
  const char *TMPDIR = getenv("TMPDIR");
  if(!TMPDIR) TMPDIR = "/tmp";

  unique_ptr<char> cName(strdup((TMPDIR + std::string("/reningsverkXXXXXX")).c_str()));
  fd = mkstemp(cName.get());

  try {
    fileName = std::string(cName.get());
  } catch(...) {
    unlink(cName.get());
    close(fd);
    throw;
  }
}

Tempfile::~Tempfile() {
  unlink(fileName.c_str());
  close(fd);
}
