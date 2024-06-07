#include <benchmark/benchmark.h>

#include "mem_new.h"
#include "mem_orig.h"


CephContext stam;


// initial sanity check for both implementations
void exp_orig()
{
  MemoryModel mm{&stam};
  MemoryModel::snap s;
  mm.sample(&s);

  [[maybe_unused]] volatile char* p = new char[24 * 1024 * 1024];
  MemoryModel::snap t;
  mm.sample(&t);
  delete[] p;

  fmt::print("{}:\t{}\n\t\t{}\n", __func__, s, t);
}


void exp_new()
{
  MM2 mm;
  auto s = mm.sample();
  [[maybe_unused]] volatile char* p = new char[36 * 1024 * 1024];
  auto t = mm.sample();
  // delete[] p;

  fmt::print("{}:\t{}\n\t\t{}\n", __func__, *s, *t);
}


#if 0
int main()
{
  exp_orig();
  exp_new();
}
#endif


// when the creation of the object is part of the benchmark
void BM_ORIG(benchmark::State& state)
{
  [[maybe_unused]] volatile long rss;
  for (auto _ : state) {
    MemoryModel mm{&stam};
    MemoryModel::snap s;
    mm.sample(&s);
    rss = s.rss;
  }
}
BENCHMARK(BM_ORIG);

void BM_NEW(benchmark::State& state)
{
  [[maybe_unused]] volatile long rss;
  for (auto _ : state) {
    MM2 mm;
    auto s = mm.sample();
    rss = s->rss;
  }
}
BENCHMARK(BM_NEW);

void BM_NEW_samp2(benchmark::State& state)
{
  [[maybe_unused]] volatile long rss;
  for (auto _ : state) {
    MM2 mm;
    auto s = mm.sample_take2();
    rss = s->rss;
  }
}
BENCHMARK(BM_NEW_samp2);


#if 1

BENCHMARK_MAIN();

#endif
