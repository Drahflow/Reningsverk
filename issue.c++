#include "issue.h"

#include "reningsverk.h"

using namespace std;

void Issue::cacheInitiative(Initiative *i) {
  initiativeCache.push_back(i);
  initiativeCacheValid = true;
}

vector<Initiative *> Issue::findInitiatives() const {
  if(initiativeCacheValid) return initiativeCache;
  return r.findInitiatives(*this);
}
