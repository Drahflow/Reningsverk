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

string TerminalUI::readline() {
  string ret;
  getline(cin, ret);
  return ret;
}

void TerminalUI::menuIssueSet(std::vector<Issue *> v) {
  bool run = true;
  bool help = false;

  while(run) {
    std::string k = "a";
    std::map<char, Initiative *> inis;

    for(auto &i: v) {
      for(auto &j: i->findInitiatives()) {
        inis[k[0]] = j;
        k[0] = nextKey(k[0]);
      }
    }

    Choices c;
    k = "a";

    c[""] = [&]{ run = false; };
    c["help"] = [&] { help = true; };
    c["diff"] = [=] {
      if(args.size() != 1 || args[0].size() != 2)
        throw user_error("usage: diff xy     - x and y being initiative keys");

      auto a = inis.find(args[0][0]);
      auto b = inis.find(args[0][1]);
      if(a == inis.end()) throw user_error("initative does not exist");
      if(b == inis.end()) throw user_error("initative does not exist");

      Tempfile aFile, bFile;

      ofstream(aFile.name()) << "===" << a->second->name() << "===" << endl << a->second->currentDraft()->content() << endl;
      ofstream(bFile.name()) << "===" << b->second->name() << "===" << endl << b->second->currentDraft()->content() << endl;

      std::string cmd = std::string("${DIFF:-vimdiff} '") + aFile.name() + "' '" + bFile.name() + "'";
      system(cmd.c_str());
    };

    for(auto &i: v) {
      cout << "===" << i->id() << "===" << endl;
      c[i->id()] = [=]{ menuIssue(i); };

      for(auto &j: i->findInitiatives()) {
        char stateChar = '.';
        if(j->amSupporter()) stateChar = 's';
        if(j->isRevoked()) stateChar = 'R';

        cout << k << ") "
          << setw(4) << j->supporterCount()
          << "/" << setw(4) << j->satisfiedSupporterCount()
          << " " << stateChar
          << "  " << j->name() << endl;
        if(j->note() != "") {
          istringstream note(j->note());
          string output;
          getline(note, output);
          cout << "                " << output << endl;
        }
        c[k] = [=]{ menuInitiative(j); };
        if(inis[k[0]] != j) throw logic_error("initiative iteration diverged");

        k[0] = nextKey(k[0]);
      }
    }

    if(help) {
      cout << "^  ^^^^ ^^^^ ^  ^^^^^^" << endl;
      cout << "|   |    |   |     |  " << endl;
      cout << "|   |    |   |     '-- title of the initiative" << endl;
      cout << "|   |    |   '-------- initative is [s]upported by you" << endl;
      cout << "|   |    |             initative is [R]evoked by authors" << endl;
      cout << "|   |    '------------ number of satisfied supporters (no outstanding suggestions)" << endl;
      cout << "|   '----------------- number of supporters" << endl;
      cout << "'--------------------- type this letter to select initiative" << endl;
      cout << "=== Available commands ===" << endl;
      cout << "<number> - select issue" << endl;
      cout << "<letter> - select initiative" << endl;
      cout << "diff xy - compare two initiatives x and y" << endl;
    }

    help = false;
    handleChoice("IssueSet", c);
  }
}

void TerminalUI::menuIssue(Issue *) {
}

void TerminalUI::menuInitiative(Initiative *i) {
  bool run = true;
  bool help = false;
  while(run) {
    std::map<char, Suggestion *> sugs;
    char k = 'a';
    for(auto &j: i->findSuggestions()) {
      sugs[k] = j;
      k = nextKey(k);
    }

    Choices c;
    c[""] = [&]{ run = false; };
    c["help"] = [&] { help = true; };

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

    std::string txt = i->currentDraft()->content();
    cout << "===" << i->name() << "===" << endl;
    cout << txt << endl;
    cout << "=== Suggestions ===" << endl;

    for(auto &i: sugs) {
      cout << i.first << ") " << i.second->name() << endl;
      c[std::string() + i.first] = [=]{ menuSuggestion(i.second); };
    }

    if(help) {
      cout << "^  ^^^^^^^" << endl;
      cout << "|      |" << endl;
      cout << "|      '-- title of suggestion" << endl;
      cout << "'--------- type this letter to access suggestion" << endl;
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
    }

    help = false;
    handleChoice("Initiative", c);
  }
}

void TerminalUI::menuSuggestion(Suggestion *s) {
  bool run = true;
  bool help = false;
  while(run) {
    cout << "===" << s->name() << "===" << endl;
    cout << s->content() << endl;

    Choices c;
    c[""] = [&]{ run = false; };
    c["help"] = [&] { help = true; };
    c["mn"] = [=] { s->setOpinion(Suggestion::MUST_NOT); };
    c["sn"] = [=] { s->setOpinion(Suggestion::SHOULD_NOT); };
    c["s"] = [=] { s->setOpinion(Suggestion::SHOULD); };
    c["m"] = [=] { s->setOpinion(Suggestion::MUST); };
    c["f"] = [=] { s->setOpinion(Suggestion::FULFILLED); };
    c["u"] = [=] { s->setOpinion(Suggestion::UNFULFILLED); };
    c["n"] = [=] { s->resetOpinion(); };

    if(help) {
      cout << "=== Available Commands ===" << endl;
      cout << "mn - mark suggestion as 'must not'" << endl;
      cout << "sn - mark suggestion as 'should not'" << endl;
      cout << "s - mark suggestion as 'should'" << endl;
      cout << "m - mark suggestion as 'must'" << endl;
      cout << "f - mark suggestion as 'fulfilled'" << endl;
      cout << "u - mark suggestion as 'unfulfilled'" << endl;
      cout << "n - remove mark on suggestion" << endl;
    }

    help = false;
    handleChoice("Suggestion", c);
  }
}

void TerminalUI::menuAreaSet(std::vector<Area *> areas) {
  bool run = true;
  bool help = false;
  while(run) {
    Choices c;
    c[""] = [&]{ run = false; };
    c["help"] = [&] { help = true; };

    std::map<char, Area *> areaMap;
    char k = 'a';
    for(auto &i: areas) {
      areaMap[k] = i;
      k = nextKey(k);
    }

    for(auto &i: areaMap) {
      c[std::string() + i.first] = [=]{ menuArea(i.second); };
      cout << i.first << ") " << i.second->name() << endl;
    }

    if(help) {
      cout << "^" << endl;
      cout << "|" << endl;
      cout << "'-- type this letter to access area" << endl;
      cout << "=== Available Commands ===" << endl;
      cout << "<letter> - select area" << endl;
    }

    help = false;
    handleChoice("AreaSet", c);
  }
}

void TerminalUI::menuArea(Area *a) {
  bool run = true;
  bool help = false;
  while(run) {
    Choices c;
    c[""] = [&]{ run = false; };
    c["help"] = [&] { help = true; };
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

    if(help) {
      cout << "=== Available Commands ===" << endl;
      cout << "prop - propose an initiative" << endl;
    }

    help = false;
    handleChoice("Area", c);
  }
}

void TerminalUI::operator() () {
  bool help = false;
  while(!cin.eof()) {
    cout << "type help for instructions" << endl;

    if(help) {
      cout << "help - display this" << endl;
      cout << "info - query lqfb instance for general info" << endl;
      cout << "admi - select all issues in admission" << endl;
      cout << "disc - select all issues in discussion" << endl;
      cout << "vote - select all issues in voting" << endl;
      cout << "area - list all areas" << endl;
      cout << endl;
      cout << "syntax" << endl;
      cout << "*xy... <command> <args...>" << endl;
      cout << "     - execute <command> for each x, y, ..." << endl;
      cout << endl;
      cout << "entering an empty line will generally leave a submenu" << endl;
    }

    help = false;
    handleChoice("", Choices{
        { "help", [&]{ help = true; }},
        { "info", [=]{ cout << r.getInfo() << endl; }},
        { "admi", [=]{ menuIssueSet(r.findIssues(IssueState::ADMISSION)); }},
        { "disc", [=]{ menuIssueSet(r.findIssues(IssueState::DISCUSSION)); }},
        { "vote", [=]{ menuIssueSet(r.findIssues(IssueState::VOTING)); }},
        { "area", [=]{ menuAreaSet(r.findAreas()); }},
      });
  }
}

char TerminalUI::nextKey(char k) {
  if(k >= 'a' && k < 'z') return ++k;
  if(k >= 'A' && k < 'Z') return ++k;
  if(k == 'z') return 'A';
  throw std::runtime_error("Out of assignable keys");
}
