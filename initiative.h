#ifndef INITIATIVE_H
#define INITIATIVE_H

#include "entity.h"

#include "draft.h"
#include "suggestion.h"

#include <sstream>

class Issue;

class Initiative: public Entity {
  public:
    Initiative(Reningsverk &r, const Json::Value &data): Entity(r, data), currentDraftCache(nullptr), issueCache(nullptr), amSupporterCache(0) { }

    std::string issueId() const { return str(data["issue_id"]); }

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
    Draft *currentDraftCache;
    Issue *issueCache;
    int amSupporterCache;
};

#endif
