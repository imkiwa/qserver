#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <sys/types.h>

enum {
  SERVEROPT_LOG = 0x00000001,
};

typedef struct qserver_s qserver_t;
typedef struct qserver_data_s qserver_data_t;
typedef struct qserver_table_s qserver_table_t;

struct qserver_data_s
{
  int fd;
  struct sockaddr_in addr;
  socklen_t addr_len;
  
  ssize_t buffer_bytes;
  char *buffer;
  void *userdata;
};

struct qserver_table_s
{
  void (*on_data)(qserver_data_t *cb);
  void (*on_reply)(qserver_data_t *cb);
  void (*on_open)(qserver_data_t *cb);
  void (*on_close)(qserver_data_t *cb);
};

struct qserver_s
{
  int opt;
  int status;
  int fd;
  qserver_table_t callbacks;
};


int qserver_loop(qserver_t *server);

int qserver_running(qserver_t *server);

int qserver_shutdown(qserver_t *server, int flag);
