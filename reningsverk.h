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
    std::vector<Initiative *> findInitiatives(const Issue &);
    Draft *findCurrentDraft(const Initiative &);

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
};

#endif
