#include <signal.h>
#include <errno.h>
#include <string.h>

#include "qserver.h"
#include "qserver-extension.h"

#define UNUSED __attribute__((unused))

qserver_t *pserver;

void on_sigint(int sig UNUSED)
{
  if (qserver_running(pserver)) {
    printf(":: Reciving SIGINT, shutdown server\n");
    qserver_shutdown(pserver, 1);
  }
}

void on_data(qserver_data_t *cb UNUSED)
{
  // 处理客户端的消息
}


void on_reply(qserver_data_t *cb)
{
  const static char *msg = "Recived: ";
  // 回复客户端
  send(cb->fd, msg, strlen(msg), 0);
  send(cb->fd, cb->buffer, cb->buffer_bytes, 0);
}

int main(int argc UNUSED, char **argv UNUSED)
{
  signal(SIGINT, on_sigint);
  
  printf(":: Creating socket\n");
  int fd = qserverext_sock_get(3456);
  if (fd < 0) {
    fprintf(stderr, "E: socket() failed: %s\n", strerror(errno));
    return 1;
  }
  
  if (qserverext_sock_nonblock(fd) < 0) {
    fprintf(stderr, "E: fcntl() failed: %s\n", strerror(errno));
    close(fd);
    return 1;
  }
  
  printf(":: Listening on 0.0.0.0:3456\n");
  printf(":: Main looping\n");
  
  qserver_t server = {
    .fd = fd,
    .callbacks = {
      .on_data = on_data,
      .on_reply = on_reply,
    },
  };
  pserver = &server;
  
  qserver_loop(pserver);
  
  printf(":: Cleaning up\n");
  close(fd);
}

