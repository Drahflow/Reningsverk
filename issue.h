#ifndef ISSUE_H
#define ISSUE_H

#include "initiative.h"

#include "entity.h"

#include <vector>
#include <memory>
#include <stdexcept>

enum class IssueState {
  OPEN, // pseude-state used for selection only
  ADMISSION,
  DISCUSSION,
  VERIFICATION,
  VOTING,
  CANCELED,
  CALCULATION,
  FINISHED
};

class Initiative;

class Issue: public Entity {
  public:
    Issue(Reningsverk &r, const Json::Value &data): Entity(r, data), initiativeCacheValid(false) { }

    std::vector<Initiative *> findInitiatives() const;

    std::string areaId() const { return str(data["area_id"]); }

    IssueState state() const {
      if(data["state"] == "admission") return IssueState::ADMISSION;
      if(data["state"] == "discussion") return IssueState::DISCUSSION;
      if(data["state"] == "verification") return IssueState::VERIFICATION;
      if(data["state"] == "voting") return IssueState::VOTING;
      if(data["state"] == "canceled_revoked_before_accepted") return IssueState::CANCELED;
      if(data["state"] == "canceled_issue_not_accepted") return IssueState::CANCELED;
      if(data["state"] == "canceled_after_revocation_during_discussion") return IssueState::CANCELED;
      if(data["state"] == "canceled_after_revocation_during_verification") return IssueState::CANCELED;
      if(data["state"] == "calculation") return IssueState::CALCULATION;
      if(data["state"] == "canceled_no_initiative_admitted") return IssueState::CANCELED;
      if(data["state"] == "finished_without_winner") return IssueState::FINISHED;
      if(data["state"] == "finished_with_winner") return IssueState::FINISHED;
      throw std::runtime_error("invalid issue state");
    }

    void createInitiative(const std::string &name, const std::string &title) const;

    // if called, must be called for all initiatives
    void cacheInitiative(Initiative *);
    void flushCacheInitative();

  private:
    bool initiativeCacheValid;
    std::vector<Initiative *> initiativeCache;
};

#endif
