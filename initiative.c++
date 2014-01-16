#include "initiative.h"

#include "reningsverk.h"

Draft *Initiative::currentDraft() {
  if(currentDraftCache) return currentDraftCache;
  return r.findCurrentDraft(*this);
}
