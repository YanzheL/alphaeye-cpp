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
#include <boost/lockfree/spsc_queue.hpp>
#include <tbb/concurrent_queue.h>
#include <thread>

namespace alphaeye {

class BGSMotionDetector : public MotionDetector {
 public:

  explicit BGSMotionDetector(
      VideoOutputNode *recorder,
      int sample_interval = 2,
      double threshold = 5,
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

  void _analyze(const cv::Mat &frame);

 protected:
  int history_;
  double threshold_;
  int sample_interval_;
  bool enable_roi_drawing_ = false;
  cv::Mat last_result_;
  cv::Mat last_mask_;
  int sample_ct_ = 0;
  bool stop_requested_ = false;
  std::shared_ptr<cv::BackgroundSubtractor> engine_;
  std::thread analyze_thread_;
//  boost::lockfree::spsc_queue<cv::Mat, boost::lockfree::capacity<30>> task_queue_;
  tbb::concurrent_bounded_queue<cv::Mat> task_queue_;
};

}

#endif //ALPHAEYE_CPP_BGS_MOTION_DETECTOR_H
