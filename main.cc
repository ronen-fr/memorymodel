#include <benchmark/benchmark.h>

#include "mem_new.h"
#include "mem_orig.h"

#define RUN_THE_BM


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
  auto r = mm.sample();
  // delete[] p;

  fmt::print("{}:\t{}\n\t\t{}\n\t\t{}\n", __func__, *s, *t, *r);
}


#ifndef RUN_THE_BM
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
    auto s = mm.sample();
    rss = s->rss;
  }
}
BENCHMARK(BM_NEW_samp2);


// with object creation outside of the loop (and - for mem2, it means the file is opened only once)

void BM_ORIG2(benchmark::State& state)
{
  MemoryModel mm{&stam};
  MemoryModel::snap s;
  [[maybe_unused]] volatile long rss;
  for (auto _ : state) {
    mm.sample(&s);
    rss = s.rss;
  }
}
BENCHMARK(BM_ORIG2);

void BM_NEW2(benchmark::State& state)
{
  MM2 mm;
  [[maybe_unused]] volatile long rss;
  for (auto _ : state) {
    auto s = mm.sample();
    assert(s->rss > 0);
    rss = s->rss;
  }
}
BENCHMARK(BM_NEW2);



#ifdef RUN_THE_BM

BENCHMARK_MAIN();

#endif
