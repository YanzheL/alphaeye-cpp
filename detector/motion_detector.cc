// =======================================================================
// Copyright (c) 2016-2020 by LI YANZHE. All rights reserved.
// Author: LI YANZHE <lee.yanzhe@yanzhe.org>
// Created by LI YANZHE on 6/23/19.
// =======================================================================
#include "motion_detector.h"
#include <iostream>

using namespace std;

namespace alphaeye {

//void MotionDetector::_invokeHooks(const std::string event, void *args)

void MotionDetector::enable() {
  std::lock_guard<std::mutex> lk(m_);
  if (enabled_) {
    cout << "Detector is already enabled!" << endl;
    return;
  }
  enabled_ = true;
  cout << "Detector enabled" << endl;
}

//void MotionDetector::registerEventHook(std::string event, void *hook) {
//  auto it = hooks_.find(event);
//  if (it == hooks_.end()) {
//    cout << "Cannot find event <" << event << ">" << endl;
//    return;
//  }
//  it->second.push_back(hook);
//}

void MotionDetector::disable() {
  std::lock_guard<std::mutex> lk(m_);
  if (!enabled_) {
    cout << "Detector is already disabled!" << endl;
    return;
  }
  enabled_ = false;
  has_motion_ = false;
  cout << "MotionDetector disabled" << endl;
}

MotionDetector::MotionDetector(VideoOutputNode *recorder) : recorder_{recorder} {
  cout << "MotionDetector created" << endl;
}

}
