#pragma once

#include <list>
#include <functional>
#include <assert.h>
#include <tuple>

using size_t = unsigned long;

#ifndef CHUNK_SIZE
#define CHUNK_SIZE 10
#endif

#ifndef HEAP_SIZE
#define HEAP_SIZE 1024
#endif

#define MB 1000000
#define MEM_SIZE HEAP_SIZE * MB

#if defined(MEM_THRESH) > 100
#define MEM_THRESH 100
#endif

#if defined(MEM_THRESH) < 0
#define MEM_THRESH 25
#endif

#ifndef MEM_THRESH
#define MEM_THRESH 60
#endif

namespace memman {

  template <typename Tobj>
  class Pointer;

  namespace {

    template<class Tobj>
    class MemoryChunk final {
    public:
      using CountControler = std::function<void(int)>;
    public:
      MemoryChunk() {
        for (int i = 0; i < CHUNK_SIZE; i++) {
          chunk_[i] = nullptr;
          counters_[i] = 0;
          cnt_ctrl_[i] = [this, i](int op) {
            static int index = i;
            counters_[index] += op;
          };
          free_space_.emplace_back(i);
        }
      }
      ~MemoryChunk() {
        for (int i = 0; i < CHUNK_SIZE; i++) {
          if (chunk_[i] != nullptr)
            delete chunk_[i];
        }
      }

      template<typename... Args>
      auto Allocate(Args&&... args) -> Tobj* {
        if (!free_space_.empty()) {
          auto iter = free_space_.front();
          if (chunk_[iter.Get()] != nullptr) {
            Tobj obj(args...);
            *(chunk_[iter.Get()]) = obj;
          }
          else {
            chunk_[iter.Get()] = new Tobj(std::forward<Args>(args)...);
          }
          counters_[iter.Get()] = 1;

          free_space_.pop_front();
          managed_space_.push_back(iter);

          return chunk_[iter.Get()];
        }
        else
          // return nullptr so that the manager creates a new memory chunk
          return nullptr;
      }

      bool IsFull(void) { return managed_space_.size() == CHUNK_SIZE ? true : false; }
      bool IsEmpty(void) { return managed_space_.size() == 0; }
      auto Size(void) -> size_t { return managed_space_.size(); }

      class Iterator {
      public:
        Iterator(int _index) : index_(_index) {}
        ~Iterator() = default;

        auto Get() const -> const int { return index_; }

        auto GetCount(void) const -> size_t { return counters_[index_]; }
        void IncreaseCount(void) { ++(counters_[index_]); }
        void DecreaseCount(void) { --(counters_[index_]); }

      private:
        int index_;
      };

      friend Iterator;
    private:
      Tobj* chunk_[CHUNK_SIZE];
      size_t counters_[CHUNK_SIZE];
      CountControler cnt_ctrl_[CHUNK_SIZE];

      std::list<Iterator> free_space_;
      std::list<Iterator> managed_space_;
    };

    class MemoryObserver final {
    public:
      using ObserverFunc = std::function<size_t(void)>;
    public:
      static auto Get(void) -> MemoryObserver& {
        static MemoryObserver singleton;
        return singleton;
      }

      void RegisterObserver(const ObserverFunc& f) { observers_.push_back(f); }
      void SweepIfThreshold(void) {}

    private:
      std::list<ObserverFunc> observers_;
      size_t mem_size_ = MEM_SIZE; // soft max
      size_t mem_used_cache_ = 0;

      MemoryObserver() = default;
      MemoryObserver(const MemoryObserver&) = delete;
      MemoryObserver(MemoryObserver&&) = delete;
      ~MemoryObserver() = default;
    };

    template <class Tobj>
    class MemoryManager final {
    public:
      static auto Get() -> MemoryManager& {
        static MemoryManager singleton_;
        return singleton_;
      }

      template<typename... Args>
      auto New(Args&&... args) -> Pointer<Tobj> {
        Tobj* new_obj = nullptr;
        for (auto chunk : chunk_list_) {
          new_obj = chunk.Allocate(std::forward<Args>(args)...);
          if (new_obj != nullptr)
            return Pointer<Tobj>(new_obj);
        }
        if (new_obj == nullptr) {
          chunk_list_.emplace_back();
          new_obj = chunk_list_.back().Allocate(std::forward<Args>(args)...);
          return Pointer<Tobj>(new_obj);
        }
        else {
          assert(false);
        }
      }

    private:
      std::list<MemoryChunk<Tobj>> chunk_list_;

      MemoryManager() {
        MemoryObserver::Get().RegisterObserver(
          [this]() {
            return CHUNK_SIZE * sizeof(Tobj) * chunk_list_.size();
          }
        );
      }
      MemoryManager(const MemoryManager&) = delete;
      MemoryManager(MemoryManager&&) = delete;
      ~MemoryManager() {
        chunk_list_.clear();
      }
    };

  } // namespace

  template <typename Tobj>
  class Pointer {
  public:
    Pointer(Tobj* obj) : ptr_(obj) {}
    Pointer(const Pointer& _obj) {
      ptr_ = _obj.ptr_;
      del_(-1);
      _obj.del_(1);
      del_ = _obj.del_;
    }
    Pointer(Pointer&& _obj) {
      ptr_ = _obj.ptr_;
      del_(-1);
      del_ = _obj.del_;
    }

    auto Get() const -> Tobj& { return *ptr_; }
    auto Get() -> Tobj& { return *ptr_; }

    auto operator*(void) -> Tobj& { return *ptr_; }
    auto operator*(void) const -> Tobj& { return *ptr_; }
    auto operator->(void) -> Tobj& { return *ptr_; }
    auto operator->(void) const -> Tobj& { return *ptr_; }

    friend auto operator<<(std::ostream& os, const Pointer<Tobj>& p) -> std::ostream& {
      return os << p.Get();
    }

    friend class MemoryManager<Tobj>;

  private:
    using DeleterFunc = std::function<void(int)>;
    Tobj* ptr_;
    DeleterFunc del_;


  };

  template<typename Tobj, typename... Args>
  auto make_pointer(Args&&... args) -> Pointer<Tobj> { return MemoryManager<Tobj>::Get().New(std::forward<Args>(args)...); }

} // namespace memman
