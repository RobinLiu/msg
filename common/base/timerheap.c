#include "timer.h"
#include "common/include/logging.h"
#include "lock.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

/** Maximum value for signed 32-bit integer. */
#define MAXINT32  0x7FFFFFFFL
#define DEFAULT_TIMER_HEAP_SIZE  64

#define HEAP_PARENT(X)  (X == 0 ? 0 : (((X) - 1) / 2))
#define HEAP_LEFT(X)  (((X)+(X))+1)

#define TIME_VAL_MSEC(t) ((t).sec * 1000 + (t).msec)

#define TIME_VAL_EQ(t1, t2)  ((t1).sec==(t2).sec && (t1).msec==(t2).msec)

#define TIME_VAL_GT(t1, t2)  ((t1).sec>(t2).sec || \
                                ((t1).sec==(t2).sec && (t1).msec>(t2).msec))

#define TIME_VAL_GTE(t1, t2) (TIME_VAL_GT(t1,t2) || \
                                 TIME_VAL_EQ(t1,t2))

#define TIME_VAL_LT(t1, t2)  (!(TIME_VAL_GTE(t1,t2)))

#define TIME_VAL_LTE(t1, t2) (!TIME_VAL_GT(t1, t2))

#define TIME_VAL_ADD(t1, t2)     do {          \
          (t1).sec += (t2).sec;     \
          (t1).msec += (t2).msec;     \
          time_val_normalize(&(t1)); \
            } while (0)

#define TIME_VAL_SUB(t1, t2)     do {          \
          (t1).sec -= (t2).sec;     \
          (t1).msec -= (t2).msec;     \
          time_val_normalize(&(t1)); \
            } while (0)

typedef struct timer_heap_t {
  int max_size;
  int cur_size;
  lock_t timer_lock;
  timer_entry **heap;
  int *timer_ids;
  int timer_ids_freelist;
  int fd[2];
} timer_heap_t;

//timer_heap_t* ht = NULL;
timer_heap_t* g_timer_heap = NULL;

void time_val_normalize(time_val *t) {
  CHECK(NULL != t);
  if (t->msec >= 1000) {
    t->sec += (t->msec / 1000);
    t->msec = (t->msec % 1000);
  } else if (t->msec <= -1000) {
    do {
      t->sec--;
      t->msec += 1000;
    } while (t->msec <= -1000);
  }

  if (t->sec >= 1 && t->msec < 0) {
    t->sec--;
    t->msec += 1000;

  } else if (t->sec < 0 && t->msec > 0) {
    t->sec++;
    t->msec -= 1000;
  }
}

void gettickcount(time_val* expires) {
  CHECK(NULL != expires);
  struct timeval tv;
  int ret = gettimeofday(&tv, NULL);
  if(0 == ret) {
    expires->sec = tv.tv_sec;
    expires->msec = tv.tv_usec/1000;
  } else {
    expires->sec = expires->msec = 0;
  }
}

void lock_timer_heap(timer_heap_t* ht) {
  CHECK(NULL != ht);
  lock(&ht->timer_lock);
}

void unlock_timer_heap(timer_heap_t* ht) {
  CHECK(NULL != ht);
  unlock(&ht->timer_lock);
}

static void copy_node(timer_heap_t* ht, int slot, timer_entry* moved_node) {
  CHECK(NULL != ht && NULL != moved_node);
  ht->heap[slot] = moved_node;
  ht->timer_ids[moved_node->timer_id] = slot;
}

static int pop_freelist(timer_heap_t* ht) {
  CHECK(NULL != ht);
  int new_timer_id = ht->timer_ids_freelist;
  ht->timer_ids_freelist = -ht->timer_ids[ht->timer_ids_freelist];

  return new_timer_id;
}

static void push_freelist(timer_heap_t* ht, int old_timer_id) {
  CHECK(NULL != ht);
  ht->timer_ids[old_timer_id] = -ht->timer_ids_freelist;
  ht->timer_ids_freelist = old_timer_id;
}

static void reheap_down(timer_heap_t* ht, timer_entry* moved_node,
                        int slot, int child) {
  CHECK(NULL != ht && NULL != moved_node);
  while(child < ht->cur_size) {
    if (child + 1 < ht->cur_size &&
        TIME_VAL_LT(ht->heap[child + 1]->timer_value,
                    ht->heap[child ]->timer_value)) {
      child++;
    }

    if (TIME_VAL_LT(ht->heap[child]->timer_value, moved_node->timer_value)) {
      copy_node(ht, slot, ht->heap[child]);
      slot = child;
      child = HEAP_LEFT(child);
    } else {
      break;
    }
  }

  copy_node(ht, slot, moved_node);
}

static void reheap_up(timer_heap_t* ht, timer_entry* moved_node,
                      int slot, int parent) {
  CHECK(NULL != ht && NULL != moved_node);
  while (slot > 0) {
    if (TIME_VAL_LT(moved_node->timer_value, ht->heap[parent]->timer_value)) {
      copy_node(ht, slot, ht->heap[parent]);
      slot = parent;
      parent = HEAP_PARENT(slot);
    } else {
      break;
    }
  }

  copy_node(ht, slot, moved_node);
}

static timer_entry* remove_node(timer_heap_t* ht, int slot) {
  CHECK(NULL != ht);
  timer_entry* removed_node = ht->heap[slot];
  push_freelist(ht, removed_node->timer_id);

  ht->cur_size--;
  removed_node->timer_id = -1;

  if (slot < ht->cur_size) {
    int parent;
    timer_entry* moved_node = ht->heap[ht->cur_size];
    copy_node(ht, slot, moved_node);
    parent = HEAP_PARENT(slot);

    if (TIME_VAL_GTE(moved_node->timer_value, ht->heap[parent]->timer_value)) {
      reheap_down(ht, moved_node, slot, HEAP_LEFT(slot));
    } else {
      reheap_up(ht, moved_node, slot, parent);
    }
  }

  return removed_node;
}

static void grow_heap(timer_heap_t* ht) {
  CHECK(NULL != ht);
  int new_size = ht->max_size * 2;
  int* new_timer_ids = NULL;
  int i = 0;

  //first grow the heap.
  timer_entry** new_heap = 0;
  new_heap = (timer_entry**) malloc(sizeof(timer_entry*) * new_size);
  CHECK(NULL != new_heap);
  memcpy(new_heap, ht->heap, ht->max_size * sizeof(timer_entry*));
  free(ht->heap);
  ht->heap = new_heap;

  //grow the array of timer ids;
  new_timer_ids = malloc(new_size * sizeof(int));
  CHECK(NULL != new_timer_ids);
  memcpy(new_timer_ids, ht->timer_ids, ht->max_size * sizeof(int));
  free(ht->timer_ids);

  ht->timer_ids = new_timer_ids;

  for(i = ht->max_size; i < new_size; ++i) {
    ht->timer_ids[i] = -(i + 1);
  }
  ht->max_size = new_size;
}

static void insert_node(timer_heap_t* ht, timer_entry* new_node) {
  CHECK(NULL != ht && NULL != new_node);
  if (ht->cur_size + 2 >= ht->max_size) {
    grow_heap(ht);
  }
  reheap_up(ht, new_node, ht->cur_size, HEAP_PARENT(ht->cur_size));
  ht->cur_size++;
}

static int schedule_entry(timer_heap_t* ht, timer_entry* entry,
                          time_val* future_time) {
  CHECK(NULL != ht && NULL != entry && NULL != future_time);
  if (ht->cur_size < ht->max_size) {
    entry->timer_id = pop_freelist(ht);
    entry->timer_value = *future_time;
    insert_node(ht, entry);
    return 0;
  } else {
    return -1;
  }
}

static void cancel(timer_heap_t* ht, timer_entry* entry) {
  CHECK(NULL != ht && NULL != entry);
  long timer_node_slot;
  if (entry->timer_id < 0 || entry->timer_id > ht->max_size) {
    return;
  }

  timer_node_slot = ht->timer_ids[entry->timer_id];

  if (timer_node_slot < 0) {
    return;
  }
  if (entry != ht->heap[timer_node_slot]) {
    return;
  } else {
    remove_node(ht, timer_node_slot);
  }
}

int timer_heap_create(int size, timer_heap_t** p_heap) {
  CHECK(NULL != p_heap);
  timer_heap_t* ht;
  int i;
  *p_heap = NULL;
  //size += 2;

  ht = malloc(sizeof(timer_heap_t));
  CHECK(NULL != ht);
  ht->max_size = size;
  ht->cur_size = 0;
  ht->timer_ids_freelist = 1;

  //init lock
  init_lock(&ht->timer_lock);
  if (0 != pipe(ht->fd)) {
    LOG(ERROR, "pipe failed: %s", strerror(errno));
    free(ht);
    return -1;
  }

  //alloc memeory
  ht->heap = malloc(sizeof(timer_entry*) * size);
  CHECK(NULL != ht->heap);
//  if(NULL == ht->heap) {
//    free(ht);
//    return -1;
//  }
  ht->timer_ids = malloc(sizeof(int) * size);
  CHECK(NULL != ht->timer_ids);
//  if(NULL == ht->timer_ids) {
//    free(ht->heap);
//    ht->heap = NULL;
//    free(ht);
//    return -1;
//  }

  for (i = 0; i < size; ++i) {
    ht->timer_ids[i] = -(i + 1);
  }

  *p_heap = ht;
  return 0;
}

void timer_heap_destroy(timer_heap_t** ht) {
  //TODO:validation
  timer_heap_t* tmp = *ht;
  if (NULL == tmp) {
    return;
  }

  if (NULL != tmp->timer_ids) {
    free(tmp->timer_ids);
    tmp->timer_ids = NULL;
  }

  if (NULL != tmp->heap) {
    free(tmp->heap);
    tmp->heap = NULL;
  }

  free(tmp);
  tmp = NULL;
}

void timer_entry_init(timer_entry* entry, void* user_data, TIMEOUT_FUNC cb,
                      unsigned int timeout) {
  CHECK(NULL != entry && cb != NULL);
  entry->timer_id = -1;
  entry->user_data = user_data;
  entry->cb = cb;
  entry->delay.sec = 0;
  entry->delay.msec = timeout;
  time_val_normalize(&entry->delay);
}

int timer_heap_schedule(timer_heap_t* ht, timer_entry* entry, time_val* delay) {
  CHECK(NULL != ht && NULL != entry);
  int status;
  time_val expires;
  if(!(ht && entry && delay)) {
    return -1;
  }
  if(NULL == entry->cb) {
    return -1;
  }

  if(entry->timer_id >= 1) {
    return -1;
  }

  gettickcount(&expires);
  TIME_VAL_ADD(expires, *delay);

  //lock_timer_heap(ht);
  int was_empty = ht->cur_size == 0;
  int is_timer_less = (ht->cur_size > 0
      && TIME_VAL_LT(entry->timer_value, ht->heap[0]->timer_value));
  status = schedule_entry(ht, entry, &expires);
  //notify the thread to update the polling timer;
  if (was_empty || is_timer_less) {
    LOG(INFO, "Notify thread that a new timer is added ...");
    CHECK(1 == write(ht->fd[1], "0", 1));
  }
  //unlock_timer_heap(ht);

  return status;
}


int timer_heap_cancel(timer_heap_t* ht, timer_entry* entry) {
  if (NULL == ht || NULL == entry) {
    return -1;
  }

  //lock_timer_heap(ht);
  cancel(ht, entry);
  //unlock_timer_heap(ht);

  return 0;
}


unsigned timer_heap_poll(timer_heap_t* ht, time_val* next_delay) {
  time_val now;
  unsigned count = 0;

  CHECK(NULL != ht);

  if (!ht->cur_size && next_delay) {
    next_delay->sec = next_delay->msec = MAXINT32;
    return 0;
  }

  gettickcount(&now);
  lock_timer_heap(ht);
  while(ht->cur_size && TIME_VAL_LTE(ht->heap[0]->timer_value, now)) {
    timer_entry* node = remove_node(ht, 0);
    count++;
    unlock_timer_heap(ht);
    if (node->cb) {
      LOG(INFO, "Timed out, call the callback ...");
      (*node->cb)(node->user_data);
    }
    lock_timer_heap(ht);
    if (ht->cur_size && next_delay) {
      *next_delay = ht->heap[0]->timer_value;
      if (next_delay->sec < 0 || next_delay->msec < 0) {
        next_delay->sec = next_delay->msec = 0;
      }
    } else if (next_delay) {
      next_delay->sec = next_delay->msec = MAXINT32;
    }
  }
  unlock_timer_heap(ht);
  return count;
}


int timer_heap_earliest_time(timer_heap_t* ht, time_val* timeval) {
  CHECK(NULL != ht && NULL != timeval);
  lock_timer_heap(ht);
  if (0 == ht->cur_size) {
    unlock_timer_heap(ht);
    return -1;
  }
  *timeval = ht->heap[0]->timer_value;
  unlock_timer_heap(ht);

  return 0;
}


int cal_timeout_time(timer_heap_t* ht) {
  CHECK(NULL != ht);
  int timeout;
  time_val now;
  time_val delay;

  if(0 != timer_heap_earliest_time(ht, &delay)) {
    delay.sec = delay.msec = MAXINT32;
  }

  if(MAXINT32 == delay.sec) {
    timeout = -1;
  } else {
    gettickcount(&now);
    TIME_VAL_SUB(delay, now);
    timeout = TIME_VAL_MSEC(delay);
    if (timeout < 0) {
      timeout = 0;
    }
  }

  return timeout;
}


void* timer_thread(void* data) {
  CHECK(NULL != data);
  timer_heap_t* ht = (timer_heap_t*)data;

  struct pollfd pfd = {ht->fd[0], POLLIN, 0};
  int ret = 0;
  int timeout = -1;
  char flag[1] = {0};

  while(1) {
    timeout = cal_timeout_time(ht);
    ret = poll(&pfd, 1, timeout);
    if (0 == ret) {
      LOG(INFO, "Poll time out");
    } else if (1 ==ret) {
      CHECK(1 == read(ht->fd[0], flag, 1));
    } else {
      LOG(INFO, "%s", strerror(errno));
    }
    LOG(INFO, "Start timer heap");
    timer_heap_poll(ht, NULL);
  }
}

void init_timer(msg_timer_t* timer,
                 TIMEOUT_FUNC cb,
                 void* data,
                 unsigned int timeout_time) {
  timer_entry_init(timer, data, cb, timeout_time);
}


void stop_timer(msg_timer_t* timer) {
  lock_timer_heap(g_timer_heap);
  timer_heap_cancel(g_timer_heap, timer);
  unlock_timer_heap(g_timer_heap);
}


int start_timer(msg_timer_t* timer) {
  lock_timer_heap(g_timer_heap);
  timer_heap_cancel(g_timer_heap, timer);
  int ret = timer_heap_schedule(g_timer_heap, timer, &timer->delay);
  unlock_timer_heap(g_timer_heap);
  CHECK(0 == ret, "add timer failed");
  return ret;
}

int renew_timer(msg_timer_t* timer, uint32 timeout_time) {
  timer->delay.sec = 0;
  timer->delay.msec = timeout_time;
  time_val_normalize(&timer->delay);
  return start_timer(timer);
}

bool is_timer_started(msg_timer_t* timer) {
  return (-1 != timer->timer_id);
}

int timer_will_expire_in(msg_timer_t* timer) {
  time_val now;
  time_val future;
  int timeout;
  future.sec = timer->timer_value.sec;
  future.msec = timer->timer_value.msec;
  gettickcount(&now);
  TIME_VAL_SUB(future, now);
  timeout = TIME_VAL_MSEC(future);
  return timeout;
}

int init_timer_thread() {
  int ret = 0;
  ret = timer_heap_create(DEFAULT_TIMER_HEAP_SIZE, &g_timer_heap);
  CHECK(0 == ret, "timer_heap_create failed.");
  pthread_t thread_id;
  ret = pthread_create(&thread_id, NULL, &timer_thread, (void*)g_timer_heap);
  if (0 != ret) {
    LOG(ERROR, "create timer thread failed \n");
    timer_heap_destroy(&g_timer_heap);
  }
  CHECK(0 == ret, "Create timer thread failed");

  ret = pthread_kill(thread_id, 0);
  while (ESRCH == ret) {
    sleep(1);
    ret = pthread_kill(thread_id, 0);
  }

  return ret;
}

//int main() {
//  init_timer_thread();
//  msg_timer_t timer;
//  msg_timer_t timer2;
//  int test_data = 3;
//  int test_data2 = 2;
//  init_timer(&timer, &timeout_callback, (void*)&test_data, 3000);
//  init_timer(&timer2, &timeout_callback, (void*)&test_data2, 1000);
//  start_timer(&timer);
//  sleep(1);
//  start_timer(&timer2);
//  stop_timer(&timer);
//  sleep(1000);
//  return 0;
//}
