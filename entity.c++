#include "entity.h"

#include "reningsverk.h"

std::string Entity::str(const Json::Value &v) const {
  return r.str(v);
}
