#ifndef ENTITY_H
#define ENTITY_H

class Reningsverk;

#include <string>

#include <jsoncpp/json/json.h>

// A generic LQFB database entity
class Entity {
  public:
    Entity(Reningsverk &r, const Json::Value &data): r(r), data(data) { }

    std::string id() const { return str(data["id"]); }

  protected:
    std::string str(const Json::Value &v) const;

    Reningsverk &r;
    Json::Value data;
};

#endif
