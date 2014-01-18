#ifndef TEMPFILE_H
#define TEMPFILE_H

#include <string>

class Tempfile {
  public:
    Tempfile();
    ~Tempfile();
    const std::string &name() { return fileName; };

  private:
    int fd;
    std::string fileName;
};

#endif
