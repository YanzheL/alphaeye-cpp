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
#include <sys/ioctl.h>
using namespace std;

namespace alphaeye {

void DetectorController::_worker() {
  cout << "DetectorController worker thread started" << endl;
  int addrlen = sizeof(address);
  const int bsize = 2;
  char buffer[bsize] = {0};
  fd_set master_set, working_set;
  FD_ZERO(&master_set);
  struct timeval timeout;
  int max_sd = socket_fd_;
  int rc, desc_ready;
  FD_SET(socket_fd_, &master_set);
  bool close_conn;
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
  while (!stop_requested_) {
    memcpy(&working_set, &master_set, sizeof(master_set));
    rc = select(max_sd + 1, &working_set, NULL, NULL, &timeout);
    if (rc < 0) {
      cerr << "\tselect() failed" << endl;
      continue;
    } else if (rc == 0) {
      continue;
    }
    desc_ready = rc;
    for (int i = 0; i <= max_sd && desc_ready > 0; ++i) {
      if (FD_ISSET(i, &working_set)) {
        desc_ready -= 1;
        if (i == socket_fd_) {
//          cout << "Listening socket is readable!" << endl;
          int conn;
          do {
            conn = accept(socket_fd_, NULL, NULL);
            if (conn < 0) {
              if (errno != EWOULDBLOCK) {
                cerr << "\taccept() failed." << endl;
                stop_requested_ = true;
              }
              break;
            }
            cout << "\tNew incoming connection [" << conn << "]." << endl;
            FD_SET(conn, &master_set);
            if (conn > max_sd)
              max_sd = conn;
          } while (conn != -1);
        } else {
          cout << "\tDescriptor [" << i << "] is readable." << endl;
          close_conn = false;
          while (true) {
            rc = recv(i, buffer, sizeof(buffer), 0);
            if (rc < 0) {
              if (errno != EWOULDBLOCK) {
                cerr << "\trecv() failed." << endl;
                close_conn = true;
              }
              break;
            }
            if (rc == 0) {
              cerr << "\tConnection closed." << endl;
              close_conn = true;
              break;
            }
            int len = rc;
            printf("\t[%d] bytes received, data = [%d].\n", len, buffer[0]);
            _handle_data(buffer);
            rc = send(i, buffer, 2, 0);
            memset(buffer, 0, bsize);
            if (rc < 0) {
              cerr << "\tsend() failed." << endl;
              close_conn = true;
              break;
            }
          }
          if (close_conn) {
//            cout << "Closing " << i << endl;
            close(i);
            FD_CLR(i, &master_set);
            if (i == max_sd) {
              while (!FD_ISSET(max_sd, &master_set))
                max_sd -= 1;
            }
          }
        }
      }
    }
  }
  cout << "DetectorController worker thread exited" << endl;
}

DetectorController::DetectorController(std::shared_ptr<MotionDetector> detector, int port)
    : detector_{detector} {
  int rc;
  rc = socket(AF_INET, SOCK_STREAM, 0);
  if (rc == 0) {
    cerr << "socket failed" << endl;
    exit(1);
  }
  socket_fd_ = rc;
  int opt = 1, on = 1;
  rc = setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                  &opt, sizeof(opt));
  if (rc < 0) {
    cerr << "setsockopt failed" << endl;
    exit(1);
  }
  rc = ioctl(socket_fd_, FIONBIO, (char *) &on);
  if (rc < 0) {
    perror("ioctl() failed");
    close(socket_fd_);
    exit(1);
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
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
  worker_thread_.join();
  cout << "DetectorController destroyed" << endl;
}
void DetectorController::_handle_data(char *data) {
  if (data[0] == 1u) {
    detector_->enable();
    data[0] = 1;
    data[1] = 1;
  } else if (data[0] == 2u) {
    detector_->disable();
    data[0] = 2;
    data[1] = 1;
  } else if (data[0] == 3u) {
    data[0] = 3;
    data[1] = detector_->isEnabled();
  } else if (data[0] == 4u) {
    data[0] = 4;
    data[1] = detector_->isMotionStarted();
  } else {
    data[0] = 0xf;
    data[1] = 0xf;
  }
}

}
