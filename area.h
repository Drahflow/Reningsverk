#ifndef AREA_H
#define AREA_H

#include "entity.h"

#include "policy.h"
#include "issue.h"

class Area: public Entity {
  public:
    Area(Reningsverk &r, const Json::Value &data): Entity(r, data), openIssueCacheValid(false) { }

    std::string name() const { return data["name"].asString(); }
    std::string description() const { return data["description"].asString(); }

    std::vector<Policy *> findAllowedPolicies() const;
    std::vector<Issue *> findOpenIssues() const;
    std::string defaultPolicyId() const;

    void createIssue(const Policy *, const std::string &name, const std::string &content);

    void cacheDefaultPolicyId(const std::string &id) { defaultPolicyIdCache = id; }
    // if called, must be called for all open issues
    void cacheOpenIssue(Issue *);
    void flushCacheOpenIssue();

  private:
    std::string defaultPolicyIdCache;
    bool openIssueCacheValid;
    std::vector<Issue *> openIssueCache;
};

#endif
