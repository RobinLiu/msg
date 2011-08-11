#include "app_info.h"
#include "common/include/common.h"
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
typedef struct network_info {
  int8          mac[MAC_ADDR_LEN];
  //maybe ip addr and port num if using layer 3;
} network_info_t;

typedef struct group_info {
  uint8   group_id;
  uint8   role;
  uint16  app_list[MAX_APP_NUM_IN_GROUP];
} group_info_t;


typedef struct node_info {
  node_id_t         node_id;
  network_info_t    network_info;
  group_info_t      group_info[MAX_GROUP_NUM_IN_NODE];
} node_info_t;

typedef struct cluster_info {
  node_info_t node_info[MAX_NODE_NUM];
} cluster_info_t;



node_info_t     g_node_info[MAX_NODE_NUM];
node_id_t       g_node_id;
uint8           g_group_id;
uint16          g_app_id;
cluster_info_t  cluster_info;
uint8           g_msg_center_group_id;
uint16          g_msg_center_app_id;

#define BASE_PORT_NUM 5700

uint8 get_self_group_id() {
  return g_group_id;
}

uint16 get_self_app_id() {
  return g_app_id;
}

uint8 get_self_role() {
  return cluster_info.node_info[g_node_id].group_info[g_group_id].role;
}


//TODO: optimize this piece of code. using other data structure to reduce the loops
uint8 get_app_location(msg_receiver_t app, node_id_t* node_list) {
  int32 i = 0;
  int32 j = 0;
  int32 k = 0;
//  LOG(INFO, "app group %d", app.group_id);
  for(i = 0; i < MAX_NODE_NUM; ++i) {
    for(j = 0; j < MAX_GROUP_NUM_IN_NODE; j++) {
      if (cluster_info.node_info[i].group_info[j].group_id == app.group_id) {
        switch(app.role) {
          case RANGE_STANDBY:
            if (cluster_info.node_info[i].group_info[j].role == 0) {
              node_list[k++] = cluster_info.node_info[i].node_id;
              LOG(INFO, "Find group %d in node %d", app.group_id, node_list[k-1]);
            }
            break;
          case RANGE_ACTIVE:
            if (cluster_info.node_info[i].group_info[j].role == 1) {
              node_list[k++] = cluster_info.node_info[i].node_id;
//              LOG(INFO, "Find group %d in node %d", app.group_id, node_list[k-1]);
            }
            break;
          case RANGE_BOTH:
            node_list[k++] = cluster_info.node_info[i].node_id;
            LOG(INFO, "Find group %d in node %d", app.group_id, node_list[k-1]);
            break;
          default:
            LOG(WARNING, "Unkown delvery range.");
            break;
        }//end switch
      }//end if
    }// end for
  }//end for

  return k;
}

node_id_t get_self_node_id() {
  return g_node_id;
}

void read_config() {
  FILE* file;
  file = fopen("config.txt", "r");
  CHECK(NULL != file);
  char buf[128] = {0};
  fgets(buf, sizeof(buf), file);
  int tmp = 0;
  sscanf(buf, "NodeID=%d", &tmp);
  g_node_id = (node_id_t)tmp;

  memset(buf, 0, sizeof(buf));
  fgets(buf, sizeof(buf), file);
  sscanf(buf, "GroupID=%d", &tmp);
  g_group_id = (uint8)tmp;

  memset(buf, 0, sizeof(buf));
  fgets(buf, sizeof(buf), file);
  sscanf(buf, "AppID=%d", &tmp);
  g_app_id = (uint16)tmp;

  memset(buf, 0, sizeof(buf));
  fgets(buf, sizeof(buf), file);
  sscanf(buf, "MSGGroupID=%d", &tmp);
  g_msg_center_group_id = (uint8)tmp;

  memset(buf, 0, sizeof(buf));
  fgets(buf, sizeof(buf), file);
  sscanf(buf, "MSGAppID=%d", &tmp);
  g_msg_center_app_id = (uint16)tmp;

  LOG(INFO, "g_msg_center_group_id:%d, g_msg_center_app_id:%d",
      g_msg_center_group_id, g_msg_center_app_id);
  fclose(file);
}

void init_group_info(group_info_t* group) {
  static uint8 group_id = 0;
  static uint16 app_id = 0;
  group->group_id = group_id++;
  group->role = 1;
  int32 i = 0;
  for(; i < MAX_APP_NUM_IN_GROUP ; ++i) {
    group->app_list[i] = app_id++;
  }
}

void init_node_info(node_info_t* node, int32 index) {
  node->node_id = index;
  node->network_info.mac[0] = 0xaa;
  node->network_info.mac[1] = 0xbb;
  node->network_info.mac[2] = 0xcc;
  node->network_info.mac[3] = 0xdd;
  node->network_info.mac[4] = 0xee;
  node->network_info.mac[5] = 0xf0 + (uint8)index;
  uint32 j = 0;
  if (index > 0 && (index % 2)) {
    memcpy(node->group_info, (node-1)->group_info, sizeof(node->group_info));
    uint32 k = 0;
    for(k = 0; k < MAX_GROUP_NUM_IN_NODE; ++k) {
      node->group_info[k].role = 0;
    }
  } else {
    for(j = 0; j < MAX_GROUP_NUM_IN_NODE; ++j) {
      init_group_info(&node->group_info[j]);
    }
  }
}

void init_sys_info() {
  int32 i = 0;
  for(i = 0; i < MAX_NODE_NUM; ++i) {
    init_node_info(&cluster_info.node_info[i], i);
  }
  read_config();
}

void print_cluster_info() {
  int32 i = 0;
  int32 j = 0;
  for(i = 0; i < MAX_NODE_NUM; ++i) {
    LOG(INFO, "Node id: %d", cluster_info.node_info[i].node_id);
    for(j = 0; j < MAX_GROUP_NUM_IN_NODE; ++j) {
        LOG(INFO, "Group: %d in node",
            cluster_info.node_info[i].group_info[j].group_id);
    }
  }
}

//void init_msg_center() {
//  init_system();
//  read_config();
////  print_cluster_info();
//}


void get_self_mac(uint8* mac){
  memcpy(mac, cluster_info.node_info[g_node_id].network_info.mac, MAC_ADDR_LEN);
}

void get_peer_mac(node_id_t node_id, uint8* mac) {
  memcpy(mac, cluster_info.node_info[node_id].network_info.mac, MAC_ADDR_LEN);
}

uint8 get_msg_center_group_id() {
  return g_msg_center_group_id;
}

uint16 get_msg_center_app_id() {
  return g_msg_center_app_id;
}


void get_serv_info(node_id_t node_id, int8* serv_ip, uint16* serv_port) {
  strcpy((char*)serv_ip, "127.0.0.1");
  *serv_port = (uint16)(BASE_PORT_NUM + node_id);
  LOG(INFO, "Node:%d, ip:%s, port:%d", node_id, serv_ip, *serv_port);
}

uint16 get_self_port() {
  return (uint16)(BASE_PORT_NUM + g_node_id);
}
