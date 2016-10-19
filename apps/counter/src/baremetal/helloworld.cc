//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "Printer.h"
#include "../Counter.h"

void AppMain() {

  Counter::Init();

  {
    char str[80];
    Counter::theCounter->inc();
    snprintf(str, 80, "Hello World count=%d\n", 
	       Counter::theCounter->val());
    printer->Print(str);
  }
}
