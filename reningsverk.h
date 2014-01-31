#ifndef RENINGSVERK_H
#define RENINGSVERK_H

#include "issue.h"
#include "area.h"
#include "localstore.h"

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
#include <sqlite3.h>

class Reningsverk {
  public:
    Reningsverk(const std::string &key, const std::string &user, const std::string &password);

    std::string getInfo();
    std::vector<Area *> findAreas();
    std::vector<Issue *> findIssues(const IssueState &);
    std::vector<Issue *> findOpenIssues(const Area &);
    std::vector<Policy *> findAllowedPolicies(const Area &);
    Issue *findIssue(const Initiative &);
    std::vector<Initiative *> findInitiatives(const Issue &);
    Draft *findCurrentDraft(const Initiative &);
    std::vector<Draft *> findDrafts(const Initiative &);
    std::vector<Suggestion *> findSuggestions(const Initiative &);
    bool amSupporter(const Initiative &);
    bool haveVoted(const Issue &);
    std::string defaultPolicyId(const Area &);

    void support(const Initiative &, bool yes);
    void createSuggestion(const Initiative &, const std::string &name, const std::string &content);
    void createInitiative(const Issue &, const std::string &name, const std::string &content);
    void createIssue(const Area &, const Policy &policy, const std::string &name, const std::string &content);
    void setOpinionDegree(const Suggestion &, int degree);
    void setOpinionFulfilment(const Suggestion &, bool fulfilled);
    void resetOpinion(const Suggestion &);
    void castVote(const Issue &, const std::map<Initiative *, int> &ballot);

    std::string getLocal(const std::string &key) { return localData->get(key); }
    void setLocal(const std::string &key, const std::string &value) { return localData->set(key, value); }

    static std::string str(int i);
    static std::string str(const Json::Value &v);

  private:
    bool DUMP = true;

    std::unique_ptr<Poco::Net::HTTPSClientSession> httpSession;
    std::map<std::string, std::string> httpCookies;
    std::string sessionKey;
    std::string csrfToken;
    std::unique_ptr<LocalStore> localData;

    enum Method {
      GET, POST
    };

    std::istream &http(const Method &, const std::string &path,
        const std::map<std::string, Json::Value> &data);
    Json::Value lqfb(const Method &, const std::string &path,
        const std::map<std::string, Json::Value> &data);

    std::map<std::string, std::unique_ptr<Area>> areaCache;
    std::map<std::string, std::unique_ptr<Policy>> policyCache;
    std::map<std::string, std::unique_ptr<Issue>> issueCache;
    std::map<std::string, std::unique_ptr<Initiative>> initiativeCache;
    std::map<std::string, std::unique_ptr<Draft>> draftCache;
    std::map<std::string, std::unique_ptr<Suggestion>> suggestionCache;

    void precacheIssues(const std::vector<Issue *> &);

    template<typename T, typename J> T *encache(std::map<std::string, std::unique_ptr<T>> &cache, const J &j) {
      const std::string idStr = str(j["id"]);
      if(cache.find(idStr) == cache.end()) cache[idStr] = std::unique_ptr<T>(new T(*this, j));
      return cache[idStr].get();
    }

    template<typename T, typename J> T *encache(std::map<std::string, std::unique_ptr<T>> &cache, const J &j, std::vector<T *> &created) {
      const std::string idStr = str(j["id"]);
      if(cache.find(idStr) == cache.end()) {
        cache[idStr] = std::unique_ptr<T>(new T(*this, j));
        created.push_back(cache[idStr].get());
      }
      return cache[idStr].get();
    }
};

#endif
