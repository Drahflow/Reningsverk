#ifndef SUGGESTION_H
#define SUGGESTION_H

#include "entity.h"

class Suggestion: public Entity {
  public:
    Suggestion(Reningsverk &r, const Json::Value &data): Entity(r, data) { }

    std::string initiativeId() const { return str(data["initiative_id"]); };
    std::string name() const { return data["name"].asString(); }
    std::string content() const { return data["content"].asString(); }

    enum Opinion { MUST_NOT, SHOULD_NOT, SHOULD, MUST };
    enum Fulfillment { FULFILLED, UNFULFILLED };

    void setOpinion(const Opinion &o);
    void setOpinion(const Fulfillment &f);
    void resetOpinion();

    bool seen() const;
    void setSeen(bool yes) const;
};

#endif
