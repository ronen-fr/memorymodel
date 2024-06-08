
#include "mem_new.h"
#if defined(__linux__)
#include <malloc.h>
#endif

#include <charconv>
#include <fstream>

#define dout_subsys ceph_subsys_

using namespace std;


using std::cout;
using snap = MM2::snap;


std::optional<snap> MM2::sample_take2()
{
  if (!proc_status.is_open()) {
    cout << "check_memory_usage unable to open "
            "/proc/self/status"
         << endl;
    return std::nullopt;
  }
  proc_status.clear();
  proc_status.seekg(0);
  snap s;


  // we will be looking for 6 entries
  int yet_to_find = 6;
  while (!proc_status.eof() && yet_to_find) {
    string ln;
    getline(proc_status, ln);
    const auto r = ln.c_str();
    const auto e = r + ln.size();
    // cout << "line: " << ln << " sz:" << ln.size() << endl;
    // fmt::print("\t\tline:{} sz:{} at offset:{:s}\n", ln, ln.size(), r + 7);

    // clang-format off
    if (ln.starts_with("VmSize:")) { from_chars(r + 7, e, s.size); yet_to_find--; }
    else if (ln.starts_with("VmRSS:")) { from_chars(r + 6, e, s.rss); yet_to_find--; }
    else if (ln.starts_with("VmHWM:")) { from_chars(r + 6, e, s.hwm); yet_to_find--; }
    else if (ln.starts_with("VmLib:")) { from_chars(r + 6, e, s.lib); yet_to_find--; }
    else if (ln.starts_with("VmPeak:")) { auto [ptr,ec] = from_chars(r + 7, e, s.peak); yet_to_find--; if (ec != std::errc()) {cout << "error: " << *(r+7) << endl;} }
    else if (ln.starts_with("VmData:")) { from_chars(r + 7, e, s.data); yet_to_find--; }
    else if (ln.starts_with("VmData:")) { from_chars(r + 7, e, s.data); yet_to_find--; }
    //fmt::print("\tyet_to_find: {} collected:{}\n", yet_to_find, s);
    // clang-format on
  }

  return s;
}


inline bool MM2::cmp_against(const std::string& ln, std::string_view param, long& v) const
{
  if (ln.size() < (param.size() + 10)) {
    return false;
  }
  if (ln.starts_with(param)) {
    auto p = ln.c_str();
    auto s = ln.c_str() + param.size();
    // charconv does not like leading spaces
    while (*s && isblank(*s)) {
      s++;
    }
    /*auto [ptr, ec] = */ from_chars(s, p + ln.size(), v);

    // fmt::print("\t\ts<{:s}> v:{}  {}\n", s, v, (ec == std::errc()) ? "ok" : "error");
    return true;
  }
  return false;
}

std::optional<snap> MM2::sample()
{
  if (!proc_status.is_open()) {
    cout << "check_memory_usage unable to open "
            "/proc/self/status"
         << endl;
    return std::nullopt;
  }
  proc_status.clear();
  proc_status.seekg(0);
  snap s;

  // we will be looking for 6 entries
  int yet_to_find = 6;
  while (!proc_status.eof() && yet_to_find > 0) {
    string ln;
    getline(proc_status, ln);
    // const auto r = ln.c_str();
    //  const auto e = r + ln.size();
    //  cout << "line: " << ln << " sz:" << ln.size() << endl;
    //  fmt::print("\t\tline:{} sz:{} at offset:{:s}\n", ln, ln.size(), r + 7);

    if (cmp_against(ln, "VmSize:", s.size) || cmp_against(ln, "VmRSS:", s.rss) ||
        cmp_against(ln, "VmHWM:", s.hwm) || cmp_against(ln, "VmLib:", s.lib) ||
        cmp_against(ln, "VmPeak:", s.peak) || cmp_against(ln, "VmData:", s.data)) {
      yet_to_find--;
    }
  }
#ifdef NOT_YET
  f.open("/proc/self/maps");
  if (!f.is_open()) {
    cout << "check_memory_usage unable to open "
            "/proc/self/maps"
         << endl;
    return std::nullopt;
  }

  long heap = 0;
  while (f.is_open() && !f.eof()) {
    string line;
    getline(f, line);

    const char* start = line.c_str();
    const char* dash = start;
    while (*dash && *dash != '-')
      dash++;
    if (!*dash)
      continue;
    const char* end = dash + 1;
    while (*end && *end != ' ')
      end++;
    if (!*end)
      continue;
    unsigned long long as = strtoll(start, 0, 16);
    unsigned long long ae = strtoll(dash + 1, 0, 16);

    end++;
    const char* mode = end;

    int skip = 4;
    while (skip--) {
      end++;
      while (*end && *end != ' ')
        end++;
    }
    if (*end)
      end++;

    long size = ae - as;

    /*
     * anything 'rw' and anon is assumed to be heap.
     */
    if (mode[0] == 'r' && mode[1] == 'w' && !*end)
      heap += size;
  }

  s.heap = heap >> 10;
#endif
  return s;
}

// should return 'expected'
long MM2::compute_heap()
{
  if (!proc_maps.is_open()) {
    cout << "check_memory_usage unable to open "
            "/proc/self/maps"
         << endl;
    return 0;
  }

  proc_maps.clear();
  proc_maps.seekg(0);
  long heap = 0;

  while (proc_maps.is_open() && !proc_maps.eof()) {
    string line;
    getline(proc_maps, line);

    const char* start = line.c_str();
    const char* dash = start;
    while (*dash && *dash != '-')
      dash++;
    if (!*dash)
      continue;
    const char* end = dash + 1;
    while (*end && *end != ' ')
      end++;
    if (!*end)
      continue;
    unsigned long long as = strtoll(start, 0, 16);
    unsigned long long ae = strtoll(dash + 1, 0, 16);

    end++;
    const char* mode = end;

    int skip = 4;
    while (skip--) {
      end++;
      while (*end && *end != ' ')
        end++;
    }
    if (*end)
      end++;

    long size = ae - as;

    /*
     * anything 'rw' and anon is assumed to be heap.
     */
    if (mode[0] == 'r' && mode[1] == 'w' && !*end)
      heap += size;
  }

  return heap >> 10;
}

long MM2::compute_heap2()
{
  if (!proc_maps.is_open()) {
    cout << "check_memory_usage unable to open "
            "/proc/self/maps"
         << endl;
    return 0;
  }

  proc_maps.clear();
  proc_maps.seekg(0);
  long heap = 0;

  while (proc_maps.is_open() && !proc_maps.eof()) {
    string line;
    getline(proc_maps, line);

//     fmt::print("\tline:{}\n", line);

    if (line.length() < 48) {
      // a malformed line. We expect at least
      // '560c03f8d000-560c03fae000 rw-p 00000000 00:00 0'
      continue;
    }

    const char* start = line.c_str();
    const char* dash = start;
    while (*dash && *dash != '-')
      dash++;
    if (!*dash)
      continue;
    const char* end = dash + 1;
    while (*end && *end != ' ')
      end++;
    if (!*end)
      continue;

    auto addr_end = end;
    end++;
    const char* mode = end;


//     // for debugging:
//     {
//       uint64_t as{0ull};
//       from_chars(start, dash, as, 16);
//       uint64_t ae{0ull};
//       from_chars(dash + 1, addr_end, ae, 16);
//       fmt::print("\t\tas:{:x} ae:{:x} -> {}\n", as, ae, ((ae - as) >> 10));
//     }


    /*
     * anything 'rw' and anon is assumed to be heap.
     * But we should count lines with inode '0' and '[heap]' as well
     */
    if (mode[0] != 'r' || mode[1] != 'w') {
      continue;
    }

    auto the_rest = line.substr(5 + end - start);

//     fmt::print("\n\tline:|{}| {}/{}) |{}|\n", line, mode[0], mode[1], the_rest);
    if (!the_rest.starts_with("00000000 00:00 0")) {
      continue;
    }
    if (the_rest.ends_with("[stack]")) {
      // RRR should we exclude the stack?
      continue;
    }

    // calculate and sum the size of the heap segment
    uint64_t as{0ull};
    from_chars(start, dash, as, 16);
    uint64_t ae{0ull};
    from_chars(dash + 1, addr_end, ae, 16);
//     fmt::print("\t\tas:{:x} ae:{:x} -> {}\n", as, ae, ((ae - as) >> 10));
    long size = ae - as;
    heap += size;
  }

  return heap >> 10;
}

/*
improvement in the heap calculation function, just from having the file open:

-------------------------------------------------------
Benchmark             Time             CPU   Iterations
-------------------------------------------------------
BM_ORIG           10621 ns        10510 ns        68111
BM_NEW            10832 ns        10683 ns        65282
BM_NEW_samp2      10681 ns        10600 ns        66600
BM_ORIG2          11382 ns        11273 ns        64043
BM_NEW2            5079 ns         5041 ns       138949
BM_HEAP_ORIG      19173 ns        19043 ns        36073
BM_HEAP_NEW       15594 ns        15492 ns        44471

heap2, which uses substr:
-------------------------------------------------------
Benchmark             Time             CPU   Iterations
-------------------------------------------------------
BM_ORIG           11299 ns        11178 ns        63913
BM_NEW            11244 ns        11124 ns        63750
BM_NEW_samp2      10948 ns        10836 ns        64014
BM_ORIG2          11203 ns        11088 ns        64590
BM_NEW2            5318 ns         5260 ns       133662
BM_HEAP_ORIG      19732 ns        19430 ns        36269
BM_HEAP_NEW       16294 ns        16079 ns        43558
BM_HEAP_NEW2      13689 ns        13534 ns        53960



*/
