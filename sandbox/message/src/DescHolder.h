#ifndef MESSAGE_DESCHOLDER_H
#define MESSAGE_DESCHOLDER_H
#include <poll.h>
#include "mutex.h"
#include "common/include/common.h"

static const uint32 DEFAULT_SET_LEN = 128;

class DescHolder {
public:
  DescHolder(uint32 size=DEFAULT_SET_LEN): m_size(size),m_fdCount(0) {
    CHECK(m_size > 0);
    m_fdList = new struct pollfd[size];
    for (int i = 0; i < m_size; ++i) {
      m_fdList[i].fd = -1;
      m_fdList[i].events = 0;
      m_fdList[i].revents = 0;
    }

  };

  inline struct pollfd* getList() {
    return m_fdList;
  }

  inline void addFdToSet(int fd, short events) {
    if(m_fdCount == m_size) {
      reSizeSet();
    }
    for(int i = 0; i < m_size; ++i) {
      if (m_fdList[i].fd == -1) {
        m_fdList[i].fd = fd;
        m_fdList[i].events = events;
        break;
      }
    }
    ++m_fdCount;
  }

  inline void reSizeSet() {
    m_size += DEFAULT_SET_LEN;
    struct pollfd* tmp = new struct pollfd[m_size];
    for (int i = 0; i < m_fdCount; ++i) {
      tmp[i].fd = m_fdList[i].fd;
      tmp[i].events = m_fdList[i].events;
      tmp[i].revents = m_fdList[i].revents;
    }
  }

  HAUtils::Mutex m_mutex;
private:
  struct pollfd* m_fdList;
  uint32 m_size;
  uint32 m_fdCount;
};
#endif
