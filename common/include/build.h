#ifndef INCLUDE_BUILD_H
#define INCLUDE_BUILD_H
#ifdef __cplusplus
extern "C" {
#endif

//Here defines compilation flags

//using posix thread library
#define USING_POSIX_THREAD      1
#define USING_POSIX_MSG_QUEUE   1
#define DEBUG_LOCK              1
//enable message debug
#define DBG_MSG                 1

//using layer 2 to transport message
#define USING_LAYER2            1
#define ENABLE_VLAN             0
#define SUPPORT_2N_REDUNDANCY   1
#define USE_TIMED_SEND          1


#ifdef __cplusplus
}
#endif
#endif
