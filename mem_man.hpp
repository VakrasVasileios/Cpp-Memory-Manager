#pragma once

#include <list>

using size_t = unsigned long;

#ifndef CHUNK_SIZE
#define CHUNK_SIZE 10
#endif

template <class Tobj>
class Pointer {
public:
  Pointer() { ptr_ = new Tobj; };
  Pointer(Pointer& _ptr) = default;
  Pointer(Pointer&& _ptr) = default;

  auto Get() const -> Tobj { return ptr_; }

  friend class MemoryManager<Tobj>;

private:
  Tobj* ptr_;
  size_t ref_count_ = 0;
};

template<class Tobj>
class MemoryChunk final {
public:
  MemoryChunk() {
    for (int i = 0; i < CHUNK_SIZE; i++)
      free_space_.emplace_back(i);
  }
  ~MemoryChunk() {}

  bool IsFull(void) { return managed_space_.size() == CHUNK_SIZE ? true : false; }

protected:
  class Iterator {
  public:
    Iterator(int _index) : index_(_index) {}
    ~Iterator() = default;

  private:
    int index_;
  };

  friend Iterator;
private:
  Pointer<Tobj> chunk_[CHUNK_SIZE];
  std::list<Iterator> free_space_;
  std::list<Iterator> managed_space_;
};

template <class Tobj>
class MemoryManager final {
public:
  static auto Get() -> MemoryManager& { return singleton_; }

  template<typename Args>
  auto New(Args...) -> Pointer<Tmem> { }

private:
  MemoryManager singleton_;
  std::list<MemoryChunk<Tobj>> chunk_list_;

  MemoryManager() = default;
  MemoryManager(const MemoryManager&) = delete;
  MemoryManager(MemoryManager&&) = delete;
};