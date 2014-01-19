#include "initiative.h"

#include "reningsverk.h"

Draft *Initiative::currentDraft() const {
  if(currentDraftCache) return currentDraftCache;
  return r.findCurrentDraft(*this);
}

std::vector<Draft *> Initiative::findDrafts() const {
  return r.findDrafts(*this);
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

void Initiative::createSuggestion(const std::string &name, const std::string &content) const {
  return r.createSuggestion(*this, name, content);
}

bool Initiative::amSupporter() const {
  if(amSupporterCache) return amSupporterCache == 1;
  return r.amSupporter(*this);
}

std::string Initiative::note() const {
  return r.getLocal("ini." + id() + ".note");
}

void Initiative::setNote(const std::string &note) const {
  r.setLocal("ini." + id() + ".note", note);
}
