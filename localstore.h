#ifndef LOCALSTORE_H
#define LOCALSTORE_H

#include <string>
#include <map>

#include <jsoncpp/json/json.h>

class LocalStore {
  public:
    LocalStore(const std::string &filename);
    ~LocalStore();

    std::string get(const std::string &key) {
      if(data[key].isNull()) return "";
      return data[key].asString();
    }

    void set(const std::string &key, const std::string &value) {
      data[key] = value;
    }

  private:
    std::string filename;
    Json::Value data;
};

#endif
