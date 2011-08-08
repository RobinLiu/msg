#ifndef MESSAGE_SINGLETON_H
#define MESSAGE_SINGLETON_H
//#ifdef __cplusplus
//extern "C" {
//#endif

#include "Mutex.h"
#include "MutexHolder.h"



class MsgCenter {
public:
  static MsgCenter* get_instance();
  void add_1();

  ~MsgCenter();
  int sum;
private:
  MsgCenter();

  static MsgCenter* _instance;
};

//#ifdef __cplusplus
//}
//#endif
#endif
