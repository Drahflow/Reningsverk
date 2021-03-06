#include "reningsverk.h"

#include <iostream>
#include <sstream>
#include <set>
#include <list>
#include <iomanip>

#include <Poco/Net/HTMLForm.h>

#include <openssl/ssl.h>

using namespace std;
using namespace Poco::Net;
using namespace Poco;

const std::string HOST = "lqfb.piratenpartei.de";
const std::string WEB = "/lf";
const std::string API = "/api";
const std::set<std::string> validCerts {
  "02:fa:f3:e2:91:43:54:68:60:78:57:69:4d:f5:e4:5b:68:85:18:68",
  "94:80:7b:1c:78:8d:d2:fc:be:19:c8:48:1c:e4:1c:fa:b8:a4:c1:7f",
  "92:e5:ed:69:d9:d0:68:74:51:5c:9e:e7:b4:63:ed:c9:d8:c8:b8:03",
};

// TODO: all HTML "parsing" would better be done with a regex

// Certificate authorities are not trustworthy. #NSATimes
int checkFixedCertificate(int, X509_STORE_CTX *ctx) {
  X509 *cert = X509_STORE_CTX_get_current_cert(ctx);
  
  ostringstream hash;
  for(size_t i = 0; i < SHA_DIGEST_LENGTH; ++i) {
    if(i) hash << ":";
    hash << hex << setw(2) << setfill('0') << static_cast<unsigned int>(cert->sha1_hash[i]);
  }
  
  const int valid = validCerts.count(hash.str());
  if(!valid) {
    cout << cert->name << endl;
    cout << "Certificate fingerprint (sha1): " << hash.str() << endl;
    throw std::runtime_error("fingerprint not in hardcoded set - refusing to connect");
  }
  return valid;
}

Reningsverk::Reningsverk(const string &key, const string &user, const string &password) {
  Poco::Net::Context::Ptr context =
      new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "","", Poco::Net::Context::VERIFY_RELAXED, 9, false);
  httpSession = unique_ptr<HTTPSClientSession>(new HTTPSClientSession(HOST, HTTPSClientSession::HTTPS_PORT, context));

  // Poco does not support checking each certificate in a chain
  SSL_CTX_set_verify(context->sslContext(), SSL_VERIFY_PEER, checkFixedCertificate);

  if(!getenv("HOME")) throw runtime_error("no $HOME environment variable");
  localData = unique_ptr<LocalStore>(new LocalStore(getenv("HOME") + std::string("/.reningsverk")));

  string loginString;
  istream &loginStream = http(GET, WEB + "/index/login.html", {});
  list<string> tokens;

  while(loginStream) {
    string tok;
    loginStream >> tok;
    tokens.push_back(tok);
    if(tokens.size() > 3) tokens.pop_front();

    if(tok == "name=\"_webmcp_csrf_secret\"") csrfToken = tokens.front();
  }
  if(csrfToken.length() != 40) throw std::runtime_error("could not find CSRF token");
  csrfToken = csrfToken.substr(7, 32);
  
  cout << "CSRF-Token: " << csrfToken << endl;

  istream &loggedIn = http(POST, WEB + "/index/login", {
      {"password", password},
      {"login", user},
      {"_webmcp_csrf_secret", csrfToken},
      {"_webmcp_routing.ok.view", "index"},
      {"_webmcp_routing.ok.module", "index"},
      {"_webmcp_routing.ok.mode",  "redirect"},
      {"_webmcp_routing.error.view", "login"},
      {"_webmcp_routing.error.module", "index"},
      {"_webmcp_routing.error.mode", "forward"},
      {"_webmcp_routing.default.view", "login"},
      {"_webmcp_routing.default.module", "index"},
      {"_webmcp_routing.default.mode", "forward"},
      });

  StreamCopier().copyStream(loggedIn, cout);

  sessionKey = lqfb(POST, API + "/session", {{"key", key}})["session_key"].asString();
}

std::istream &Reningsverk::http(const Method &method, const std::string &path,
    const std::map<std::string, Json::Value> &data) {

  auto httpMethod = HTTPRequest::HTTP_POST;
  switch(method) {
    case GET: httpMethod = HTTPRequest::HTTP_GET; break;
    case POST: httpMethod = HTTPRequest::HTTP_POST; break;
    default: throw std::logic_error("invalid http method specified");
  }

  HTTPRequest request(httpMethod, path);
  {
    NameValueCollection cookies;
    for(auto &i: httpCookies) cookies.set(i.first, i.second);
    request.setCookies(cookies);
  }
  HTMLForm form;
  for(const auto &i: data) form.add(i.first, i.second.asString());
  if(sessionKey != "") form.add("session_key", sessionKey);
  form.prepareSubmit(request);
  if(DUMP) {
    request.write(cout);
  }
  std::ostream &reqStream = httpSession->sendRequest(request);
  form.write(reqStream);
  if(DUMP) {
    form.write(cout);
    cout << endl;
  }

  HTTPResponse response;
  istream &resStream = httpSession->receiveResponse(response);
  if(DUMP) cout << response.getStatus() << " " << response.getReason() << endl;
  if(DUMP) for(auto &i: response) cout << i.first << ": " << i.second << endl;

  {
    vector<HTTPCookie> cookies; response.getCookies(cookies);
    for(auto &i: cookies) httpCookies[i.getName()] = i.getValue();
    if(DUMP) for(auto &i: cookies) cout << i.getName() << ": " << i.getValue() << endl;
  }

  if(DUMP) {
    cout << "Cookies now: " << endl;
    for(auto &i: httpCookies) cout << i.first << ": " << i.second << endl;
  }

  return resStream;
}

Json::Value Reningsverk::lqfb(const Method &method, const std::string &path,
    const std::map<std::string, Json::Value> &data) {

  istream &resStream = http(method, path, data);
  Json::Value ret;
  Json::Reader().parse(resStream, ret);
  if(DUMP) cout << ret.toStyledString() << endl;
  return ret;
}

std::string Reningsverk::getInfo() {
  return lqfb(GET, API + "/info", {}).toStyledString();
}

vector<Area *> Reningsverk::findAreas() {
  vector<Area *> ret;
  auto v = lqfb(GET, API + "/area", {})["result"];
  for(auto &i: v) ret.push_back(encache(areaCache, i));

  return ret;
}

vector<Policy *> Reningsverk::findAllowedPolicies(const Area &a) {
  vector<Json::Value> ids;
  {
    auto v = lqfb(GET, API + "/allowed_policy", {{"area_id", a.id()}})["result"];
    transform(v.begin(), v.end(), back_inserter(ids), [&](Json::Value &p) { return p["policy_id"]; });
  }

  bool any = false;
  ostringstream idString;
  for(auto &i: ids) {
    if(any) idString << ",";
    any = true;
    idString << i.asInt();
  }

  vector<Policy *> ret;
  auto v = lqfb(GET, API + "/policy", {{"policy_id", idString.str()}})["result"];
  for(auto &i: v) ret.push_back(encache(policyCache, i));

  return ret;
}

void Reningsverk::precacheIssues(const vector<Issue *> &issues) {
  if(issues.empty()) return;

  ostringstream issueIds;
  bool any = false;

  for(auto &i: issues) {
    if(any) issueIds << ",";
    any = true;
    issueIds << i->id();
  }

  auto v2 = lqfb(GET, API + "/initiative", {{"issue_id", issueIds.str()}})["result"];
  std::vector<Initiative *> loadedInis;
  for(auto &i: v2) {
    Initiative *ini = encache(initiativeCache, i, loadedInis);

    cout << i << endl;
    if(issueCache.find(str(i["issue_id"])) == issueCache.end()) throw std::runtime_error("Server returned data which was not requested");
    issueCache[str(i["issue_id"])]->cacheInitiative(initiativeCache[str(i["id"])].get());
    ini->cacheIssue(issueCache[str(i["issue_id"])].get());
  }

  if(loadedInis.empty()) return;

  any = false;
  ostringstream iniIds;
  for(auto &i: loadedInis) {
    if(any) iniIds << ",";
    any = true;
    iniIds << i->id();
  }

  auto v3 = lqfb(GET, API + "/draft", {{"initiative_id", iniIds.str()}, {"current_draft", "1"}})["result"];
  if(v3.size() != loadedInis.size()) throw runtime_error("no 1-1 correspondence between initiatives and current drafts");
  for(auto &i: v3) {
    encache(draftCache, i);

    if(initiativeCache.find(str(i["initiative_id"])) == initiativeCache.end()) throw std::runtime_error("Server returned data which was not requested");
    initiativeCache[str(i["initiative_id"])]->cacheCurrentDraft(draftCache[str(i["id"])].get());
  }
}

vector<Issue *> Reningsverk::findIssues(const IssueState &state) {
  std::string strState;
  switch(state) {
    case IssueState::OPEN: strState = "open"; break;
    case IssueState::ADMISSION: strState = "admission"; break;
    case IssueState::DISCUSSION: strState = "discussion"; break;
    case IssueState::VOTING: strState = "voting"; break;
    default: throw std::logic_error("invalid issue state specified");
  }

  vector<Issue *> ret, newIssues;

  auto v = lqfb(GET, API + "/issue", {{"issue_state", strState}})["result"];
  for(auto &i: v) ret.push_back(encache(issueCache, i, newIssues));

  precacheIssues(newIssues);
  return ret;
}

vector<Issue *> Reningsverk::findOpenIssues(const Area &area) {
  areaCache[area.id()]->flushCacheOpenIssue();

  vector<Issue *> ret, newIssues;
  auto v = lqfb(GET, API + "/issue", {{"area_id", area.id()}, {"issue_state", "open"}})["result"];
  for(auto &i: v) ret.push_back(encache(issueCache, i, newIssues));

  precacheIssues(newIssues);
  return ret;
}

Issue *Reningsverk::findIssue(const Initiative &i) {
  auto v = lqfb(GET, API + "/issue", {{"issue_id", i.issueId()}})["result"];
  if(v.size() != 1) throw std::runtime_error("issue-by-id returned non-one-sized result");

  Issue *ret = encache(issueCache, v[0u]);
  initiativeCache[i.id()]->cacheIssue(ret);

  return ret;
}

vector<Initiative *> Reningsverk::findInitiatives(const Issue &i) {
  vector<Initiative *> ret;

  auto v = lqfb(GET, API + "/initiative", {{"issue_id", i.id()}})["result"];
  for(auto &i: v) {
    Initiative *ini = encache(initiativeCache, i);
    ret.push_back(ini);

    cout << i << endl;
    if(issueCache.find(str(i["issue_id"])) == issueCache.end()) throw std::runtime_error("Server returned data which was not requested");
    issueCache[str(i["issue_id"])]->cacheInitiative(initiativeCache[str(i["id"])].get());
    ini->cacheIssue(issueCache[str(i["issue_id"])].get());
  }
  
  return ret;
}

Draft *Reningsverk::findCurrentDraft(const Initiative &i) {
  auto v = lqfb(GET, API + "/draft", {{"initiative_id", i.id()}, {"current_draft", "1"}})["result"];
  if(v.size() != 1) throw std::runtime_error("Initiative has not exactly one current draft");

  for(auto &i: v) {
    encache(draftCache, i);

    if(initiativeCache.find(str(i["initiative_id"])) == initiativeCache.end()) throw std::runtime_error("Server returned data which was not requested");
    initiativeCache[str(i["initiative_id"])]->cacheCurrentDraft(draftCache[str(i["id"])].get());

    return draftCache[str(i["id"])].get();
  }

  throw std::logic_error("One-sized vector was not iterated");
}

std::vector<Draft *> Reningsverk::findDrafts(const Initiative &i) {
  std::vector<Draft *> ret;

  auto v = lqfb(GET, API + "/draft", {{"initiative_id", i.id()}})["result"];
  for(auto &i: v) ret.push_back(encache(draftCache, i));

  return ret;
}

vector<Suggestion *> Reningsverk::findSuggestions(const Initiative &i) {
  vector<Suggestion *> ret;

  auto v = lqfb(GET, API + "/suggestion", {{"initiative_id", i.id()}})["result"];
  for(auto &i: v) ret.push_back(encache(suggestionCache, i));

  return ret;
}

void Reningsverk::setOpinionDegree(const Suggestion &s, int degree) {
  // lqfb(POST, API + "/opinion", {{"suggestion_id", s.id()}, {"degree", str(degree)}});
  lqfb(POST, WEB + "/opinion/update", {
      {"suggestion_id", s.id()},
      {"degree", str(degree)},
      {"_webmcp_csrf_secret", csrfToken},
      {"_webmcp_routing.default.view", "show"},
      {"_webmcp_routing.default.params.initiative_id", str(s.initiativeId())},
      {"_webmcp_routing.default.module", "suggestion"},
      {"_webmcp_routing.default.mode", "redirect"},
      {"_webmcp_routing.default.id", str(s.id())},
      });
}

void Reningsverk::setOpinionFulfilment(const Suggestion &s, bool fulfilled) {
  // lqfb(POST, API + "/opinion", {{"suggestion_id", s.id()}, {"fulfilled", fulfilled? "1": "0"}});
  lqfb(POST, WEB + "/opinion/update", {
      {"suggestion_id", s.id()},
      {"fulfilled", fulfilled? "true": "false"},
      {"_webmcp_csrf_secret", csrfToken},
      {"_webmcp_routing.default.view", "show"},
      {"_webmcp_routing.default.params.initiative_id", str(s.initiativeId())},
      {"_webmcp_routing.default.module", "suggestion"},
      {"_webmcp_routing.default.mode", "redirect"},
      {"_webmcp_routing.default.id", str(s.id())},
      });
}

void Reningsverk::resetOpinion(const Suggestion &s) {
  // lqfb(POST, API + "/opinion", {{"suggestion_id", s.id()}, {"delete", "1"}});
  lqfb(POST, WEB + "/opinion/update", {
      {"suggestion_id", s.id()},
      {"delete", "true"},
      {"_webmcp_csrf_secret", csrfToken},
      {"_webmcp_routing.default.view", "show"},
      {"_webmcp_routing.default.params.initiative_id", str(s.initiativeId())},
      {"_webmcp_routing.default.module", "suggestion"},
      {"_webmcp_routing.default.mode", "redirect"},
      {"_webmcp_routing.default.id", str(s.id())},
      });
}

void Reningsverk::support(const Initiative &i, bool yes) {
  // lqfb(POST, API + "/supporter", {{"initiative_id", i.id()}, {"draft_id", i.currentDraft()->id()}, {"delete", yes? "0": "1"}});
  lqfb(POST, WEB + "/initiative/" + (yes? "add_support": "remove_support"), {
      {"_webmcp_csrf_secret", csrfToken},
      {"_webmcp_routing.default.view", "show"},
      {"_webmcp_routing.default.params.tab", "suggestions"},
      {"_webmcp_routing.default.module", "initiative"},
      {"_webmcp_routing.default.mode", "redirect"},
      {"_webmcp_routing.default.id", i.id()},
      {"_webmcp_id", i.id()},
      });

  initiativeCache[i.id()]->flushCacheAmSupporter();
}

void Reningsverk::createSuggestion(const Initiative &i, const string &name, const string &content) {
  lqfb(POST, WEB + "/suggestion/add", {
      {"name", name},
      {"initiative_id", i.id()},
      {"formatting_engine", "rocketwiki"},
      {"degree", "1"},
      {"content", content},
      {"_webmcp_csrf_secret", csrfToken},
      {"_webmcp_routing.default.view", "show"},
      {"_webmcp_routing.default.params.tab", "suggestions"},
      {"_webmcp_routing.default.module", "initiative"},
      {"_webmcp_routing.default.mode", "redirect"},
      {"_webmcp_routing.default.id", i.id()},
      });
}

void Reningsverk::createInitiative(const Issue &i, const string &name, const string &content) {
  lqfb(POST, WEB + "/initiative/create", {
      {"name", name},
      {"issue_id", i.id()},
      {"formatting_engine", "rocketwiki"},
      {"draft", content},
      {"discussion_url", ""},
      {"area_id", i.areaId()},
      {"_webmcp_csrf_secret", csrfToken},
      {"_webmcp_routing.default.view", "new"},
      {"_webmcp_routing.default.module", "initiative"},
      {"_webmcp_routing.default.mode", "forward"},
      });

  issueCache[i.id()]->flushCacheInitative();
}

void Reningsverk::createIssue(const Area &a, const Policy &p, const string &name, const string &content) {
  lqfb(POST, WEB + "/initiative/create", {
      {"policy_id", p.id()},
      {"name", name},
      {"formatting_engine", "rocketwiki"},
      {"draft", content},
      {"discussion_url", ""},
      {"area_id", a.id()},
      {"_webmcp_csrf_secret", csrfToken},
      {"_webmcp_routing.default.view", "new"},
      {"_webmcp_routing.default.module", "initiative"},
      {"_webmcp_routing.default.mode", "forward"},
      });

  areaCache[a.id()]->flushCacheOpenIssue();
}

bool Reningsverk::amSupporter(const Initiative &i) {
  // bool supporting = lqfb(GET, API + "/supporter", {
  //     {"initiative_id", i.id()},
  //     {"unit_id", "1"}
  //     })["results"].size() > 0;
  
  for(auto &i: initiativeCache) i.second->cacheAmSupporter(false);

  istream &html = http(GET, WEB + "/index/index.html", {
      {"tab", "open"},
      {"filter_interest", "supported"},
      {"filter_delegation", "any"}
      });

  bool support = false;
  bool author = false;
  std::vector<std::string> authoredInis;

  while(html) {
    string tok;
    html >> tok;
    if(tok == "class=\"initiative\"><div") {
      support = false;
      author = false;
    }
    if(tok == "src=\"../static/icons/16/thumb_up_green.png\"") support = true;
    if(tok == "src=\"../static/icons/16/user_edit.png\"") author = true;
    if(tok.substr(0, 25) == "class=\"initiative_link\">i") {
      auto currentIni = tok.substr(25, tok.size() - 25 - 1);
      if(support) {
        auto cache = initiativeCache.find(currentIni);
        if(cache != initiativeCache.end()) cache->second->cacheAmSupporter(true);
      }
      if(author) {
        authoredInis.push_back(currentIni);
      }
    }
  }

  for(auto &i: authoredInis) {
    istream &html = http(GET, WEB + "/initiative/show/" + i + ".html", {});
    while(html) {
      string tok;
      html >> tok;
      if(tok == "src=\"../../static/icons/16/thumb_up_green.png\"") {
        auto cache = initiativeCache.find(i);
        if(cache != initiativeCache.end()) cache->second->cacheAmSupporter(true);
      }
    }
  }

  cout << i.id() << endl;
  return initiativeCache[i.id()]->amSupporter();
}

bool Reningsverk::haveVoted(const Issue &issue) {
  // the API seems badly lacking here
  
  for(auto &i: issueCache) i.second->cacheHaveVoted(false);

  istream &html = http(GET, WEB + "/index/index.html", {
      {"tab", "open"},
      {"filter", "frozen"},
      {"filter_voting", "voted"}
      });

  while(html) {
    string tok;
    html >> tok;
    if(tok.length() > 15 &&
        tok[0] == '#' &&
        tok.substr(tok.length() - 14, 14) == "</a></div><div") {
      auto votedIssue = tok.substr(1, tok.size() - 14 - 1);
      auto cache = issueCache.find(votedIssue);
      if(cache != issueCache.end()) cache->second->cacheHaveVoted(true);
    }
  }

  cout << issue.id() << endl;
  return issueCache[issue.id()]->haveVoted();
}

std::string Reningsverk::defaultPolicyId(const Area &a) {
  auto v = lqfb(GET, API + "/allowed_policy", {{"area_id", a.id()}})["result"];

  std::string defaultPolicy;

  for(auto &i: v) {
    if(i["default_policy"].asBool()) {
      if(defaultPolicy != "") throw runtime_error("multiple policies reported as default");
      defaultPolicy = str(i["policy_id"].asInt());
    }
  }

  if(defaultPolicy != "") areaCache[a.id()]->cacheDefaultPolicyId(defaultPolicy);

  return defaultPolicy;
}

void Reningsverk::castVote(const Issue &issue, const map<Initiative *, int> &ballot) {
  ostringstream ballotStr;

  for(auto &i: ballot) {
    ballotStr << i.first->id() << ":" << i.second << ";";
  }

  lqfb(POST, WEB + "/vote/update", {
      {"update_comment", ""},
      {"scoring", ballotStr.str()},
      {"issue_id", issue.id()},
      {"formatting_engine", "rocketwiki"},
      {"comment", ""},
      {"_webmcp_csrf_secret", csrfToken},
      {"_webmcp_routing.default.view", "list"},
      {"_webmcp_routing.default.module", "vote"},
      {"_webmcp_routing.default.mode", "forward"},
      });

  for(auto &i: issueCache) i.second->flushCacheHaveVoted();
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
