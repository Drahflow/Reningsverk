#include "area.h"

#include "reningsverk.h"

using namespace std;

std::vector<Policy *> Area::findAllowedPolicies() const {
  return r.findAllowedPolicies(*this);
}

std::string Area::defaultPolicyId() const {
  if(defaultPolicyIdCache != "") return defaultPolicyIdCache;
  return r.defaultPolicyId(*this);
}

void Area::createIssue(const Policy *p, const std::string &name, const std::string &content) {
  r.createIssue(*this, *p, name, content);
}

void Area::cacheOpenIssue(Issue *i) {
  openIssueCache.push_back(i);
  openIssueCacheValid = true;
}

void Area::flushCacheOpenIssue() {
  openIssueCache.clear();
  openIssueCacheValid = false;
}

vector<Issue *> Area::findOpenIssues() const {
  if(openIssueCacheValid) return openIssueCache;
  return r.findOpenIssues(*this);
}
