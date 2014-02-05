#ifndef TERMINALUI_H
#define TERMINALUI_H

#include "reningsverk.h"
#include "raii.h"

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
    virtual ~TerminalUI() { }

    void operator() ();

  protected:
    virtual std::string colorAlert(const std::string &s) { return s; }
    
  private:
    typedef std::unordered_map<std::string, std::function<void()>> Choices;

    Reningsverk r;

    class user_error: public std::runtime_error {
      public:
        user_error(const std::string &what_arg): runtime_error(what_arg) { }
        user_error(const char *what_arg): runtime_error(what_arg) { }
    };

    std::string readline();

    std::string command; // command from user input
    std::vector<std::string> args; // arguments from user input

    std::vector<std::string> presuppliedInput;

    template<typename M> void handleChoice(const std::string &prompt, const M &m) {
      if(presuppliedInput.size()) {
        std::cout << prompt << "> ";
        for(auto &s: presuppliedInput) std::cout << s << " ";
        std::cout << std::endl;

        command = presuppliedInput[0];
        args.clear();

        copy(presuppliedInput.begin() + 1, presuppliedInput.end(), std::back_inserter(args));

        handleCommand(m);
      } else {
        while(!std::cin.eof()) {
          try {
            command = "";
            args.clear();

            std::cout << prompt << "> " << std::flush;

            std::istringstream input(readline());
            input >> command;

            copy(std::istream_iterator<std::string>(input),
                std::istream_iterator<std::string>(), std::back_inserter(args));
            
            handleCommand(m);
            return;
          } catch(user_error &e) {
            std::cout << e.what() << std::endl;
          }
        }
      }

      auto c = m.find("");
      if(c != m.end()) {
        c->second();
      }
    }

    template<typename M> void handleCommand(const M &m) {
      if(command[0] == '*') {
        auto iteration = command.substr(1);
        auto savedArgs = args;

        for(auto c: iteration)
          if(m.find(std::string() + c) == m.end())
            throw user_error("no such command (in iteration range)");

        for(auto c: iteration) {
          presuppliedInput = savedArgs;
          auto raii = make_raii([&] () mutable { presuppliedInput.clear(); });

          std::cout << "Handling Iteration " << c << std::endl;
          m.find(std::string() + c)->second();
          std::cout << "Done Iteration " << c << std::endl;
        }
      } else {
        auto cmd = m.find(command);
        if(cmd == m.end()) throw user_error("no such command: " + command);

        cmd->second();
      }
    }

    template<typename D> void menu(const D &);
    template<typename D> void menuContent(const D &, Choices &);
    template<typename D> void menuHelp(const D &, Choices &);
    template<typename D> std::string menuPrompt();

    char nextKey(char);
};

#endif
