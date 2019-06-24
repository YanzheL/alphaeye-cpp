// =======================================================================
// Copyright (c) 2016-2020 by LI YANZHE. All rights reserved.
// Author: LI YANZHE <lee.yanzhe@yanzhe.org>
// Created by LI YANZHE on 6/23/19.
// =======================================================================
#include "video_output_node.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include "util/util.h"
#include <boost/filesystem.hpp>
#include <wait.h>
using namespace std;
namespace fs = boost::filesystem;

namespace alphaeye {

void VideoOutputNode::enable() {
  std::lock_guard<std::mutex> lk(m);
  if (enabled_) {
    cerr << "Recorder is already enabled" << endl;
    return;
  }
  cout << "Enabling recorder..." << endl;
  t_enable_ = epoch_time();
  time_t ttm = std::time(nullptr);
  tm *ltm = localtime(&ttm);
  char buffer[100];
  strftime(buffer, sizeof(buffer), file_format_.c_str(), ltm);
  fs::path dir(out_dir_);
  fs::path file(buffer);
  fs::path full_path = dir / file;
  cur_file_ = full_path.string();
  char pipeline_spec_buf[1000];
//  sprintf(pipeline_spec_buf, pipeline_format.c_str(), fps_, cur_file_.c_str());
  sprintf(pipeline_spec_buf, pipeline_format.c_str(), cur_file_.c_str());
  std::string pipeline_spec{pipeline_spec_buf};
  cout << "Opening pipeline [" << pipeline_spec << "]" << endl;
  cur_writer_ = make_shared<cv::VideoWriter>(
      pipeline_spec,
      cv::CAP_GSTREAMER,
      0,
      fps_,
      cv::Size{width_, height_}
  );

  if (!cur_writer_->isOpened()) {
    cerr << "Cannot open VideoWriter" << endl;
    return;
  }
  cout << "Recorder enabled" << endl;
  enabled_ = true;
}

void VideoOutputNode::disable() {
  std::lock_guard<std::mutex> lk(m);
  if (!enabled_) {
    cout << "Recorder is already disabled!" << endl;
    return;
  }
  double end_t = epoch_time();
  cur_writer_ = nullptr;
  _ffmpeg_worker(fps_, cur_file_, cur_file_ + ".mp4");
  cur_file_ = "";
  enabled_ = false;
}

void VideoOutputNode::_worker() {
  cout << "Recorder worker thread started" << endl;
  while (true) {
    std::pair<cv::Mat, double> data;
    if (task_queue_.pop(data)) {
      process(data.first, data.second);
    } else if (stop_requested_) {
      break;
    }
  }
  cout << "Recorder worker thread exited" << endl;
}

void VideoOutputNode::process(cv::Mat frame, double prob) {
  std::lock_guard<std::mutex> lk(m);
  if (!enabled_) {
//    cerr << "Recorder is not enabled" << endl;
    return;
  }
  avg_prob_ = prob;
  cur_writer_->write(frame);

}

void VideoOutputNode::_ffmpeg_worker(int fps, std::string input, std::string output) {
  int pid = fork();
  if (pid == 0) {
    cout << "Forking FFmpeg worker process" << endl;
    int ret = execlp(
        "ffmpeg",
        "ffmpeg",
        "-hide_banner",
        "-y",
        "-framerate",
        std::to_string(fps).c_str(),
        "-i",
        input.c_str(),
        "-c",
        "copy",
        output.c_str(),
        NULL
    );
    cout << "Unexpected execlp() result, ret = " << ret << endl;
    exit(0);
  } else if (pid > 0) {
    cout << "Forked FFmpeg worker process, pid = " << pid << endl;
    while (!ff_procs_.push(make_pair(pid, input)));
  } else {
    cerr << "FFmpeg worker process fork failed" << endl;
  }
}

VideoOutputNode::VideoOutputNode(std::string name,
                                 int fps,
                                 int width,
                                 int height,
                                 std::string out_dir,
                                 std::string file_format)
    : name{name},
      fps_{fps},
      width_{width},
      height_{height},
      out_dir_{out_dir},
      file_format_{file_format} {
  worker_thread_ = thread(&VideoOutputNode::_worker, this);
  ff_thread_ = thread(&VideoOutputNode::_ff_gc, this);
  cout << "VideoOutputNode created" << endl;
}

void VideoOutputNode::_ff_gc() {
  cout << "FFmpeg GC thread started" << endl;
  while (true) {
    std::pair<int, string> data;
    if (ff_procs_.pop(data)) {
      int pid = data.first;
      string file = data.second;
      int status;
      if (waitpid(pid, &status, WNOHANG) > 0) {
        cout << "Cleaned subprocess, pid = " << pid << ", file = " << file << endl;
        if (remove(file.c_str()) != 0) {
          cerr << "Error removing file " << file << endl;
        }
      } else {
        ff_procs_.push(data);
      }

    } else if (stop_requested_) {
      break;
    } else {
      std::this_thread::sleep_for(std::chrono::seconds(3));
    }
  }
  cout << "FFmpeg GC thread exited" << endl;
}

VideoOutputNode::~VideoOutputNode() {
  stop_requested_ = true;
  if (enabled_) {
    disable();
  }
  worker_thread_.join();
  ff_thread_.join();
  cout << "VideoOutputNode destroyed" << endl;
}

}
