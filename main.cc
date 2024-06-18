#include <benchmark/benchmark.h>

#include "mem_new.h"
#include "mem_orig.h"

//#define RUN_THE_BM


CephContext stam;


// initial sanity check for both implementations
void exp_orig()
{
  MemoryModel mm{&stam};
  MemoryModel::snap s;
  mm.sample(&s);

  auto heap1 = mm.compute_heap();

  [[maybe_unused]] volatile char* p = new char[24 * 1024 * 1024];
  MemoryModel::snap t;
  mm.sample(&t);
  delete[] p;
  auto heap2 = mm.compute_heap();

  fmt::print("{}:\t{}\n\t\t{}\n\theap: {} {}\n", __func__, s, t, heap1, heap2);
}


void exp_new()
{
  MM2 mm;
  auto s = mm.sample();
  auto heap1 = mm.compute_heap();
  [[maybe_unused]] volatile char* p = new char[36 * 1024 * 1024];
  auto t = mm.sample();
  auto r = mm.sample();
  auto heap2 = mm.compute_heap2();
  delete[] p;
  auto heap3 = mm.compute_heap3();

  fmt::print("{}:\t{}\n\t\t{}\n\t\t{}\n\theap: {} {} {}\n", __func__, *s, *t, *r, heap1,
             heap2, heap3);
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


// with object creation outside of the loop (and - for mem2, it means the file is opened
// only once)

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

// just "compute-heap" for both

void BM_HEAP_ORIG(benchmark::State& state)
{
  MemoryModel mm{&stam};
  MemoryModel::snap s;
  [[maybe_unused]] volatile long hp;
  for (auto _ : state) {
    hp = mm.compute_heap();
  }
}
BENCHMARK(BM_HEAP_ORIG);

void BM_HEAP_NEW(benchmark::State& state)
{
  MM2 mm;
  [[maybe_unused]] volatile long hp;
  for (auto _ : state) {
    hp = mm.compute_heap();
  }
}
BENCHMARK(BM_HEAP_NEW);

void BM_HEAP_NEW2(benchmark::State& state)
{
  MM2 mm;
  [[maybe_unused]] volatile long hp;
  for (auto _ : state) {
    hp = mm.compute_heap2();
  }
}
BENCHMARK(BM_HEAP_NEW2);

void BM_HEAP_NEW3(benchmark::State& state)
{
  MM2 mm;
  [[maybe_unused]] volatile long hp;
  for (auto _ : state) {
    hp = mm.compute_heap3();
  }
}
BENCHMARK(BM_HEAP_NEW3);

#ifdef RUN_THE_BM

BENCHMARK_MAIN();

#endif
