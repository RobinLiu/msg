#include "message.h"


Message::Message(Message& aMessage):header(aMessage.header), pdata(NULL) {
  SetData(aMessage.pdata, aMessage.header.messageLength);
}

Message& Message::operator=(Message& aMessage){
  SetData(aMessage.pdata, aMessage.header.messageLength);
  return (*this);  
}

void Message::SetData(const uint8* msgData, const uint32 msgDataLength) {
  if (NULL != pdata) {
    delete [] pdata;
	pdata = NULL;
  }
  if (msgDataLength > 0) {
	pdata = new uint8[msgDataLength];
    if (NULL == pdata) {
      LOG(ERROR)<<"Allocate buffer for message failed";
	  header.messageLength = 0;
    }
    LOG(INFO)<<"msg is "<<msgData<<" and len is "<<msgDataLength;
    memcpy(pdata, msgData, msgDataLength);
    header.messageLength = msgDataLength;
  }
  
}

error_no_t GetMsgDest(const Message& msg, HostList& hostList) {
//  const uint32 pid = msg.header.receiver.GetPid();
  //TODO: add route function here;
  HostId host;
//  hostList.instert(host);
  return SUCCESS_EC;
	
}

class Connection;
class ConnectionMgr;
error_no_t GetHostConnection(HostId& hostId) {

	return SUCCESS_EC;
}
