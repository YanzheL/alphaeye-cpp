// =======================================================================
// Copyright (c) 2016-2020 by LI YANZHE. All rights reserved.
// Author: LI YANZHE <lee.yanzhe@yanzhe.org>
// Created by LI YANZHE on 6/26/19.
// =======================================================================
#ifndef ALPHAEYE_CPP_DB_WRITER_H
#define ALPHAEYE_CPP_DB_WRITER_H
#include <sqlite3.h>
#include <string>
namespace alphaeye {

class DBWriter {
 public:
  explicit DBWriter(std::string location);

  bool write_video_info(time_t tstart, time_t tstop, double prob, std::string filename);

  ~DBWriter();

 protected:
  sqlite3 *db_;
  std::string t_format_ = "%Y-%m-%d_%H-%M-%S";

  void select_stmt(const char *stmt);

};

}

#endif //ALPHAEYE_CPP_DB_WRITER_H
