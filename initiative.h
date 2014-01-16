#ifndef INITIATIVE_H
#define INITIATIVE_H

#include "draft.h"

#include <sstream>

#include <jsoncpp/json/json.h>

class Reningsverk;

class Initiative {
  public:
    Initiative(Reningsverk &r, const Json::Value &data): r(r), data(data), currentDraftCache(nullptr) { }

    std::string id() const {
      std::ostringstream str;
      str << data["id"].asUInt();
      return str.str();
    };

    std::string name() const { return data["name"].asString(); }
    Draft *currentDraft();

    void cacheCurrentDraft(Draft *d) { currentDraftCache = d; }

  private:
    Reningsverk &r;
    Json::Value data;

    Draft *currentDraftCache;
};

#endif
