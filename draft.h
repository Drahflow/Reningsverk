#ifndef DRAFT_H
#define DRAFT_H

#include "entity.h"

class Draft: public Entity {
  public:
    Draft(Reningsverk &r, const Json::Value &data): Entity(r, data) { }

    std::string content() const { return data["content"].asString(); }
    std::string created() const { return data["created"].asString(); }

    bool seen() const;
    void setSeen(bool yes) const;
};

#endif
