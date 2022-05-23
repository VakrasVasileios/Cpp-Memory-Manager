#pragma once

#include <list>
#include <functional>
#include <assert.h>

using size_t = unsigned long;

#ifndef CHUNK_SIZE
#define CHUNK_SIZE 10
#endif

namespace memman {

  template <class Tobj>
  class Pointer {
  public:
    Pointer(Tobj* _obj) { ptr_ = _obj; }
    Pointer(const Pointer& _obj) { ptr_ = _obj.ptr_; }
    Pointer(Pointer&& _obj) { ptr_ = _obj.ptr_; }

    auto Get() const -> Tobj* { return ptr_; }

    auto operator*(void) -> Tobj& { return *ptr_; }
    auto operator*(void) const -> Tobj& { return *ptr_; }
    auto operator->(void) -> Tobj& { return *ptr_; }
    auto operator->(void) const -> Tobj& { return *ptr_; }

    friend auto operator<<(std::ostream& os, const Pointer<Tobj>& p) -> std::ostream& {
      return os << p.Get();
    }

  private:
    using DeleterFunc = std::function<void()>();
    Tobj* ptr_;
    DeleterFunc del_;
  };

  namespace {

    template<class Tobj>
    class MemoryChunk final {
    public:
      MemoryChunk() {
        for (int i = 0; i < CHUNK_SIZE; i++) {
          chunk_[i].first = nullptr;
          free_space_.emplace_back(i);
        }
      }
      ~MemoryChunk() {
        for (int i = 0; i < CHUNK_SIZE; i++) {
          if (chunk_[i].first != nullptr)
            delete chunk_[i].first;
        }
      }

      template<typename... Args>
      auto Allocate(Args&&... args) -> Tobj* {
        if (!free_space_.empty()) {
          auto iter = free_space_.front();
          if (chunk_[iter.Get()].first != nullptr) {
            Tobj obj(args...);
            *(chunk_[iter.Get()].first) = obj;
          }
          else {
            chunk_[iter.Get()].first = new Tobj(std::forward<Args>(args)...);
          }
          chunk_[iter.Get()].second = 1;

          free_space_.pop_front();
          managed_space_.push_back(iter);

          return chunk_[iter.Get()].first;
        }
        else
          // return nullptr so that the manager creates a new memory chunk
          return nullptr;
      }

      bool IsFull(void) { return managed_space_.size() == CHUNK_SIZE ? true : false; }
      bool IsEmpty(void) { return managed_space_.size() == 0; }
      auto Size(void) -> size_t { return managed_space_.size(); }

    protected:
      class Iterator {
      public:
        Iterator(int _index) : index_(_index) {}
        ~Iterator() = default;

        auto Get() const -> const int { return index_; }

        auto GetCount(void) const -> size_t { return chunk_[index_].second; }
        void IncreaseCount(void) { ++(chunk_[index_].second); }
        void DecreaseCount(void) { --(chunk_[index_].second); }

      private:
        int index_;
      };

      friend Iterator;
    private:
      std::pair<Tobj*, size_t> chunk_[CHUNK_SIZE];
      std::list<Iterator> free_space_;
      std::list<Iterator> managed_space_;
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
        Tobj* new_obj;
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
      int delete_count_ = 0, new_count_ = 0;

      MemoryManager() = default;
      MemoryManager(const MemoryManager&) = delete;
      MemoryManager(MemoryManager&&) = delete;
      ~MemoryManager() {
        chunk_list_.clear();
      }
    };

  } // namespace

  template<typename Tobj, typename... Args>
  auto make_pointer(Args&&... args) -> Pointer<Tobj> { return MemoryManager<Tobj>::Get().New(std::forward<Args>(args)...); }

} // namespace memman
