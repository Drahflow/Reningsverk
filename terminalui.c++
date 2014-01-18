#include "terminalui.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>
#include <cstdio>
#include <fstream>

#include <stdlib.h>

using namespace std;

string TerminalUI::readline() {
  string ret;
  getline(cin, ret);
  return ret;
}

void TerminalUI::menuIssueSet(std::vector<Issue *> v) {
  char k = 'a';
  std::map<char, Initiative *> inis;

  for(auto &i: v) {
    for(auto &j: i->findInitiatives()) {
      inis[k] = j;
      k = nextKey(k);
    }
  }

  bool run = true;
  while(run) {
    Choices c;
    std::string k = "a";

    c[""] = [&]{ run = false; };
    c["help"] = [] {
      cout << "<number> - select issue" << endl;
      cout << "<letter> - select initiative" << endl;
      cout << "diff - compare two initiatives" << endl;
    };
    c["diff"] = [=] {
      if(args.size() != 1 || args[0].size() != 2)
        throw user_error("usage: diff xy     - x and y being initiative keys");

      auto a = inis.find(args[0][0]);
      auto b = inis.find(args[0][1]);
      if(a == inis.end()) throw user_error("initative does not exist");
      if(b == inis.end()) throw user_error("initative does not exist");

      const char *TMPDIR = getenv("TMPDIR");
      if(!TMPDIR) TMPDIR = "/tmp";

      unique_ptr<char> aName(strdup((TMPDIR + std::string("/abcdXXXXXX")).c_str()));
      unique_ptr<char> bName(strdup((TMPDIR + std::string("/abcdXXXXXX")).c_str()));
      int aFd = mkstemp(aName.get());
      int bFd = mkstemp(bName.get());

      ofstream(aName.get()) << "===" << a->second->name() << "===" << endl << a->second->currentDraft()->content() << endl;
      ofstream(bName.get()) << "===" << b->second->name() << "===" << endl << b->second->currentDraft()->content() << endl;

      std::string cmd = std::string("${DIFF:-vimdiff} '") + aName.get() + "' '" + bName.get() + "'";
      system(cmd.c_str());

      unlink(aName.get());
      unlink(bName.get());
      close(aFd);
      close(bFd);
    };

    for(auto &i: v) {
      cout << "===" << i->id() << "===" << endl;
      c[i->id()] = [=]{ menuIssue(i); };

      for(auto &j: i->findInitiatives()) {
        cout << k << ") " << j->name() << endl;
        c[k] = [=]{ menuInitiative(j); };
        if(inis[k[0]] != j) throw logic_error("initiative iteration diverged");

        k[0] = nextKey(k[0]);
      }
    }

    handleChoice("IssueSet", c);
  }
}

void TerminalUI::menuIssue(Issue *) {
}

void TerminalUI::menuInitiative(Initiative *i) {
  std::map<char, Suggestion *> sugs;
  char k = 'a';
  for(auto &j: i->findSuggestions()) {
    sugs[k] = j;
    k = nextKey(k);
  }

  bool run = true;
  while(run) {
    Choices c;
    c[""] = [&]{ run = false; };
    c["help"] = [=] {
      cout << "<letter> - select suggestion" << endl;
      if(i->findIssue()->state() == IssueState::ADMISSION ||
          i->findIssue()->state() == IssueState::DISCUSSION) {
        cout << "sup - support initiative" << endl;
        cout << "rej - reject (un-support) initiative" << endl;
      }
    };

    if(i->findIssue()->state() == IssueState::ADMISSION ||
        i->findIssue()->state() == IssueState::DISCUSSION) {
      c["sup"] = [=] { i->support(true); };
      c["rej"] = [=] { i->support(false); };
    }

    std::string txt = i->currentDraft()->content();
    cout << "===" << i->name() << "===" << endl;
    cout << txt << endl;
    cout << "=== Suggestions ===" << endl;

    for(auto &i: sugs) {
      cout << i.first << ") " << i.second->name() << endl;
      c[std::string() + i.first] = [=]{ menuSuggestion(i.second); };
    }

    handleChoice("Initiative", c);
  }
}

void TerminalUI::menuSuggestion(Suggestion *s) {
  cout << "===" << s->name() << "===" << endl;
  cout << s->content() << endl;

  bool run = true;
  while(run) {
    Choices c;
    c[""] = [&]{ run = false; };
    c["help"] = [] {
      cout << "mn - mark suggestion as 'must not'" << endl;
      cout << "sn - mark suggestion as 'should not'" << endl;
      cout << "s - mark suggestion as 'should'" << endl;
      cout << "m - mark suggestion as 'must'" << endl;
      cout << "f - mark suggestion as 'fulfilled'" << endl;
      cout << "u - mark suggestion as 'unfulfilled'" << endl;
      cout << "n - remove mark on suggestion" << endl;
    };
    c["mn"] = [=] { s->setOpinion(Suggestion::MUST_NOT); };
    c["sn"] = [=] { s->setOpinion(Suggestion::SHOULD_NOT); };
    c["s"] = [=] { s->setOpinion(Suggestion::SHOULD); };
    c["m"] = [=] { s->setOpinion(Suggestion::MUST); };
    c["f"] = [=] { s->setOpinion(Suggestion::FULFILLED); };
    c["u"] = [=] { s->setOpinion(Suggestion::UNFULFILLED); };
    c["n"] = [=] { s->resetOpinion(); };

    handleChoice("Suggestion", c);
  }
}

void TerminalUI::operator() () {
  while(!cin.eof()) {
    cout << "type help for instructions" << endl;

    handleChoice("", Choices{
        { "help", []{
          cout << "help - display this" << endl;
          cout << "info - query lqfb instance for general info" << endl;
          cout << "admi - select all issues in admission" << endl;
          cout << "disc - select all issues in discussion" << endl;
          cout << "vote - select all issues in voting" << endl;
          cout << endl;
          cout << "syntax" << endl;
          cout << "*xy... <command> <args...>" << endl;
          cout << "     - execute <command> for each x, y, ..." << endl;
        }},
        { "info", [=]{ cout << r.getInfo() << endl; }},
        { "admi", [=]{ menuIssueSet(r.findIssues(IssueState::ADMISSION)); }},
        { "disc", [=]{ menuIssueSet(r.findIssues(IssueState::DISCUSSION)); }},
        { "vote", [=]{ menuIssueSet(r.findIssues(IssueState::VOTING)); }}
      });
  }
}

char TerminalUI::nextKey(char k) {
  if(k >= 'a' && k < 'z') return ++k;
  if(k >= 'A' && k < 'Z') return ++k;
  if(k == 'z') return 'A';
  throw std::runtime_error("Out of assignable keys");
}
