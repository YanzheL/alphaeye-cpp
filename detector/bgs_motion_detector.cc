// =======================================================================
// Copyright (c) 2016-2020 by LI YANZHE. All rights reserved.
// Author: LI YANZHE <lee.yanzhe@yanzhe.org>
// Created by LI YANZHE on 6/23/19.
// =======================================================================
#include "bgs_motion_detector.h"
#include <iostream>
#include "util/util.h"

using namespace std;
using namespace cv;
namespace alphaeye {

BGSMotionDetector::BGSMotionDetector(
    VideoOutputNode *recorder,
    int sample_interval,
    double threshold,
    bool enable_roi_drawing,
    int history,
    int nmixtures,
    double backgroundRatio,
    double noiseSigma
)
    : MotionDetector(recorder),
      history_{history},
      threshold_{threshold},
      sample_interval_{sample_interval},
      enable_roi_drawing_{enable_roi_drawing} {
  task_queue_.set_capacity(30);
  engine_ = cv::bgsegm::createBackgroundSubtractorMOG(history, nmixtures, backgroundRatio, noiseSigma);
  stop_requested_ = false;
  analyze_thread_ = thread(&BGSMotionDetector::_analyze_worker, this);
  cout << "BGSMotionDetector created" << endl;
}

void BGSMotionDetector::_analyze_worker() {
  cout << "Detector analyze worker thread started" << endl;
  cv::Mat task;
  while (true) {
    if (task_queue_.try_pop(task))
      _analyze(task);
    else if (stop_requested_)
      break;
  }
  cout << "Detector analyze worker thread exited" << endl;
}

void BGSMotionDetector::_analyze(const cv::Mat &frame) {
  if (!enabled_)
    return;
  frame.copyTo(last_result_);
  Mat gray;
  cvtColor(frame, gray, COLOR_BGR2GRAY);
  GaussianBlur(gray, gray, {21, 21}, 0.0);
  engine_->apply(gray, last_mask_);
  double prob = countNonZero(last_mask_) * 1000.0 / last_mask_.size().area();
  if (prob > threshold_) {
    cout << "Motion detected, prob = " << prob << endl;
    has_motion_ = true;
    recorder_->enable(prob);
  } else if (has_motion_) {
    has_motion_ = false;
  }
}

void BGSMotionDetector::analyze(const cv::Mat &frame) {
  recorder_->put(frame);
  if (sample_ct_ == 0) {
    task_queue_.push(frame);
  }
  sample_ct_ = (sample_ct_ + 1) % sample_interval_;
}

BGSMotionDetector::~BGSMotionDetector() {
  stop_requested_ = true;
  analyze_thread_.join();
  if (enabled_) {
    disable();
  }
  cout << "BGSMotionDetector destroyed" << endl;
}

}
