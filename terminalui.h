#ifndef TERMINALUI_H
#define TERMINALUI_H

#include "reningsverk.h"

#include <string>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <iterator>
#include <algorithm>

class TerminalUI {
  public:
    TerminalUI(Reningsverk &&r): r{std::move(r)} { }

    void operator() ();
    
  private:
    typedef std::unordered_map<std::string, std::function<void()>> Choices;

    Reningsverk r;

    class user_error: public std::runtime_error {
      public:
        user_error(const std::string &what_arg): runtime_error(what_arg) { }
        user_error(const char *what_arg): runtime_error(what_arg) { }
    };

    std::string readline();

    std::vector<std::string> args; // arguments of user input

    template<typename M> void handleChoice(const std::string &prompt, const M &m) {
      while(!std::cin.eof()) {
        try {
          std::cout << prompt << "> " << std::flush;

          std::istringstream input(readline());
          std::string command;
          input >> command;

          args.clear();
          copy(std::istream_iterator<std::string>(input),
              std::istream_iterator<std::string>(), std::back_inserter(args));
          
          auto c = m.find(command);
          if(c == m.end()) throw user_error("no such command");

          c->second();
          return;
        } catch(user_error &e) {
          std::cout << e.what() << std::endl;
        }
      }

      auto c = m.find("");
      if(c != m.end()) {
        c->second();
      }
    }

    void menuIssueSet(std::vector<Issue *>);
    void menuIssue(Issue *);
    void menuInitiative(Initiative *);

    char nextKey(char);
};

#endif
