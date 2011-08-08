//============================================================================
// Name        : Log.cpp
// Author      : Robin
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C, Ansi-style
//============================================================================

#include "message_center.h"
#include <unistd.h>
#include <iostream>

int main(void) {
  MsgCenter* m = MsgCenter::get_instance();
  int i = 0;
  while(i++ < 100) {
    m->add_1();
    sleep(1);
    std::cout<<"sum is "<<m->sum<<"\n";

  }
}
