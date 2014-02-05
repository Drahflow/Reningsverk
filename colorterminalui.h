#ifndef COLORTERMINALUI_H
#define COLORTERMINALUI_H

#include "terminalui.h"

class ColorTerminalUI: public TerminalUI {
  public:
    ColorTerminalUI(Reningsverk &&r): TerminalUI(std::move(r)) { }

  protected:
    std::string colorAlert(const std::string &s) { return "\033[31;1m" + s + "\033[0m"; }
};

#endif
