// =======================================================================
// Copyright (c) 2016-2020 by LI YANZHE. All rights reserved.
// Author: LI YANZHE <lee.yanzhe@yanzhe.org>
// Created by LI YANZHE on 3/18/19.
// =======================================================================

#ifndef LYZTOYS_UTIL_H
#define LYZTOYS_UTIL_H

#include <tuple>
#include <type_traits>
#include <chrono>

#define GETTIME(X, MSG)                                              \
{                                                                    \
    struct timespec start, finish;                                   \
    double elapsed;                                                  \
    clock_gettime(CLOCK_MONOTONIC, &start);                         \
    {X}                                                              \
    clock_gettime(CLOCK_MONOTONIC, &finish);                        \
    elapsed = (finish.tv_sec - start.tv_sec);                        \
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1e9;               \
    std::cout <<#MSG<<" time = " << elapsed << "s" << std::endl;     \
}

#define GETTIME_HIGH(X, MSG)                                                     \
{                                                                                 \
  auto t_start = std::chrono::high_resolution_clock::now();                       \
  { X }                                                                           \
  auto t_end = std::chrono::high_resolution_clock::now();                         \
  double dur = std::chrono::duration<double, std::nano>(t_end - t_start).count(); \
  long s = (long) (dur / 1e9);                                                    \
  long ms = (long) ((dur / 1e6) - s * 1e3);                                       \
  long us = (long) ((dur / 1e3) - s * 1e6 - ms * 1e3);                            \
  long ns = ((long) dur) % 1000;                                                  \
  std::cout << #MSG                                                               \
  << " time = "                                                                   \
  << s << " s "                                                                   \
  << ms << " ms "                                                                 \
  << us << " us "                                                                 \
  << ns << " ns"                                                                  \
  << std::endl;                                                                   \
}

namespace lyz {

template<std::size_t I = 0, typename FuncT, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
for_each_in_tuple(std::tuple<Tp...> &, FuncT) {}

template<std::size_t I = 0, typename FuncT, typename... Tp>
inline typename std::enable_if<I < sizeof...(Tp), void>::type
for_each_in_tuple(std::tuple<Tp...> &t, FuncT f) {
  f(std::get<I>(t));
  for_each_in_tuple<I + 1, FuncT, Tp...>(t, f);
}

}

#define HAS_MEMBER_FUNC(type, name)                             \
(std::is_member_function_pointer<decltype(&type::name)>::value)

inline double epoch_time() {
  return std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

#endif //LYZTOYS_UTIL_H