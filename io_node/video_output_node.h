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

namespace alphaeye {
class VideoOutputNode {
 public:
  explicit VideoOutputNode(
      std::string name,
      int fps,
      int width,
      int height,
      std::string out_dir,
      std::string file_format = "%Y-%m-%d_%H-%M-%S.h264"
  );

  ~VideoOutputNode();

  inline bool put(cv::Mat frame, double prob) {
    return task_queue_.push(std::make_pair(frame, prob));
  }

  void enable();

  void disable();

  void process(cv::Mat frame, double prob);

 public:

  std::string name;

  std::string out_dir;

 protected:

  void _worker();

  void _ffmpeg_worker(int fps, std::string input, std::string output);

  void _ff_gc();

 protected:
//  std::vector<VideoOutputNode *> output_nodes;
  bool enabled_ = false;
  int fps_;
  int width_;
  int height_;
  std::string file_format_;
  std::string cur_file_;
  std::thread worker_thread_;
  std::thread ff_thread_;
  std::mutex m_;
  bool stop_requested_ = false;
  double avg_prob_ = 0;
  const std::string motion_pipe_ = " appsrc do-timestamp=true is-live=true format=time !"
                                   " videoconvert !"
                                   " video/x-raw, format=I420 !"
                                   " queue !"
                                   " omxh264enc target-bitrate=15000000 control-rate=variable !"
                                   " filesink sync=true location=%s ";
  const std::string realtime_pipe_ = " appsrc do-timestamp=true is-live=true format=time !"
                                     " videoconvert !"
                                     " video/x-raw, format=BGR !"
                                     " queue !"
                                     " rtpvrawpay !"
                                     " udpsink port=7758 host=localhost";
  std::shared_ptr<cv::VideoWriter> motion_writer_;
  std::shared_ptr<cv::VideoWriter> realtime_writer_;
  boost::lockfree::spsc_queue<std::pair<cv::Mat, double>, boost::lockfree::capacity<30>> task_queue_;
  boost::lockfree::spsc_queue<std::pair<int, std::string>, boost::lockfree::capacity<30>> ff_procs_;
};
}

#endif //ALPHAEYE_CPP_VIDEO_OUTPUT_NODE_H
