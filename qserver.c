#include "qserver.h"

#define CALL(fn, arg...) \
  if (fn) fn(arg)

enum {
  MAX_EVENT = 40, MAX_POLL = 256,
  BUFFER_SIZE = 1024,
};


static qserver_data_t* cb_data_new()
{
  qserver_data_t *data = (qserver_data_t*) malloc(sizeof(qserver_data_t));
  if (!data) {
    return NULL;
  }
  
  data->buffer = (char*) malloc(sizeof(char) * BUFFER_SIZE);
  if (!data->buffer) {
    free(data);
    return NULL;
  }
  
  return data;
}


static void cb_data_delete(qserver_data_t *cb)
{
  if (cb) {
    free(cb->buffer);
    free(cb);
  }
}

static int process_io(qserver_t *server, int ep, struct epoll_event *ev)
{
  qserver_data_t *data = ev->data.ptr;
  if (!data) {
    return -1;
  }
  
  int fd = data->fd;
  ssize_t n = 0;
  struct epoll_event nev;
  
  nev.data.ptr = data;
  nev.events = EPOLLET;
  
  if (ev->events & EPOLLIN) {
    if ((n = read(fd, data->buffer, BUFFER_SIZE)) == 0) {
      // connection disconnected
      printf(":: Connection disconnected, removing from epoll\n");
      close(fd);
      data->fd = 0;
      CALL(server->callbacks.on_close, data);
      cb_data_delete(data);
      epoll_ctl(ep, EPOLL_CTL_DEL, fd, NULL);
      return 0;
    }
    
    data->buffer_bytes = n;
    nev.events |= EPOLLOUT;
    epoll_ctl(ep, EPOLL_CTL_MOD, fd, &nev);
    
    CALL(server->callbacks.on_data, data);
  }
  
  if (ev->events & EPOLLOUT) {
    CALL(server->callbacks.on_reply, data);
    nev.events |= EPOLLIN;
    epoll_ctl(ep, EPOLL_CTL_MOD, fd, &nev);
  }
  
  return 0;
}


int qserver_shutdown(qserver_t *server, int flag)
{
  if (flag) {
    server->status = flag;
  }
  
  return server->status;
}


int qserver_running(qserver_t *server)
{
  return !qserver_shutdown(server, 0);
}


int qserver_loop(qserver_t *server)
{
  if (!server || server->fd < 0) {
    return -1;
  }
  
  int ep = epoll_create(MAX_POLL);
  if (ep < 0) {
    return -1;
  }
  
  int sock = server->fd;
  int nfds = 0;
  struct epoll_event ev;
  struct epoll_event events[MAX_EVENT];
  
  memset(&ev, '\0', sizeof(ev));
  memset(events, '\0', sizeof(events));
  
  ev.events = EPOLLIN | EPOLLET;
  epoll_ctl(ep, EPOLL_CTL_ADD, sock, &ev);
  
  while (qserver_running(server)) {
    nfds = epoll_wait(ep, events, MAX_EVENT, 0);
    for (int i = 0; i < nfds; ++i) {
      qserver_data_t *curdata = events[i].data.ptr;
      
      // New client
      if (!curdata) {
        qserver_data_t *cb = cb_data_new();
        if (!cb) {
          fprintf(stderr, "W: cb_data_new() failed: %s\n", strerror(errno));
          continue;
        }
         
        int conn = accept(sock, (struct sockaddr*) &cb->addr, &cb->addr_len);
        if (conn < 0) {
          fprintf(stderr, "W: accept() failed, %s\n", strerror(errno));
          continue;
        }
        
        printf(":: New connection from: %s\n", inet_ntoa(cb->addr.sin_addr));
        cb->fd = conn;
        ev.data.ptr = (void*) cb;
        ev.events = EPOLLIN | EPOLLET;
        epoll_ctl(ep, EPOLL_CTL_ADD, conn, &ev);
        CALL(server->callbacks.on_open, cb);

      // I/O Operations
      } else {
        process_io(server, ep, events + i);
      }
    } // for
  } // while
  
  close(ep);
  return 0;
}

