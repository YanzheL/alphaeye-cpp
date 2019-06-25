// =======================================================================
// Copyright (c) 2016-2020 by LI YANZHE. All rights reserved.
// Author: LI YANZHE <lee.yanzhe@yanzhe.org>
// Created by LI YANZHE on 6/23/19.
// =======================================================================
#ifndef ALPHAEYE_CPP_VIDEO_OUTPUT_NODE_H
#define ALPHAEYE_CPP_VIDEO_OUTPUT_NODE_H
#include <string>
#include <vector>
#include <opencv2/core/mat.hpp>
#include <opencv2/videoio/videoio.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <thread>
#include <tbb/concurrent_queue.h>

namespace alphaeye {
class VideoOutputNode {
 public:
  VideoOutputNode(std::string name,
                  int fps,
                  int width,
                  int height,
                  std::string out_dir,
                  int cooldown_secs = 10,
                  std::string file_format = "%Y-%m-%d_%H-%M-%S.h264");

  ~VideoOutputNode();

  void put(cv::Mat frame);

  void enable();

  void process(cv::Mat frame);

 public:

  std::string name;

  std::string out_dir;

 protected:

  void _worker();

  void _realtime_worker();

  void _ffmpeg_worker(int fps, std::string input, std::string output);

  void _ff_gc();

  void _make_cur_writer();

  void _disable();

 protected:
//  std::vector<VideoOutputNode *> output_nodes;
  bool enabled_ = false;
  const int fps_;
  const int width_;
  const int height_;
  const std::string file_format_;
  std::string cur_file_;
  std::thread worker_thread_;
  std::thread realtime_thread_;
  std::thread ff_thread_;
  std::mutex m_;
  const int cooldown_init_val_;
  const int cooldown_guard_;
  int cur_cooldown_ct_;
  int cur_cooldown_guard_;
  bool stop_requested_ = false;
  bool realtime_enabled_ = true;
  const std::string motion_pipe_ = " appsrc do-timestamp=true is-live=true format=time !"
                                   " videoconvert !"
                                   " video/x-raw, format=I420 !"
                                   " queue !"
                                   " omxh264enc target-bitrate=3000000 control-rate=variable !"
                                   " filesink sync=true location=";
  const std::string realtime_pipe_ = " appsrc !"
                                     " queue !"
                                     " rtpvrawpay !"
                                     " udpsink port=7758 host=127.0.0.1 ";
  std::shared_ptr<cv::VideoWriter> motion_writer_;
  std::shared_ptr<cv::VideoWriter> realtime_writer_;
  tbb::concurrent_bounded_queue<cv::Mat> task_queue_;
  tbb::concurrent_bounded_queue<cv::Mat> realtime_queue_;
  tbb::concurrent_bounded_queue<std::pair<int, std::string>> ff_procs_;
};
}

#endif //ALPHAEYE_CPP_VIDEO_OUTPUT_NODE_H
