// =======================================================================
// Copyright (c) 2016-2020 by LI YANZHE. All rights reserved.
// Author: LI YANZHE <lee.yanzhe@yanzhe.org>
// Created by LI YANZHE on 6/23/19.
// =======================================================================
#include "detector_controller.h"
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
using namespace std;

namespace alphaeye {
void DetectorController::_worker() {
  cout << "DetectorController worker thread started" << endl;
  int addrlen = sizeof(address);
  const int bsize = 128;
  char buffer[bsize] = {0};
  while (!stop_requested_) {
    int conn = accept(socket_fd_, (struct sockaddr *) &address, (socklen_t *) &addrlen);
    if (conn < 0) {
      cerr << "accept failed" << endl;
      return;
    }
    cout << "Accepting new connection" << endl;
    while (recv(conn, buffer, bsize, 0) > 0) {
      cout << "Data received = " << string(buffer) << endl;
      if (strcmp(buffer, "enable") == 0) {
        detector_->enable();
        send(conn, "ok", 2, 0);
      } else if (strcmp(buffer, "disable") == 0) {
        detector_->disable();
        send(conn, "ok", 2, 0);
      } else {
        send(conn, "unknown", 7, 0);
      }
      memset(buffer, 0, bsize);
    }
  }
  cout << "DetectorController worker thread exited" << endl;
}

DetectorController::DetectorController(std::shared_ptr<MotionDetector> detector, int port)
    : detector_{detector} {
  socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd_ == 0) {
    cerr << "socket failed" << endl;
    return;
  }
  int opt = 1;
  if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                 &opt, sizeof(opt))) {
    cerr << "setsockopt failed" << endl;
    return;
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  // Forcefully attaching socket to the port 8080
  if (bind(socket_fd_, (struct sockaddr *) &address,
           sizeof(address)) < 0) {
    cerr << "setsockopt failed" << endl;
    return;
  }
  if (listen(socket_fd_, 5) < 0) {
    cerr << "setsockopt failed" << endl;
    return;
  }
  worker_thread_ = thread(&DetectorController::_worker, this);
}

DetectorController::~DetectorController() {
  stop_requested_ = true;
  close(socket_fd_);
  worker_thread_.join();
}
}
