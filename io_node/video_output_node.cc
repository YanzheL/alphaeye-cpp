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

VideoOutputNode::VideoOutputNode(
    std::string name,
    int fps,
    int width,
    int height,
    std::string out_dir,
    int cooldown_secs,
    std::string file_format
) : name{name},
    fps_{fps},
    width_{width},
    height_{height},
    out_dir{out_dir},
    cooldown_init_val_{cooldown_secs * fps},
    cooldown_guard_{30 * fps},
    cur_cooldown_ct_{cooldown_init_val_},
    file_format_{file_format} {
  task_queue_.set_capacity(60);
  realtime_queue_.set_capacity(3 * fps);
  ff_procs_.set_capacity(30);
  realtime_writer_ = make_shared<cv::VideoWriter>(
      realtime_pipe_,
      cv::CAP_GSTREAMER,
      0,
      fps_,
      cv::Size{width_, height_}
  );
  cout << "Opening realtime pipeline [" << realtime_pipe_ << "]" << endl;
  if (!realtime_writer_->isOpened()) {
    cerr << "Cannot open realtime_writer_" << endl;
    exit(1);
  }
  worker_thread_ = thread(&VideoOutputNode::_worker, this);
  realtime_thread_ = thread(&VideoOutputNode::_realtime_worker, this);
  ff_thread_ = thread(&VideoOutputNode::_ff_gc, this);
  cout << "VideoOutputNode created" << endl;
}

VideoOutputNode::~VideoOutputNode() {
  stop_requested_ = true;
  if (enabled_) {
    _disable();
  }
  worker_thread_.join();
  realtime_thread_.join();
  ff_thread_.join();
  cout << "VideoOutputNode destroyed" << endl;
}

void VideoOutputNode::enable() {
  cur_cooldown_ct_ = cooldown_init_val_;
  std::lock_guard<std::mutex> lk(m_);
  cout << "Enabling recorder..." << endl;
  if (enabled_) {
    return;
  }
  cur_cooldown_guard_ = cooldown_guard_;
  _make_cur_writer();
  cout << "Recorder enabled" << endl;
  enabled_ = true;
}

void VideoOutputNode::_disable() {
  cout << "Disabling recorder..." << endl;
  if (!enabled_) {
    cerr << "Recorder is already disabled!" << endl;
    return;
  }
  motion_writer_ = nullptr;
  _ffmpeg_worker(fps_, cur_file_, cur_file_ + ".mp4");
  cur_file_ = "";
  enabled_ = false;
  cur_cooldown_guard_ = cooldown_guard_;
  cur_cooldown_ct_ = cooldown_init_val_;
}

void VideoOutputNode::_worker() {
  cout << "Recorder _worker thread started" << endl;
  while (true) {
    cv::Mat data;
    if (task_queue_.try_pop(data)) {
      process(data);
    } else if (stop_requested_) {
      break;
    } else {
//      cout << "_worker blocking" << endl;
    }
  }
  cout << "Recorder _worker thread exited" << endl;
}

void VideoOutputNode::_realtime_worker() {
  cout << "Recorder _realtime_worker thread started" << endl;
  while (true) {
    cv::Mat data;
    if (realtime_queue_.try_pop(data)) {
      if (realtime_enabled_) realtime_writer_->write(data); // Record frame anyway.
    } else if (stop_requested_) {
      break;
    } else {
//      cout << "_realtime_worker blocking" << endl;
    }
  }
  cout << "Recorder _realtime_worker thread exited" << endl;
}

void VideoOutputNode::process(cv::Mat frame) {
  std::lock_guard<std::mutex> lk(m_);
  if (!enabled_) {
    return;
  }
  if (cur_cooldown_ct_) { // We are cooling down
//      cout << "state 1, ct=" << cur_cooldown_ct_ << ", guard=" << cur_cooldown_guard_ << endl;
    motion_writer_->write(frame);
    --cur_cooldown_ct_;
    if (cur_cooldown_guard_ != 0)--cur_cooldown_guard_;
  } else if (cur_cooldown_guard_) { // Cooling down finished, but the duration is too short.
//      cout << "state 2, ct=" << cur_cooldown_ct_ << ", guard=" << cur_cooldown_guard_ << endl;
    cur_cooldown_ct_ = cooldown_init_val_; // Re-init counter, expecting new motions.
    --cur_cooldown_guard_;
  } else { // Cooling down finished, and min duration fulfilled. Now we can write video back to disk.
//      cout << "state 3, ct=" << cur_cooldown_ct_ << ", guard=" << cur_cooldown_guard_ << endl;
    _disable();
  }
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
    ff_procs_.push(make_pair(pid, input));
  } else {
    cerr << "FFmpeg worker process fork failed" << endl;
  }
}

void VideoOutputNode::_ff_gc() {
  cout << "FFmpeg GC thread started" << endl;
  while (true) {
    std::pair<int, string> data;
    if (ff_procs_.try_pop(data)) {
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
void VideoOutputNode::_make_cur_writer() {
  time_t ttm = std::time(nullptr);
  tm *ltm = localtime(&ttm);
  char buffer[100] = {0};
  strftime(buffer, sizeof(buffer), file_format_.c_str(), ltm);
  fs::path dir(out_dir);
  fs::path file(buffer);
  fs::path full_path = dir / file;
  cur_file_ = full_path.string();
  stringstream pipeline_spec_buf;
  pipeline_spec_buf << motion_pipe_ << cur_file_ << " ";
  cout << "cur_file_=" << cur_file_ << endl;
  std::string pipeline_spec = pipeline_spec_buf.str();
  cout << "Opening motion recording pipeline [" << pipeline_spec << "]" << endl;
  motion_writer_ = make_shared<cv::VideoWriter>(
      pipeline_spec,
      cv::CAP_GSTREAMER,
      0,
      fps_,
      cv::Size{width_, height_}
  );
  if (!motion_writer_->isOpened()) {
    cerr << "Cannot open VideoWriter" << endl;
    return;
  }
}
void VideoOutputNode::put(cv::Mat frame) {
  bool rc = realtime_queue_.try_push(frame);
//  if (!rc) cout << "realtime_put blocking" << endl;
//  else cout << "realtime_put success" << endl;
  task_queue_.push(frame);
}

}
