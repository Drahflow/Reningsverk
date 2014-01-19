#ifndef AREA_H
#define AREA_H

#include "policy.h"

#include <sstream>

#include <jsoncpp/json/json.h>

class Reningsverk;

class Area {
  public:
    Area(Reningsverk &r, const Json::Value &data): r(r), data(data) { }

    std::string id() const {
      std::ostringstream str;
      str << data["id"].asUInt();
      return str.str();
    };
    std::string name() const { return data["name"].asString(); }
    std::string description() const { return data["description"].asString(); }

    std::vector<Policy *> findAllowedPolicies() const;
    std::string defaultPolicyId() const;

    void createIssue(const Policy *, const std::string &name, const std::string &content);

    void cacheDefaultPolicyId(const std::string &id) { defaultPolicyIdCache = id; }

  private:
    Reningsverk &r;
    Json::Value data;

    std::string defaultPolicyIdCache;
};

#endif
