#ifndef ISSUE_H
#define ISSUE_H

#include "initiative.h"

#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include <stdexcept>

#include <jsoncpp/json/json.h>

enum class IssueState {
  ADMISSION,
  DISCUSSION,
  VERIFICATION,
  VOTING,
  CANCELED,
  CALCULATION,
  FINISHED
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

    // if called, must be called for all initiatives
    void cacheInitiative(Initiative *);

  private:
    Reningsverk &r;
    Json::Value data;

    bool initiativeCacheValid;
    std::vector<Initiative *> initiativeCache;
};

#endif
