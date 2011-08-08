#include "message_center.h"

#include <iostream>


MsgCenter* MsgCenter::_instance = NULL;
static Mutex g_msg_mutex;


MsgCenter::MsgCenter(): sum(0){

}
MsgCenter* MsgCenter::get_instance() {
  if (NULL == _instance) {
    MutexHolder mh(g_msg_mutex);
    if (NULL == _instance) {
      _instance = new MsgCenter();
    }
  }

//  ("_instance at %p", _instance);
  std::cout<<"_instance at " <<_instance;
  return _instance;
}


void MsgCenter::add_1() {
  MutexHolder mh(g_msg_mutex);
  sum++;
}


