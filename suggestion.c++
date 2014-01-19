#include "suggestion.h"

#include "reningsverk.h"

#include <exception>

using namespace std;

void Suggestion::setOpinion(const Opinion &o) {
  switch(o) {
    case MUST_NOT: r.setOpinionDegree(*this, -2); break;
    case SHOULD_NOT: r.setOpinionDegree(*this, -1); break;
    case SHOULD: r.setOpinionDegree(*this, 1); break;
    case MUST: r.setOpinionDegree(*this, 2); break;
    default: throw logic_error("invalid opinion set");
  }
}

void Suggestion::setOpinion(const Fulfillment &f) {
  switch(f) {
    case FULFILLED: r.setOpinionFulfilment(*this, true); break;
    case UNFULFILLED: r.setOpinionFulfilment(*this, false); break;
    default: throw logic_error("invalid fulfilment set");
  }
}

void Suggestion::resetOpinion() {
  r.resetOpinion(*this);
}

bool Suggestion::seen() const {
  return r.getLocal("suggestion." + id() + ".seen") == "true";
}

void Suggestion::setSeen(bool seen) const {
  r.setLocal("suggestion." + id() + ".seen", seen? "true": "false");
}
