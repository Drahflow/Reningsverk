#include "reningsverk.h"

#include <iostream>
#include <sstream>
#include <set>

#include <Poco/Net/HTMLForm.h>

using namespace std;
using namespace Poco::Net;
using namespace Poco;

const std::string HOST = "lqfb.piratenpartei.de";
const std::string API_BASE = "/api";

// FIXME: hardcode certificate instead
Reningsverk::Reningsverk(const std::string &key): key{key},
  httpSession{new HTTPSClientSession{HOST, HTTPSClientSession::HTTPS_PORT,
      new Context{Context::CLIENT_USE, "", "", "", Context::VERIFY_NONE, 9, true}}} {

  fetchSessionKey();
}

void Reningsverk::fetchSessionKey() {
  sessionKey = lqfb(POST, "/session", {{"key", key}}, false)["session_key"];
}

Json::Value Reningsverk::lqfb(const Method &method, const std::string &path,
    const std::map<std::string, Json::Value> &data, bool includeSessionKey) {

  auto httpMethod = HTTPRequest::HTTP_POST;
  switch(method) {
    case GET:
      httpMethod = HTTPRequest::HTTP_GET;
      if(DUMP) cout << "GET " + path << endl;
      break;
    case POST:
      httpMethod = HTTPRequest::HTTP_POST;
      if(DUMP) cout << "POST " + path << endl;
      break;
    default: throw std::logic_error("invalid http method specified");
  }

  HTTPRequest request(httpMethod, API_BASE + path);
  HTMLForm form;
  for(const auto &i: data) form.add(i.first, i.second.asString());
  if(includeSessionKey) form.add("session_key", sessionKey.asString());
  form.prepareSubmit(request);
  std::ostream &reqStream = httpSession->sendRequest(request);
  form.write(reqStream);
  if(DUMP) {
    form.write(cout);
    cout << endl;
  }

  HTTPResponse response;
  std::istream& resStream = httpSession->receiveResponse(response);
  Json::Value ret;
  Json::Reader().parse(resStream, ret);
  if(DUMP) cout << ret.toStyledString() << endl;
  return ret;
}

std::string Reningsverk::getInfo() {
  return lqfb(GET, "/info", {}).toStyledString();
}

vector<Issue *> Reningsverk::findIssues(const IssueState &state) {
  std::string strState;
  switch(state) {
    case IssueState::DISCUSSION: strState = "discussion"; break;
    default: throw std::logic_error("invalid issue state specified");
  }

  ostringstream newIssues;
  bool any = false;

  vector<Issue *> ret;
  auto v = lqfb(GET, "/issue", {{"issue_state", strState}})["result"];
  for(auto &i: v) {
    if(issueCache.find(str(i["id"])) == issueCache.end()) {
      // issueCache.emplace({str(i["id"]), {new Issue(*this, i)});
      issueCache[str(i["id"])] = unique_ptr<Issue>(new Issue(*this, i));

      if(any) newIssues << ",";
      any = true;
      newIssues << str(i["id"]);
    }

    ret.push_back(issueCache[str(i["id"])].get());
  }

  if(any) {
    auto v2 = lqfb(GET, "/initiative", {{"issue_id", newIssues.str()}})["result"];
    for(auto &i: v2) {
      if(initiativeCache.find(str(i["id"])) == initiativeCache.end()) {
        initiativeCache[str(i["id"])] = unique_ptr<Initiative>(new Initiative(*this, i));
      }

      cout << i << endl;
      if(issueCache.find(str(i["issue_id"])) == issueCache.end()) throw std::runtime_error("Server returned data which was not requested");
      issueCache[str(i["issue_id"])]->cacheInitiative(initiativeCache[str(i["id"])].get());
    }
  }

  return ret;
}

vector<Initiative *> Reningsverk::findInitiatives(const Issue &) {
  vector<Initiative *> ret;
  throw std::logic_error("FIXME TODO");
//   auto v = lqfb(GET, "/initiative", {{"issue_id", issue.id()}})["result"];
//   for(auto &i: v) ret.push_back(Initiative(*this, i));
//  return ret;
}

Draft *Reningsverk::findCurrentDraft(const Initiative &i) {
  auto v = lqfb(GET, "/draft", {{"initiative_id", i.id()}, {"current_draft", "1"}})["result"];
  if(v.size() != 1) throw std::runtime_error("Initiative has not exactly one current draft");

  for(auto &i: v) {
    if(draftCache.find(str(i["id"])) == draftCache.end()) {
      draftCache[str(i["id"])] = unique_ptr<Draft>(new Draft(*this, i));
    }

    cout << i << endl;
    if(initiativeCache.find(str(i["initiative_id"])) == initiativeCache.end()) throw std::runtime_error("Server returned data which was not requested");
    initiativeCache[str(i["initiative_id"])]->cacheCurrentDraft(draftCache[str(i["id"])].get());

    return draftCache[str(i["id"])].get();
  }

  throw std::logic_error("One-sized vector was not iterated");
}

std::string Reningsverk::str(int i) {
  ostringstream str;
  str << i;
  return str.str();
}

std::string Reningsverk::str(const Json::Value &v) {
  switch(v.type()) {
    case Json::intValue: return str(v.asInt());
    case Json::uintValue: return str(v.asUInt());
    case Json::stringValue: return v.asString();
    default: throw std::runtime_error("Invalid type for stringification");
  }
}
