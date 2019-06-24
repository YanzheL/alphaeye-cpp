// =======================================================================
// Copyright (c) 2016-2020 by LI YANZHE. All rights reserved.
// Author: LI YANZHE <lee.yanzhe@yanzhe.org>
// Created by LI YANZHE on 6/23/19.
// =======================================================================
#ifndef ALPHAEYE_CPP_DETECTOR_CONTROLLER_H
#define ALPHAEYE_CPP_DETECTOR_CONTROLLER_H
#include <memory>
#include "detector/motion_detector.h"
#include <sys/socket.h>
#include <thread>
#include <netinet/in.h>
namespace alphaeye {
class DetectorController {
 public:
  explicit DetectorController(
      std::shared_ptr<MotionDetector> detector,
      int port = 57718
  );

  ~DetectorController();

 protected:

  void _worker();

  std::string _handle_data(std::string data);

 protected:

  int socket_fd_;
  sockaddr_in address;
  std::thread worker_thread_;
  bool stop_requested_ = false;
  std::shared_ptr<MotionDetector> detector_;

};
}

#endif //ALPHAEYE_CPP_DETECTOR_CONTROLLER_H
