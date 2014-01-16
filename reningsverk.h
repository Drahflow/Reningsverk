#ifndef RENINGSVERK_H
#define RENINGSVERK_H

#include "issue.h"

#include <string>
#include <vector>
#include <memory>

#include <Poco/Net/Net.h>
#include <Poco/Net/NetSSL.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/Context.h>
#include <Poco/StreamCopier.h>
#include <jsoncpp/json/json.h>

class Reningsverk {
  public:
    Reningsverk(const std::string &key);
    Reningsverk(const char *key): Reningsverk(std::string(key)) { }

    Reningsverk(Reningsverk &&r): key(r.key), httpSession(std::move(r.httpSession)),
      sessionKey(r.sessionKey) { }

    std::string getInfo();
    std::vector<Issue *> findIssues(const IssueState &);
    Issue *findIssue(const Initiative &);
    std::vector<Initiative *> findInitiatives(const Issue &);
    Draft *findCurrentDraft(const Initiative &);
    std::vector<Suggestion *> findSuggestions(const Initiative &);

    void support(const Initiative &, bool yes);
    void setOpinionDegree(const Suggestion &, int degree);
    void setOpinionFulfilment(const Suggestion &, bool fulfilled);
    void resetOpinion(const Suggestion &);

  private:
    constexpr static bool DUMP = true;

    const std::string key;
    std::unique_ptr<Poco::Net::HTTPSClientSession> httpSession;

    Json::Value sessionKey;

    void fetchSessionKey();

    enum Method {
      GET, POST
    };

    Json::Value lqfb(const Method &, const std::string &path,
        const std::map<std::string, Json::Value> &data, bool includeSessionKey = true);

    std::string str(int i);
    std::string str(const Json::Value &v);

    std::map<std::string, std::unique_ptr<Issue>> issueCache;
    std::map<std::string, std::unique_ptr<Initiative>> initiativeCache;
    std::map<std::string, std::unique_ptr<Draft>> draftCache;
    std::map<std::string, std::unique_ptr<Suggestion>> suggestionCache;

    template<typename T, typename J> T *encache(std::map<std::string, std::unique_ptr<T>> &cache, const J &j) {
      const std::string idStr = str(j["id"]);
      if(cache.find(idStr) == cache.end()) cache[idStr] = std::unique_ptr<T>(new T(*this, j));
      return cache[idStr].get();
    }
};

#endif
