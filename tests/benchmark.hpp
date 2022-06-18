#pragma once

#include <chrono>
#include <unordered_map>
#include <string>
#include <stack>

#ifdef BENCHMARK
#define StartTimer(tag)   BenchmarkTimer::Get().Start(tag)
#define EndTimer          BenchmarkTimer::Get().End()
#define CSV_PRINT         BenchmarkTimer::Get().PrintCsvStyle()
#else
#define StartTimer(tag)
#define EndTimer
#define CSV_PRINT
#endif

typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::nanoseconds nanoseconds;

class BenchmarkTimer final {
public:
  static auto Get() -> BenchmarkTimer& {
    static BenchmarkTimer singleton;
    return singleton;
  }

  inline
    void Start(const std::string& tag) {
    tag_.push(tag);
    start_.push(Clock::now());
  }
  void End(void) {
    end_ = Clock::now();
    nanoseconds ms = std::chrono::duration_cast<nanoseconds>(end_ - start_.top());
    start_.pop();
    tags_n_times_[tag_.top()] += ms.count();
    tag_.pop();
  }

  void PrintCsvStyle(void) {
    for (auto& tag : tags_n_times_)
      std::cout << tag.first << ',';
    std::cout << '\n';

    for (auto& t : tags_n_times_)
      std::cout << t.second << ',';
    std::cout << '\n';
  }

private:
  std::unordered_map<std::string, double> tags_n_times_;
  std::stack<Clock::time_point> start_;
  Clock::time_point end_;
  std::stack<std::string> tag_;
};