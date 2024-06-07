
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

bool MM2::cmp_against(const std::string& ln,
                      std::string_view param,
                      long& v,
                      int& cnt) const
{
  if (ln.starts_with(param)) {
    cnt++;
    from_chars(ln.c_str() + param.size(), ln.c_str() + ln.size(), v);
    return true;
  }
  return false;
}

std::optional<snap> MM2::sample_take2()
{
  snap s;

  ifstream f;

  f.open("/proc/self/status");
  if (!f.is_open()) {
    cout << "check_memory_usage unable to open "
            "/proc/self/status"
         << endl;
    return std::nullopt;
  }

  // we will be looking for 6 entries
  int yet_to_find = 6;
  while (!f.eof() && yet_to_find) {
    string ln;
    getline(f, ln);
    const auto r = ln.c_str();
    const auto e = ln.c_str() + ln.size();

    // clang-format off
    if (ln.starts_with("VmSize:")) { from_chars(r + 7, e, s.size); yet_to_find--; }
    else if (ln.starts_with("VmRSS:")) { from_chars(r + 6, e, s.rss); yet_to_find--; }
    else if (ln.starts_with("VmHWM:")) { from_chars(r + 6, e, s.hwm); yet_to_find--; }
    else if (ln.starts_with("VmLib:")) { from_chars(r + 6, e, s.lib); yet_to_find--; }
    else if (ln.starts_with("VmPeak:")) { from_chars(r + 7, e, s.peak); yet_to_find--; }
    else if (ln.starts_with("VmData:")) { from_chars(r + 7, e, s.data); yet_to_find--; }
    // clang-format on
  }
  f.close();

  return s;
}



std::optional<snap> MM2::sample()
{
  snap s;

  ifstream f;

  f.open("/proc/self/status");
  if (!f.is_open()) {
    cout << "check_memory_usage unable to open "
            "/proc/self/status"
         << endl;
    return std::nullopt;
  }

  // we will be looking for 6 entries
  int yet_to_find = 6;
  while (!f.eof() && yet_to_find) {
    string line;
    getline(f, line);

    cmp_against(line, "VmSize:", s.size, yet_to_find) ||
      cmp_against(line, "VmRSS:", s.rss, yet_to_find) ||
      cmp_against(line, "VmHWM:", s.hwm, yet_to_find) ||
      cmp_against(line, "VmLib:", s.lib, yet_to_find) ||
      cmp_against(line, "VmPeak:", s.peak, yet_to_find) ||
      cmp_against(line, "VmData:", s.data, yet_to_find);
  }
  f.close();
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
