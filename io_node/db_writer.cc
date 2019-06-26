// =======================================================================
// Copyright (c) 2016-2020 by LI YANZHE. All rights reserved.
// Author: LI YANZHE <lee.yanzhe@yanzhe.org>
// Created by LI YANZHE on 6/26/19.
// =======================================================================
#include "db_writer.h"
#include "util/util.h"
#include <sqlite3.h>
#include <iostream>
#include <boost/filesystem.hpp>

#pragma temp_store_directory = 'directory-name';
using namespace std;
namespace alphaeye {

DBWriter::DBWriter(std::string location) {
  int rc;
//  sqlite3_temp_directory = "/tmp";
  rc = sqlite3_open(location.c_str(), &db_);
  if (rc) {
    cerr << "Can't open database:" << sqlite3_errmsg(db_) << endl;
    sqlite3_close(db_);
    exit(1);
  }
}
DBWriter::~DBWriter() {
  sqlite3_close(db_);
}
bool DBWriter::write_video_info(time_t tstart, time_t tstop, double prob, std::string filename) {
  static const char
      *stmt_s = "INSERT INTO videoinfo (start_time, stop_time, avg_motion_prob, location) VALUES (?,?,?,?)";
  sqlite3_stmt *stmt;
  int rc;
  rc = sqlite3_prepare_v2(db_, stmt_s, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    cerr << "Could not prepare statement." << endl;
    return false;
  }
  int pos = filename.rfind("/");
  if (pos != std::string::npos) {
    filename = filename.substr(pos + 1);
  }
  std::string sstart = lyz::format_time(tstart, t_format_);
  std::string sstop = lyz::format_time(tstop, t_format_);
  rc = sqlite3_bind_text(stmt, 1, sstart.c_str(), -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    cerr << "Could not bind value at index 1" << endl;
    return false;
  }
  rc = sqlite3_bind_text(stmt, 2, sstop.c_str(), -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    cerr << "Could not bind value at index 2" << endl;
    return false;
  }
  rc = sqlite3_bind_double(stmt, 3, prob);
  if (rc != SQLITE_OK) {
    cerr << "Could not bind value at index 3" << endl;
    return false;
  }
  rc = sqlite3_bind_text(stmt, 4, filename.c_str(), -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    cerr << "Could not bind value at index 4" << endl;
    return false;
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    cout << sqlite3_temp_directory << endl;
    printf("Could not step (execute) stmt, code = %d, tempdir=%s\n", rc, sqlite3_temp_directory);
    return false;
  }
  sqlite3_reset(stmt);
//  select_stmt("select * from videoinfo");
//  sqlite3_exec(db_, "DELETE FROM videoinfo", nullptr, nullptr, nullptr);
  return true;
}

int select_callback(void *p_data, int num_fields, char **p_fields, char **p_col_names) {

  int i;

  int *nof_records = (int *) p_data;
  (*nof_records)++;
  int first_row = 1;
  if (first_row) {
    first_row = 0;

    for (i = 0; i < num_fields; i++) {
      printf("%20s", p_col_names[i]);
    }

    printf("\n");
    for (i = 0; i < num_fields * 20; i++) {
      printf("=");
    }
    printf("\n");
  }

  for (i = 0; i < num_fields; i++) {
    if (p_fields[i]) {
      printf("%20s", p_fields[i]);
    } else {
      printf("%20s", " ");
    }
  }

  printf("\n");
  return 0;
}

void DBWriter::select_stmt(const char *stmt) {
  char *errmsg;
  int ret;
  int nrecs = 0;

  ret = sqlite3_exec(db_, stmt, select_callback, &nrecs, &errmsg);

  if (ret != SQLITE_OK) {
    printf("Error in select statement %s [%s].\n", stmt, errmsg);
  } else {
    printf("\n   %d records returned.\n", nrecs);
  }
}

}
