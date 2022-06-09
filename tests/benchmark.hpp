#pragma once

#include <chrono>
#include <unordered_map>
#include <string>
#include <stack>

#ifdef BENCHMARK
#define StartTimer(tag)   BenchmarkTimer::Get().Start(tag)
#define EndTimer          BenchmarkTimer::Get().End()
#else
#define StartTimer(tag)
#define EndTimer   
#endif

typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds milliseconds;

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
    milliseconds ms = std::chrono::duration_cast<milliseconds>(end_ - start_.top());
    start_.pop();
    tags_n_times_[tag_.top()] += ms.count();
    tag_.pop();
  }

private:
  std::unordered_map<std::string, double> tags_n_times_;
  std::stack<Clock::time_point> start_;
  Clock::time_point end_;
  std::stack<std::string> tag_;
};