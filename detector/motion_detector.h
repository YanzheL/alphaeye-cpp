// =======================================================================
// Copyright (c) 2016-2020 by LI YANZHE. All rights reserved.
// Author: LI YANZHE <lee.yanzhe@yanzhe.org>
// Created by LI YANZHE on 6/23/19.
// =======================================================================
#ifndef ALPHAEYE_CPP_MOTION_DETECTOR_H
#define ALPHAEYE_CPP_MOTION_DETECTOR_H

#include <vector>
#include <string>
#include <opencv2/core/mat.hpp>
#include "io_node/video_output_node.h"

namespace alphaeye {

class MotionDetector {

 public:

  explicit MotionDetector(VideoOutputNode *recorder);

  virtual ~MotionDetector() = default;

  inline bool isMotionStarted() { return has_motion_; }

  inline bool isEnabled() { return enabled_; }

  virtual void enable();

  virtual void disable();

  virtual void analyze(const cv::Mat &frame) = 0;

 protected:

 protected:

  bool has_motion_ = false;
  bool enabled_ = false;
  std::shared_ptr<VideoOutputNode> recorder_;
  std::mutex m_;

};
}

#endif //ALPHAEYE_CPP_MOTION_DETECTOR_H
