// =======================================================================
// Copyright (c) 2016-2020 by LI YANZHE. All rights reserved.
// Author: LI YANZHE <lee.yanzhe@yanzhe.org>
// Created by LI YANZHE on 6/23/19.
// =======================================================================
#ifndef ALPHAEYE_CPP_BGS_MOTION_DETECTOR_H
#define ALPHAEYE_CPP_BGS_MOTION_DETECTOR_H

#include "motion_detector.h"
#include <opencv2/core/mat.hpp>
#include <opencv2/bgsegm.hpp>
#include "boost/lockfree/spsc_queue.hpp"
#include <thread>

namespace alphaeye {

class BGSMotionDetector : public MotionDetector {
 public:

  explicit BGSMotionDetector(
      VideoOutputNode *recorder,
      int sample_interval = 2,
      double threshold = 5,
      int cooldown_secs = 5,
      bool enable_roi_drawing = false,
      int history = 50,
      int nmixtures = 5,
      double backgroundRatio = 0.7,
      double noiseSigma = 0.0
  );

  ~BGSMotionDetector() override;

  void analyze(const cv::Mat &frame) override;

 protected:

  void _analyze_worker();

  void _recording_worker();

  void _motionStop() override;

  void _analyze(const cv::Mat &frame);

 protected:

  double total_time_;
  double total_motion_duration_;
  int history_;
  double threshold_;
  int sample_interval_;
  int cooldown_secs_;
  bool enable_roi_drawing_ = false;
  double cur_start_;
  double last_time_;
  cv::Mat last_result_;
  cv::Mat last_mask_;
  double sum_prob_;
  unsigned long cur_motion_frames_ = 1;
  int sample_ct_;
  bool stop_requested_ = false;
  std::shared_ptr<cv::BackgroundSubtractor> engine_;
  std::thread analyze_thread_;
  boost::lockfree::spsc_queue<cv::Mat, boost::lockfree::capacity<30>> task_queue_;
};

}

#endif //ALPHAEYE_CPP_BGS_MOTION_DETECTOR_H
