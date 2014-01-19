#include "area.h"

#include "reningsverk.h"

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
