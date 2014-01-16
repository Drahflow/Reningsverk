#ifndef INITIATIVE_H
#define INITIATIVE_H

#include "draft.h"
#include "suggestion.h"

#include <sstream>

#include <jsoncpp/json/json.h>

class Reningsverk;
class Issue;

class Initiative {
  public:
    Initiative(Reningsverk &r, const Json::Value &data): r(r), data(data), currentDraftCache(nullptr), issueCache(nullptr) { }

    std::string id() const {
      std::ostringstream str;
      str << data["id"].asUInt();
      return str.str();
    };
    std::string issueId() const {
      std::ostringstream str;
      str << data["issue_id"].asUInt();
      return str.str();
    };

    std::string name() const { return data["name"].asString(); }
    Draft *currentDraft() const;
    Issue *findIssue() const;
    std::vector<Suggestion *> findSuggestions() const;

    void support(bool yes) const;

    void cacheCurrentDraft(Draft *d) { currentDraftCache = d; }
    void cacheIssue(Issue *i) { issueCache = i; }

  private:
    Reningsverk &r;
    Json::Value data;

    Draft *currentDraftCache;
    Issue *issueCache;
};

#endif
