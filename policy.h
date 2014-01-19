#ifndef POLICY_H
#define POLICY_H

#include <sstream>

#include <jsoncpp/json/json.h>

class Reningsverk;

class Policy {
  public:
    Policy(Reningsverk &r, const Json::Value &data): r(r), data(data) { }

    std::string id() const {
      std::ostringstream str;
      str << data["id"].asUInt();
      return str.str();
    };
    std::string name() const { return data["name"].asString(); }

  private:
    Reningsverk &r;
    Json::Value data;
};

#endif
