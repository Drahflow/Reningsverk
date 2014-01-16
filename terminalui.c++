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
  bool run = true;
  char k = 'a';
  std::map<char, Initiative *> inis;

  for(auto &i: v) {
    for(auto &j: i->findInitiatives()) {
      inis[k] = j;
      k = nextKey(k);
    }
  }

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
      if(args[0].size() != 2) throw user_error("usage: diff xy     - x and y being initiative keys");

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
  bool run = true;

  while(run) {
    Choices c;
    c[""] = [&]{ run = false; };

    std::string txt = i->currentDraft()->content();
    cout << "===" << i->name() << "===" << endl;
    cout << txt << endl;

    handleChoice("Initiative", c);
  }
}

void TerminalUI::operator() () {
  while(!cin.eof()) {
    cout << "type help for instructions" << endl;

    handleChoice("", Choices{
        { "help", []{
          cout << "help - display this" << endl;
          cout << "info - query lqfb instance for general info" << endl;
          cout << "disc - display all issues in discussion" << endl;
        }},
        { "info", [=]{
          cout << r.getInfo() << endl;
        }},
        { "disc", [=]{
          menuIssueSet(r.findIssues(IssueState::DISCUSSION));
        }}
      });
  }
}

char TerminalUI::nextKey(char k) {
  if(k >= 'a' && k < 'z') return ++k;
  if(k >= 'A' && k < 'Z') return ++k;
  if(k == 'z') return 'A';
  throw std::runtime_error("Out of assignable keys");
}
