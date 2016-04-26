#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <sys/types.h>

enum {
  BUFFER_SIZE = 1024,
};

enum {

};

typedef struct
{
  int fd;
  struct sockaddr_in addr;
  socklen_t addr_len;
  
  ssize_t buffer_bytes;
  char buffer[BUFFER_SIZE];
  void *userdata;
} cb_data_t;

typedef struct
{
  void (*on_data)(cb_data_t *cb);
  void (*on_reply)(cb_data_t *cb);
  void (*on_open)(cb_data_t *cb);
  void (*on_close)(cb_data_t *cb);
} cb_table_t;


int sock_get(int port);
int sock_nonblock();
int server_loop();
int server_running();
int server_shutdown();
