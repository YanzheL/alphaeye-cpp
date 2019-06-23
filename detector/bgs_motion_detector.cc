// =======================================================================
// Copyright (c) 2016-2020 by LI YANZHE. All rights reserved.
// Author: LI YANZHE <lee.yanzhe@yanzhe.org>
// Created by LI YANZHE on 6/23/19.
// =======================================================================
#include "bgs_motion_detector.h"
#include <opencv2/bgsegm.hpp>
#include <iostream>
#include "util/util.h"

using namespace std;
using namespace cv;
namespace alphaeye {

BGSMotionDetector::BGSMotionDetector(VideoOutputNode *recorder,
                                     int sample_interval,
                                     int history,
                                     double threshold,
                                     int cooldown_secs,
                                     bool enable_roi_drawing)
    : MotionDetector(recorder),
      history_{history},
      threshold_{threshold},
      cooldown_secs_{cooldown_secs},
      sample_interval_{sample_interval},
      enable_roi_drawing_{enable_roi_drawing} {
  engine_ = cv::bgsegm::createBackgroundSubtractorMOG();
  stop_requested_ = false;
  worker_thread_ = thread(&BGSMotionDetector::_worker, this);
  cout << "BGSMotionDetector created" << endl;
}

void BGSMotionDetector::_worker() {
  cout << "Detector worker thread started" << endl;
  while (true) {
    cv::Mat task;
    if (task_queue_.pop(task)) {
      _analyze(task);
    } else if (stop_requested_) {
      break;
    }
  }
  cout << "Detector worker thread exited" << endl;
}
void BGSMotionDetector::_motionStop() {
  cout << "Motion stopped" << endl;
  motion_started_ = false;
  recorder_->disable();
  sum_prob_ = 0;
  cur_motion_frames_ = 1;
}

void BGSMotionDetector::_analyze(const cv::Mat &frame) {
  double cur_time = epoch_time();
  if (!enabled_) {
    return;
  }
  frame.copyTo(last_result_);
  double cur_avg_prob = sum_prob_ / cur_motion_frames_;
  if (sample_ct_ != 0) {
    if (motion_started_) {
      recorder_->put(last_result_, cur_avg_prob);
    }
  } else {
    Mat gray;
    cvtColor(frame, gray, COLOR_BGR2GRAY);
    GaussianBlur(gray, gray, {21, 21}, 0.0);
    engine_->apply(gray, last_mask_);
    long n_pixels = last_mask_.size().area();
    double prob = countNonZero(last_mask_) * 1000.0 / n_pixels;
    if (prob > threshold_) {
      last_time_ = cur_time;
      if (!motion_started_) {
        cout << "Motion detected, prob = " << prob << endl;
        motion_started_ = true;
        cur_start_ = cur_time;
        recorder_->enable();
      }
      sum_prob_ += prob;
      ++cur_motion_frames_;
      recorder_->put(last_result_, cur_avg_prob);
    } else if (motion_started_) {
      total_motion_duration_ = cur_time - cur_start_;
      recorder_->put(last_result_, cur_avg_prob);
      double cur_cooldown = cur_time - last_time_;
      if (cur_cooldown > cooldown_secs_ && total_motion_duration_ > 30) {
        _motionStop();
      }
    }
  }
  double t2 = epoch_time();
  total_time_ += t2 - cur_time;
  sample_ct_ = (sample_ct_ + 1) % sample_interval_;
}

void BGSMotionDetector::analyze(const cv::Mat &frame) {
  while (!task_queue_.push(frame)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

BGSMotionDetector::~BGSMotionDetector() {
  stop_requested_ = true;
  worker_thread_.join();
  recorder_->disable();
  if (enabled_) {
    disable();
  }
  cout << "BGSMotionDetector destroyed" << endl;
}

}
