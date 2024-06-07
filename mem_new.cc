
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
    //cout << "line: " << ln << " sz:" << ln.size() << endl;
    //fmt::print("\t\tline:{} sz:{} at offset:{:s}\n", ln, ln.size(), r + 7);

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


bool MM2::cmp_against(const std::string& ln,
                      std::string_view param,
                      long& v,
                      int& cnt) const
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
    /*auto [ptr, ec] = */from_chars(s, p + ln.size(), v);

   //fmt::print("\t\ts<{:s}> v:{}  {}\n", s, v, (ec == std::errc()) ? "ok" : "error");

    cnt++;
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
  while (!proc_status.eof() && yet_to_find) {
    string ln;
    getline(proc_status, ln);
    const auto r = ln.c_str();
    //const auto e = r + ln.size();
    //cout << "line: " << ln << " sz:" << ln.size() << endl;
    //fmt::print("\t\tline:{} sz:{} at offset:{:s}\n", ln, ln.size(), r + 7);

    cmp_against(ln, "VmSize:", s.size, yet_to_find) ||
      cmp_against(ln, "VmRSS:", s.rss, yet_to_find) ||
      cmp_against(ln, "VmHWM:", s.hwm, yet_to_find) ||
      cmp_against(ln, "VmLib:", s.lib, yet_to_find) ||
      cmp_against(ln, "VmPeak:", s.peak, yet_to_find) ||
      cmp_against(ln, "VmData:", s.data, yet_to_find);
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
