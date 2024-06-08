#pragma once

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <string_view>


class MM2 {
 public:
  struct snap {

    // should be unsigned, per proc(5)
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
  //int* cct;
  //void _sample(snap* p);

  std::ifstream proc_status{"/proc/self/status"};
  std::ifstream proc_maps{"/proc/self/maps"};

  inline bool cmp_against(const std::string& ln, std::string_view param, long& v) const;

 public:
  //explicit MM2();

  std::optional<snap> sample();
  std::optional<snap> sample_take2();

  long compute_heap();
  long compute_heap2();
};


namespace fmt {

template <> struct formatter<MM2::snap> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext> auto format(const MM2::snap& sn, FormatContext& ctx)
  {
    return fmt::format_to(ctx.out(),
                          "[peak:{},size:{},hwm:{},rss:{},data:{},lib:{},heap:{}",
                          sn.peak, sn.size, sn.hwm, sn.rss, sn.data, sn.lib, sn.heap);
  }
};

}  // namespace fmt
