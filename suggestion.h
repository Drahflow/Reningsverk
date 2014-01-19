#ifndef SUGGESTION_H
#define SUGGESTION_H

#include <sstream>

#include <jsoncpp/json/json.h>

class Reningsverk;

class Suggestion {
  public:
    Suggestion(Reningsverk &r, const Json::Value &data): r(r), data(data) { }

    std::string id() const {
      std::ostringstream str;
      str << data["id"].asUInt();
      return str.str();
    };
    std::string initiativeId() const {
      std::ostringstream str;
      str << data["initiative_id"].asUInt();
      return str.str();
    };

    std::string name() const { return data["name"].asString(); }
    std::string content() const { return data["content"].asString(); }

    enum Opinion { MUST_NOT, SHOULD_NOT, SHOULD, MUST };
    enum Fulfillment { FULFILLED, UNFULFILLED };

    void setOpinion(const Opinion &o);
    void setOpinion(const Fulfillment &f);
    void resetOpinion();

    bool seen() const;
    void setSeen(bool yes) const;

  private:
    Reningsverk &r;
    Json::Value data;
};

#endif
