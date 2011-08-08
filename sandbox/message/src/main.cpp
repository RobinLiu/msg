#include "common/include/common.h"
#include "message.h"
#include "ConnectionHandler.h"


int main(void) {
  logging::InitLogging("outputlog.txt",
                logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG,
                logging::LOCK_LOG_FILE,
                logging::APPEND_TO_OLD_LOG_FILE,
                /*logging::ENABLE_DCHECK_FOR_NON_OFFICIAL_RELEASE_BUILDS*/
	            logging::DISABLE_DCHECK_FOR_NON_OFFICIAL_RELEASE_BUILDS);
  char* msg = "this is the message";
  //LOG(INFO)<<msg;
  MessageHeader header;
  Message a(header, (uint8*)msg, strlen(msg)+1);
  Message b;
  CHECK(b.GetData() == NULL);
  b = a;
  CHECK(b.GetData() != NULL);
  LOG(INFO)<<b.GetData();
  LOG(INFO)<<a.GetData();
  Message c(a);
  LOG(INFO)<<c.GetData();
  Message* d = new Message(c);
  LOG(INFO)<<d->GetData();
  delete d;
  

  ConnectionHandler ch("localhost", 9999);
  ch.createTCPServer("localhost", 9999);
}
