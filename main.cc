#include <iostream>
#include "util/util.h"
#include <raspicam/raspicam_cv.h>
#include <opencv2/core/mat.hpp>
#include "detector/bgs_motion_detector.h"
#include "io_node/video_output_node.h"
#include "controller/detector_controller.h"

using namespace alphaeye;
using namespace cv;
using namespace std;

int main() {
  int width = 1280;
  int height = 720;
  int fps = 24;
  int sample_interval = 10;
//  int width = 640;
//  int height = 480;
//  int fps = 30;
//  int sample_interval = 2;

  std::shared_ptr<MotionDetector> detector = make_shared<BGSMotionDetector>(
      new VideoOutputNode("out0", fps, width, height, "../data"),
      sample_interval);
  detector->enable();
  DetectorController controller(detector);
  raspicam::RaspiCam_Cv camera;
  cv::Mat frame;
  long ct = 0;
  camera.setVerticalFlip(true);
  camera.set(CAP_PROP_FORMAT, CV_8UC3);
  camera.set(CAP_PROP_FRAME_WIDTH, width);
  camera.set(CAP_PROP_FRAME_HEIGHT, height);
  camera.set(CAP_PROP_FPS, fps);
  cout << "Opening camera..." << endl;
  if (!camera.open()) {
    cerr << "Error opening the camera" << endl;
    return -1;
  }
  int stat_interval = 100;
  cout << "Warming up..." << endl;
  std::this_thread::sleep_for(std::chrono::seconds(2));
  //Start capture
  auto t_start = std::chrono::high_resolution_clock::now();
  auto t1 = t_start;
  while (true) {
    ++ct;
    camera.grab();
    camera.retrieve(frame);
    detector->analyze(frame);
    if (ct % stat_interval == 0) {
      auto t2 = std::chrono::high_resolution_clock::now();
      double dur = std::chrono::duration<double>(t2 - t1).count();
      cout << "captured "
           << ct
           << " frames, "
           << "FPS = "
           << stat_interval / dur
           << endl;
      t1 = t2;
    }
    if (ct == LLONG_MAX)break;
  }
  cout << "exiting" << endl;
  return 0;
}
