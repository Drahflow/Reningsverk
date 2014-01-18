#include "issue.h"

#include "reningsverk.h"

using namespace std;

void Issue::cacheInitiative(Initiative *i) {
  initiativeCache.push_back(i);
  initiativeCacheValid = true;
}

void Issue::flushCacheInitative() {
  initiativeCache.clear();
  initiativeCacheValid = false;
}

vector<Initiative *> Issue::findInitiatives() const {
  if(initiativeCacheValid) return initiativeCache;
  return r.findInitiatives(*this);
}

void Issue::createInitiative(const std::string &name, const std::string &content) const {
  return r.createInitiative(*this, name, content);
}
