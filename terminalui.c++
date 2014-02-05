#include "terminalui.h"

#include "tempfile.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>
#include <cstdio>
#include <fstream>
#include <iomanip>

#include <stdlib.h>

using namespace std;

static void removeBackslashR(istream &i, ostream &o) {
  string line;
  while(getline(i, line)) {
    if(line.size() > 0 && line[line.size() - 1] == '\r') line = line.substr(0, line.size() - 1);
    o << line << endl;
  }
}

static void showDiff(const std::string &a, const std::string &b) {
  Tempfile aFile, bFile;

  istringstream aStream(a), bStream(b);
  ofstream aFileStream(aFile.name()), bFileStream(bFile.name());
  removeBackslashR(aStream, aFileStream);
  removeBackslashR(bStream, bFileStream);

  std::string cmd = std::string("${DIFF:-vimdiff} '") + aFile.name() + "' '" + bFile.name() + "'";
  system(cmd.c_str());
}

string TerminalUI::readline() {
  string ret;
  getline(cin, ret);
  return ret;
}

template<typename D> void TerminalUI::menu(const D &d) {
  bool run = true;
  bool help = false;

  while(run) {
    Choices c;
    c[""] = [&] { run = false; };
    c["."] = [&] { };
    c["help"] = [&] { help = true; };

    menuContent<D>(d, c);
    if(help) menuHelp<D>(d, c);

    help = false;
    handleChoice(menuPrompt<D>(), c);
  }
}

template<> void TerminalUI::menuContent<vector<Issue *>>(const vector<Issue *> &d, Choices &c) {
  std::string k = "a";
  std::map<char, Initiative *> inis;

  for(auto &i: d) {
    for(auto &j: i->findInitiatives()) {
      inis[k[0]] = j;
      k[0] = nextKey(k[0]);
    }
  }

  k = "a";

  c["diff"] = [=] {
    if(args.size() != 1 || args[0].size() != 2)
      throw user_error("usage: diff xy     - x and y being initiative keys");

    auto a = inis.find(args[0][0]);
    auto b = inis.find(args[0][1]);
    if(a == inis.end()) throw user_error("initative does not exist");
    if(b == inis.end()) throw user_error("initative does not exist");

    showDiff(
       "===" + a->second->name() + "===\n" + a->second->currentDraft()->content(),
       "===" + b->second->name() + "===\n" + b->second->currentDraft()->content());
  };
  c["vote"] = [=] {
    if(args.size() != 1)
      throw user_error("usage: vote ab>cd_>ef   - all letters being initiative keys of the same issue");

    int group = 0;
    int neutralOffset = -1;
    vector<vector<Initiative *>> votes {{}};
    Issue *issue = nullptr;

    for(auto &c: args[0]) {
      switch(c) {
        case '_':
          if(neutralOffset != -1) throw user_error("only a single _ may specified - it denotes the unique neutral group");
          neutralOffset = group;
          break;
        case '>':
          ++group;
          votes.push_back({});
          break;
        default: {
          auto ini = inis.find(c);
          if(ini == inis.end()) throw user_error("initiative does not exist: " + c);
          votes.back().push_back(ini->second);
          if(issue && issue != ini->second->findIssue())
            throw user_error("initiatives from multiple issues mixed");
          issue = ini->second->findIssue();
        }
      }
    }

    if(neutralOffset == -1) throw user_error("no _ specified - but it is needed to determine the neutral group");

    map<Initiative *, int> ballot;
    for(unsigned int i = 0; i < votes.size(); ++i) {
      for(auto &j: votes[i]) ballot[j] = neutralOffset - i;
    }

    if(!issue) throw user_error("no initiatives specified");
    issue->castVote(ballot);
  };

  for(auto &i: d) {
    std::string stateString;
    switch(i->state()) {
      case IssueState::ADMISSION: stateString = "admission"; break;
      case IssueState::DISCUSSION: stateString = "discussion"; break;
      case IssueState::VERIFICATION: stateString = "verification"; break;
      case IssueState::VOTING: stateString = "voting"; break;
      case IssueState::CANCELED: stateString = "cancelled"; break;
      case IssueState::FINISHED: stateString = "finished"; break;
      default: stateString = "<unknown state>"; break;
    }

    cout << "===" << i->id() << " (" << stateString << ") ";
    if(i->state() == IssueState::VOTING && !i->haveVoted()) cout << colorAlert("[not voted yet] ");
    cout << "===" << endl;
    c[i->id()] = [=]{ menu(i); };

    for(auto &j: i->findInitiatives()) {
      char stateChar = '.';
      if(j->amSupporter()) stateChar = 's';
      if(j->isRevoked()) stateChar = 'R';
      string seenChar = ".";
      if(!j->currentDraft()->seen()) seenChar = colorAlert("u");

      cout << k << ") "
        << setw(4) << j->supporterCount()
        << "/" << setw(4) << j->satisfiedSupporterCount()
        << " " << stateChar << seenChar
        << "  " << j->name() << endl;
      if(j->note() != "") {
        istringstream note(j->note());
        string output;
        getline(note, output);
        cout << "                 " << output << endl;
      }
      c[k] = [=]{ menu(j); };
      if(inis[k[0]] != j) throw logic_error("initiative iteration diverged");

      k[0] = nextKey(k[0]);
    }
  }
}

template<> void TerminalUI::menuHelp<vector<Issue *>>(const vector<Issue *> &, Choices &) {
  cout << "^  ^^^^ ^^^^ ^^  ^^^^^^" << endl;
  cout << "|   |    |   ||     |  " << endl;
  cout << "|   |    |   ||     '-- title of the initiative" << endl;
  cout << "|   |    |   ||         private note below title" << endl;
  cout << "|   |    |   |'-------- initiative has an [u]nseen current draft" << endl;
  cout << "|   |    |   '--------- initative is [s]upported by you" << endl;
  cout << "|   |    |              initative is [R]evoked by authors" << endl;
  cout << "|   |    '------------- number of satisfied supporters (no outstanding suggestions)" << endl;
  cout << "|   '------------------ number of supporters" << endl;
  cout << "'---------------------- type this letter to select initiative" << endl;
  cout << "=== Available commands ===" << endl;
  cout << "<number> - select issue" << endl;
  cout << "<letter> - select initiative" << endl;
  cout << "diff xy - compare two initiatives x and y" << endl;
  cout << "vote a>bc>def_>gh>i - vote a > b = c (all supported) > d = e = f (all neutral) >" << endl;
  cout << "                        g = h > i (all opposed)" << endl;
  cout << "                      the underscore marks the neutral group" << endl;
}

template<> std::string TerminalUI::menuPrompt<vector<Issue *>>() { return "IssueSet"; }

template<> void TerminalUI::menuContent<Initiative *>(Initiative *const & i, Choices &c) {
  std::map<char, Suggestion *> sugs;
  char k = 'a';
  for(auto &j: i->findSuggestions()) {
    sugs[k] = j;
    k = nextKey(k);
  }

  if(i->findIssue()->state() == IssueState::ADMISSION ||
      i->findIssue()->state() == IssueState::DISCUSSION) {
    c["sup"] = [=] { i->support(true); };
    c["rej"] = [=] { i->support(false); };
    c["note"] = [=] {
      Tempfile tmp;
      {
        ofstream txt(tmp.name());

        istringstream note(i->note());
        {
          string line;
          while(getline(note, line)) {
            if(line.size() > 0 && line[line.size() - 1] == '\r') line = line.substr(0, line.size() - 1);
            txt << line << endl;
          }
        }
        if(i->note() == "") txt << endl;
        txt << "# Lines starting in # are ignored..." << endl
          << "#" << endl;
        txt << "# " << i->name() << endl;
        istringstream content(i->currentDraft()->content());
        string line;
        while(getline(content, line)) {
          if(line.size() > 0 && line[line.size() - 1] == '\r') line = line.substr(0, line.size() - 1);
          txt << "# " << line << endl;
        }
      }

      std::string cmd = std::string("${EDITOR:-vim} '") + tmp.name() + "'";
      system(cmd.c_str());

      {
        ifstream txt(tmp.name());
        string note;
        string line;
        while(getline(txt, line)) {
          if(!line.empty() && line[0] == '#') continue;
          note += line + "\r\n";
        }

        i->setNote(note);
      }
    };
    c["com"] = [=] {
      Tempfile tmp;
      {
        ofstream txt(tmp.name());

        txt << "Title of the suggestion" << endl
          << "The text of the suggestion goes here," << endl
          << "and in the following lines." << endl
          << "# Lines starting in # are ignored..." << endl
          << "#" << endl;
        txt << "# " << i->name() << endl;
        istringstream content(i->currentDraft()->content());
        string line;
        while(getline(content, line)) {
          if(line.size() > 0 && line[line.size() - 1] == '\r') line = line.substr(0, line.size() - 1);
          txt << "# " << line << endl;
        }
      }

      std::string cmd = std::string("${EDITOR:-vim} '") + tmp.name() + "'";
      system(cmd.c_str());

      {
        ifstream txt(tmp.name());
        string name;
        if(!getline(txt, name)) throw user_error("no title supplied");

        string content;
        string line;
        while(getline(txt, line)) {
          if(!line.empty() && line[0] == '#') continue;
          content += line + "\r\n";
        }

        if(name == "Title of the suggestion") throw user_error("title not changed");

        i->createSuggestion(name, content);
      }
    };
    c["fork"] = [=] {
      Tempfile tmp;
      {
        ofstream txt(tmp.name());

        txt << i->name() << endl;
        istringstream content(i->currentDraft()->content());
        string line;
        while(getline(content, line)) {
          if(line.size() > 0 && line[line.size() - 1] == '\r') line = line.substr(0, line.size() - 1);
          txt << line << endl;
        }

        txt  << "# Lines starting in # are ignored..." << endl
          << "#" << endl;
        for(auto &i: sugs) {
          txt << "# === " << i.second->name() << endl;
          istringstream content(i.second->content());
          string line;
          while(getline(content, line)) {
            if(line.size() > 0 && line[line.size() - 1] == '\r') line = line.substr(0, line.size() - 1);
            txt << "# " << line << endl;
          }
        }
      }

      std::string cmd = std::string("${EDITOR:-vim} '") + tmp.name() + "'";
      system(cmd.c_str());

      {
        ifstream txt(tmp.name());
        string name;
        if(!getline(txt, name)) throw user_error("no title supplied");

        string content;
        string line;
        while(getline(txt, line)) {
          if(!line.empty() && line[0] == '#') continue;
          content += line + "\r\n";
        }

        if(name == i->name()) throw user_error("title not changed");

        i->findIssue()->createInitiative(name, content);
      }
    };
  }

  c["vers"] = [=] { menu(i->findDrafts()); };
  c["seen"] = [=] { i->currentDraft()->setSeen(true); };
  c["unseen"] = [=] { i->currentDraft()->setSeen(false); };

  std::string txt = i->currentDraft()->content();
  cout << "===" << i->name() << "===" << endl;
  cout << txt << endl;

  cout << "=== Private Notes ===" << endl;
  cout << i->note() << endl;

  cout << "=== Suggestions ===" << endl;
  for(auto &i: sugs) {
    string seenChar = ".";
    if(!i.second->seen()) seenChar = colorAlert("u");
    cout << i.first << ") " << seenChar << " " << i.second->name() << endl;
    c[std::string() + i.first] = [=]{ menu(i.second); };
  }
}

template<> void TerminalUI::menuHelp<Initiative *>(Initiative *const & i, Choices &) {
  cout << "^  ^ ^^^^^^^" << endl;
  cout << "|  |     |" << endl;
  cout << "|  |     '-- title of suggestion" << endl;
  cout << "|  '-------- suggestion is [u]nseen" << endl;
  cout << "'----------- type this letter to access suggestion" << endl;
  cout << "=== Available Commands ===" << endl;
  cout << "<letter> - select suggestion" << endl;
  if(i->findIssue()->state() == IssueState::ADMISSION ||
      i->findIssue()->state() == IssueState::DISCUSSION) {
    cout << "sup - support initiative" << endl;
    cout << "rej - reject (un-support) initiative" << endl;
    cout << "com - create comment (suggestion)" << endl;
    cout << "note - edit the private note, create if necessary" << endl;
    cout << "fork - create a new initiative based upon this one" << endl;
  }
  cout << "vers - show historical draft versions" << endl;
  cout << "seen - set current draft as seen" << endl;
  cout << "unseen - set current draft as unseen" << endl;
}

template<> std::string TerminalUI::menuPrompt<Initiative *>() { return "Initiative"; }

template<> void TerminalUI::menuContent<Suggestion *>(Suggestion *const &s, Choices &c) {
    cout << "===" << s->name() << "===" << endl;
    cout << s->content() << endl;

    c["mn"] = [=] { s->setOpinion(Suggestion::MUST_NOT); s->setSeen(true); };
    c["sn"] = [=] { s->setOpinion(Suggestion::SHOULD_NOT); s->setSeen(true); };
    c["s"] = [=] { s->setOpinion(Suggestion::SHOULD); s->setSeen(true); };
    c["m"] = [=] { s->setOpinion(Suggestion::MUST); s->setSeen(true); };
    c["f"] = [=] { s->setOpinion(Suggestion::FULFILLED); s->setSeen(true); };
    c["u"] = [=] { s->setOpinion(Suggestion::UNFULFILLED); s->setSeen(true); };
    c["n"] = [=] { s->resetOpinion(); };
    c["seen"] = [=] { s->setSeen(true); };
    c["unseen"] = [=] { s->setSeen(false); };
}

template<> void TerminalUI::menuHelp<Suggestion *>(Suggestion *const &, Choices &) {
  cout << "=== Available Commands ===" << endl;
  cout << "mn - mark suggestion as 'must not'" << endl;
  cout << "sn - mark suggestion as 'should not'" << endl;
  cout << "s - mark suggestion as 'should'" << endl;
  cout << "m - mark suggestion as 'must'" << endl;
  cout << "f - mark suggestion as 'fulfilled'" << endl;
  cout << "u - mark suggestion as 'unfulfilled'" << endl;
  cout << "n - remove all marks on suggestion" << endl;
  cout << "seen - set suggestion to seen" << endl;
  cout << "unseen - set suggestion to unseen" << endl;
}

template<> std::string TerminalUI::menuPrompt<Suggestion *>() { return "Suggestion"; }

template<> void TerminalUI::menuContent<vector<Draft *>>(const vector<Draft *> &drafts, Choices &c) {
  char k = 'a';
  std::map<char, Draft *> draftMap;

  for(auto &i: drafts) {
    string seenChar = ".";
    if(!i->seen()) seenChar = colorAlert("u");

    cout << k << ") " << seenChar << " " << i->created() << endl;
    c[std::string() + k] = [=] { menu(i); };
    draftMap[k] = i;
    k = nextKey(k);
  }

  c["diff"] = [=] {
    if(args.size() != 1 || args[0].size() != 2)
      throw user_error("usage: diff xy     - x and y being draft keys");

    auto a = draftMap.find(args[0][0]);
    auto b = draftMap.find(args[0][1]);
    if(a == draftMap.end()) throw user_error("initative does not exist");
    if(b == draftMap.end()) throw user_error("initative does not exist");

    showDiff(a->second->content(), b->second->content());

    a->second->setSeen(true);
    b->second->setSeen(true);
  };
}

template<> void TerminalUI::menuHelp<vector<Draft *>>(const vector<Draft *> &, Choices &) {
  cout << "^  ^ ^^^^^^^^" << endl;
  cout << "|  |     |" << endl;
  cout << "|  |     '-- when the draft was created" << endl;
  cout << "|  '-------- draft is [u]nseen" << endl;
  cout << "'----------- draft identifiers" << endl;
  cout << "=== Available Commands ===" << endl;
  cout << "diff xy - compare drafts x and y" << endl;
}

template<> std::string TerminalUI::menuPrompt<vector<Draft *>>() { return "DraftSet"; }

template<> void TerminalUI::menuContent<Draft *>(Draft *const &d, Choices &c) {
  cout << d->content() << endl;
  if(d->seen()) {
    cout << "You have marked this draft as seen." << endl;
  } else {
    cout << "This draft is unseen." << endl;
  }

  c["seen"] = [=] { d->setSeen(true); };
  c["unseen"] = [=] { d->setSeen(false); };
}

template<> void TerminalUI::menuHelp<Draft *>(Draft *const &, Choices &) {
  cout << "=== Available Commands ===" << endl;
  cout << "seen - mark this draft as seen" << endl;
  cout << "unseen - mark this draft as unseen" << endl;
}

template<> std::string TerminalUI::menuPrompt<Draft *>() { return "Draft"; }

template<> void TerminalUI::menuContent<vector<Area *>>(const vector<Area *> &areas, Choices &c) {
  std::map<char, Area *> areaMap;
  char k = 'a';
  for(auto &i: areas) {
    areaMap[k] = i;
    k = nextKey(k);
  }

  for(auto &i: areaMap) {
    c[std::string() + i.first] = [=]{ menu(i.second); };
    cout << i.first << ") " << i.second->name() << endl;
  }
}

template<> void TerminalUI::menuHelp<vector<Area *>>(const vector<Area *> &, Choices &) {
  cout << "^" << endl;
  cout << "|" << endl;
  cout << "'-- type this letter to access area" << endl;
  cout << "=== Available Commands ===" << endl;
  cout << "<letter> - select area" << endl;
}

template<> std::string TerminalUI::menuPrompt<vector<Area *>>() { return "AreaSet"; }

template<> void TerminalUI::menuContent<Area *>(Area *const &a, Choices &c) {
  c["prop"] = [=] {
    Tempfile tmp;
    {
      ofstream txt(tmp.name());

      txt << "Title of the new initiative" << endl;
      txt << "# Please un-comment and re-comment one of the following lines" << endl;
      txt << "# to select the voting policy for the new issue" << endl;
      for(auto &p: a->findAllowedPolicies()) {
        if(p->id() != a->defaultPolicyId()) {
          txt << "# ";
        }
        txt << p->id() << " " << p->name() << endl;
      }
      txt << "# Thank you. Now for the..." << endl;
      txt << "Text of the new initiative," << endl;
      txt << "possibly spanning multiple lines" << endl;
    }

    std::string cmd = std::string("${EDITOR:-vim} '") + tmp.name() + "'";
    system(cmd.c_str());

    {
      ifstream txt(tmp.name());
      string name;
      if(!getline(txt, name)) throw user_error("no title supplied");

      std::string policyId;
      string policyLine;
      while(policyId == "" && getline(txt, policyLine)) {
        if(policyLine.empty() || policyLine[0] == '#') continue;
        istringstream(policyLine) >> policyId;
      }

      Policy *policy = nullptr;
      for(auto &i: a->findAllowedPolicies()) {
        if(i->id() == policyId) policy = i;
      }
      if(!policy) throw user_error("no or invalid policy supplied");

      string content;
      string line;
      while(getline(txt, line)) {
        if(!line.empty() && line[0] == '#') continue;
        content += line + "\r\n";
      }

      if(name == "Title of the new initiative") throw user_error("title not changed");
      if(content.size() < 20) throw user_error("insufficient content supplied");

      a->createIssue(policy, name, content);
    }
  };

  cout << "===" << a->name() << "===" << endl;
  cout << a->description() << endl;

  menuContent(a->findOpenIssues(), c);
}

template<> void TerminalUI::menuHelp<Area *>(Area *const &a, Choices &c) {
  menuHelp(a->findOpenIssues(), c);
  cout << "prop - propose an initiative" << endl;
}

template<> std::string TerminalUI::menuPrompt<Area *>() { return "Area"; }

template<> void TerminalUI::menuContent<Issue *>(Issue *const &, Choices &) {
  cout << "There is nothing here yet. Please leave." << endl;
}

template<> void TerminalUI::menuHelp<Issue *>(Issue *const &, Choices &) {
  cout << "Enter the empty line to leave this menu." << endl;
}

template<> std::string TerminalUI::menuPrompt<Issue *>() { return "Issue"; }

void TerminalUI::operator() () {
  bool help = false;
  while(!cin.eof()) {
    cout << "type help for instructions" << endl;

    if(help) {
      cout << "help - display this" << endl;
      cout << "info - query lqfb instance for general info" << endl;
      cout << "open - select all open issues" << endl;
      cout << "admi - select all issues in admission" << endl;
      cout << "disc - select all issues in discussion" << endl;
      cout << "vote - select all issues in voting" << endl;
      cout << "area - list all areas" << endl;
      cout << endl;
      cout << "syntax" << endl;
      cout << "*xy... <command> <args...>" << endl;
      cout << "     - execute <command> for each x, y, ..." << endl;
      cout << endl;
      cout << "entering a single . will redisplay output" << endl;
      cout << "entering an empty line will generally leave a submenu" << endl;
    }

    help = false;
    handleChoice("", Choices{
        { "help", [&]{ help = true; }},
        { "info", [=]{ cout << r.getInfo() << endl; }},
        { "open", [=]{ menu(r.findIssues(IssueState::OPEN)); }},
        { "admi", [=]{ menu(r.findIssues(IssueState::ADMISSION)); }},
        { "disc", [=]{ menu(r.findIssues(IssueState::DISCUSSION)); }},
        { "vote", [=]{ menu(r.findIssues(IssueState::VOTING)); }},
        { "area", [=]{ menu(r.findAreas()); }},
      });
  }
}

char TerminalUI::nextKey(char k) {
  if(k >= 'a' && k < 'z') return ++k;
  if(k >= 'A' && k < 'Z') return ++k;
  if(k == 'z') return 'A';
  throw std::runtime_error("Out of assignable keys");
}
