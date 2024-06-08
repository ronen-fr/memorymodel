#pragma once

#include <iostream>
#include <map>
#include <cstring>
#include <fmt/core.h>
#include <fmt/ranges.h>

struct CephContext
{
        /* data */
};


class MemoryModel {
public:
  struct snap {
    long peak{0};
    long size{0};
    long hwm{0};
    long rss{0};
    long data{0};
    long lib{0};
    long heap{0};

    long get_total() { return size; }
    long get_rss() { return rss; }
    long get_heap() { return heap; }
  } last;

private:
  CephContext *cct;
  void _sample(snap *p);

public:
  explicit MemoryModel(CephContext *cct);
  void sample(snap *p = 0) {
    _sample(&last);
    if (p)
      *p = last;
  }

  long compute_heap();
};


namespace fmt {

template <>
struct formatter<MemoryModel::snap> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const MemoryModel::snap& sn, FormatContext& ctx)
  {
    return fmt::format_to(
	ctx.out(), "[peak:{},size:{},hwm:{},rss:{},data:{},lib:{},heap:{}",
        sn.peak, sn.size, sn.hwm, sn.rss, sn.data, sn.lib, sn.heap);
  }
};

}  // namespace fmt

