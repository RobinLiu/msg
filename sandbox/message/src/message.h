#ifndef MESSAGE_MESSAGE_H
#define MESSAGE_MESSAGE_H
#include "common/include/common.h"

typedef uint32 MagicWord;
typedef uint8  DeliveryRange;
enum MessagePriority {NORMAL, HIGH, IMPORTANT};
typedef uint32 HostId;





#include <vector>
typedef std::vector<HostId> HostList;

class ProgramIdentifier {
  public:
  ProgramIdentifier(uint32 programId):m_programId(programId) {
  };
  uint32 GetPid() {
    return m_programId;
  }
private:
  uint32 m_programId;
};


struct MessageHeader {
  MagicWord			messageHeaderFront;
  ProgramIdentifier receiver;
  DeliveryRange		deliveryRange;
  ProgramIdentifier sender;
  MessagePriority   priority;
  uint32			sequenceNumber;
  uint32			messageLength;
  
  MessageHeader(): 
    messageHeaderFront(0),
	receiver(0),
	deliveryRange(0),
	sender(0),
	priority(NORMAL),
	sequenceNumber(0u),
	messageLength(0u){
  }
};

class Message {
public:
  Message():pdata(NULL) {
  }
  Message(MessageHeader msgHeader, 
          const uint8* msgData, 
		  const uint32 msgDataLength) 
    :header(msgHeader), pdata(NULL){
    SetData(msgData, msgDataLength);
  }
  
  Message(Message& aMessage);
  Message& operator=(Message& aMessage);
  
  ~Message() {
    if (NULL != pdata) {
      delete [] pdata;
	  pdata = NULL;
	}
  }
  
  uint8* GetData() {
    return pdata;
  }
  void SetData(const uint8* msgData, const uint32 msgDataLength);
  MessageHeader header;
  uint8*        pdata;
};

#endif
