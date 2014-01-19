#include "draft.h"

#include "reningsverk.h"

bool Draft::seen() const {
  return r.getLocal("draft." + id() + ".seen") == "true";
}

void Draft::setSeen(bool seen) const {
  r.setLocal("draft." + id() + ".seen", seen? "true": "false");
}
