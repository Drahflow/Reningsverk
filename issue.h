#ifndef ISSUE_H
#define ISSUE_H

#include "initiative.h"

#include <string>
#include <vector>
#include <sstream>
#include <memory>

#include <jsoncpp/json/json.h>

enum class IssueState {
  DISCUSSION
};

class Reningsverk;
class Initiative;

class Issue {
  public:
    Issue(Reningsverk &r, const Json::Value &data): r(r), data(data), initiativeCacheValid(false) { }

    std::vector<Initiative *> findInitiatives() const;

    std::string id() const {
      std::ostringstream str;
      str << data["id"].asUInt();
      return str.str();
    };

    // if called, must be called for all initiatives
    void cacheInitiative(Initiative *);

  private:
    Reningsverk &r;
    Json::Value data;

    bool initiativeCacheValid;
    std::vector<Initiative *> initiativeCache;
};

#endif
