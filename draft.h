#ifndef DRAFT_H
#define DRAFT_H

#include <jsoncpp/json/json.h>

class Reningsverk;

class Draft {
  public:
    Draft(Reningsverk &r, const Json::Value &data): r(r), data(data) { }

    std::string content() const { return data["content"].asString(); }

  private:
    Reningsverk &r;
    Json::Value data;
};

#endif
