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
    Pointer(const Pointer& _ptr) { ptr_ = _ptr.ptr_; del_(); }
    Pointer(Pointer&& _ptr) { ptr_ = _ptr.ptr_; del_() }

    auto Get() const -> Tobj* { return ptr_; }

    auto operator*(void) -> Tobj& { return *ptr_; }
    auto operator*(void) const -> Tobj& { return *ptr_; }
    auto operator->(void) -> Tobj& { return *ptr_; }
    auto operator->(void) const -> Tobj& { return *ptr_; }

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
        for (int i = 0; i < CHUNK_SIZE; i++)
          free_space_.emplace_back(i);
      }
      ~MemoryChunk() {}

      bool IsFull(void) { return managed_space_.size() == CHUNK_SIZE ? true : false; }
      bool IsEmpty(void) { return managed_space_.size() == 0; }
      auto Size(void) { return managed_space_.size(); }

    protected:
      class Iterator {
      public:
        Iterator(int _index) : index_(_index) {}
        ~Iterator() = default;

        auto GetCount(void) const { return chunk_[index_].second; }
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
      static auto Get() -> MemoryManager& { return singleton_; }

      template<typename Args>
      auto New(Args... args) -> Pointer<Tmem> {  }

    private:
      MemoryManager singleton_;
      std::list<MemoryChunk<Tobj>> chunk_list_;
      int delete_count_ = 0, new_count_ = 0;

      MemoryManager() = default;
      MemoryManager(const MemoryManager&) = delete;
      MemoryManager(MemoryManager&&) = delete;
    };

  } // namespace

} // namespace memman