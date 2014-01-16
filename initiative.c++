#include "initiative.h"

#include "reningsverk.h"

Draft *Initiative::currentDraft() const {
  if(currentDraftCache) return currentDraftCache;
  return r.findCurrentDraft(*this);
}

std::vector<Suggestion *> Initiative::findSuggestions() const {
  return r.findSuggestions(*this);
}

Issue *Initiative::findIssue() const {
  if(issueCache) return issueCache;
  return r.findIssue(*this);
}

void Initiative::support(bool yes) const {
  r.support(*this, yes);
}
