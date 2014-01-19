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
    Initiative(Reningsverk &r, const Json::Value &data): r(r), data(data), currentDraftCache(nullptr), issueCache(nullptr), amSupporterCache(0) { }

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
    int supporterCount() const {
      return data["supporter_count"].asUInt();
    }
    int satisfiedSupporterCount() const {
      return data["satisfied_supporter_count"].asUInt();
    }
    bool isRevoked() const {
      return !data["revoked"].isNull();
    }
    std::string note() const;

    void setNote(const std::string &note) const;
    bool amSupporter() const;
    std::string name() const { return data["name"].asString(); }
    Draft *currentDraft() const;
    std::vector<Draft *> findDrafts() const;
    Issue *findIssue() const;
    std::vector<Suggestion *> findSuggestions() const;

    void createSuggestion(const std::string &name, const std::string &content) const;
    void support(bool yes) const;

    void cacheCurrentDraft(Draft *d) { currentDraftCache = d; }
    void cacheIssue(Issue *i) { issueCache = i; }
    void cacheAmSupporter(bool supporting) { amSupporterCache = supporting? 1: -1; }
    void flushCacheAmSupporter() { amSupporterCache = 0; }

  private:
    Reningsverk &r;
    Json::Value data;

    Draft *currentDraftCache;
    Issue *issueCache;
    int amSupporterCache;
};

#endif
