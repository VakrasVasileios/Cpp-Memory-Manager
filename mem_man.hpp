#pragma once

#include <list>
#include <functional>
#include <assert.h>
#include <exception>
#include <algorithm>

using size_t = unsigned long;

#ifndef CHUNK_SIZE
#define CHUNK_SIZE 10
#endif

#ifdef HEAP_SIZE
#define MB 1000000
#define MEM_SIZE HEAP_SIZE * MB
#endif

#if defined(MEM_THRESH) > 90
#define MEM_THRESH 90
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

    class MemChunkException : public std::exception {
    public:
      MemChunkException() = default;
      ~MemChunkException() override = default;

      virtual const char* what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW override { return "Memory Chunk full Exception"; }
    };

    template<class Tobj>
    class MemoryChunk final {
    public:
      using CountControler = std::function<void(int)>;
      class Iterator;

    public:
      MemoryChunk() {
        // std::cout << "chunk_size: " << CHUNK_SIZE << std::endl;
        for (int i = 0; i < CHUNK_SIZE; i++) {
          free_space_.emplace_back(i);
          chunk_[i] = nullptr;
          counters_[i] = 0;
          cnt_ctrl_[i] = [this, i](int op) {
            static int index = i;
            counters_[index] += op;
          };
        }
        assert(std::all_of(std::begin(cnt_ctrl_), std::end(cnt_ctrl_), [](const auto& f) {return f != nullptr;}));
      }
      ~MemoryChunk() {
        for (int i = 0; i < CHUNK_SIZE; i++) {
          if (chunk_[i] != nullptr)
            delete chunk_[i];
        }
      }

      template<typename... Args>
      auto Allocate(Args&&... args) -> Iterator {
        std::cout << "free_space size: " << free_space_.size() << std::endl;
        if (!free_space_.empty()) {
          int index = free_space_.front();
          std::cout << "free index is: " << index << std::endl;
          if (chunk_[index] != nullptr) {
            Tobj obj(args...);
            *(chunk_[index]) = obj;
          }
          else {
            chunk_[index] = new Tobj(std::forward<Args>(args)...);
          }
          counters_[index] = 1;

          free_space_.pop_front();
          std::cout << "free_space size after pop: " << free_space_.size() << std::endl;

          managed_space_.push_back(index);

          return Iterator(chunk_[index], counters_[index], cnt_ctrl_[index]);
        }
        else // if the chunk is full find and move 0 ref count pointer to free space
          SweepManagedMem();
        throw MemChunkException();
      }

      void SweepManagedMem(void) {
        for (int i = 0; i < CHUNK_SIZE; i++) {
          if (counters_[i] == 0) {
            auto iter = std::find(managed_space_.begin(), managed_space_.end(), i);
            free_space_.push_back(*iter);
            managed_space_.erase(iter);
          }
        }
      }

      bool IsFull(void) { return managed_space_.size() == CHUNK_SIZE ? true : false; }
      bool IsEmpty(void) { return managed_space_.size() == 0; }
      auto Size(void) -> size_t { return managed_space_.size(); }

      class Iterator {
      public:
        Iterator(Tobj* _obj, size_t _counter, const CountControler& _ctrl) : obj_(_obj), counter_(_counter), ctrl_(_ctrl) {}
        ~Iterator() = default;

        auto GetPointer(void) -> Tobj* { return obj_; }
        auto GetCount(void) const -> size_t { return counter_; }
        auto GetCntCtrl(void) const -> const CountControler& { return ctrl_; }

      private:
        Tobj* obj_;
        size_t counter_;
        const CountControler& ctrl_;
      };

    private:
      Tobj* chunk_[CHUNK_SIZE];
      size_t counters_[CHUNK_SIZE];
      CountControler cnt_ctrl_[CHUNK_SIZE];

      std::list<int> free_space_;
      std::list<int> managed_space_;
    };

    class MemoryObserver final {
    public:
      using ObserverFunc = std::function<size_t(void)>;
      using ManagerSweeper = std::function<void(void)>;
    public:
      static auto Get(void) -> MemoryObserver& {
        static MemoryObserver singleton;
        return singleton;
      }

      void RegisterObserver(const ObserverFunc& f) { observers_.push_back(f); }
      void RegisterSweeper(const ManagerSweeper& f) { sweepers_.push_back(f); }
#ifdef HEAP_SIZE
      bool CanRequestMemory(size_t size) {
        size_t mem = 0;
        for (auto& obs : observers_)
          mem += obs();
        SweepIfThreshold(mem >= static_cast<double>(MEM_THRESH / 100) * MEM_SIZE);
        return mem + size > MEM_SIZE ? false : true;
      }
#endif
      void SweepMemory(void) { SweepIfThreshold(true); }

    private:
      std::list<ObserverFunc> observers_;
      std::list<ManagerSweeper> sweepers_;

#ifdef HEAP_SIZE
      size_t mem_size_ = MEM_SIZE; // Hard max
#endif

      void SweepIfThreshold(bool reached) {
        if (reached) {
          for (auto& sweeper : sweepers_)
            sweeper();
        }
      }

      MemoryObserver() {
        // std::cout << "mem_size: " << MEM_SIZE
        //   << "\nmem_thresh: " << MEM_THRESH
        //   << std::endl;
      }
      MemoryObserver(const MemoryObserver&) = delete;
      MemoryObserver(MemoryObserver&&) = delete;
      ~MemoryObserver() {
        observers_.clear();
      };
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
        try {
          std::cout << "Finding mem chunk\n";
          auto& chunk = chunk_list_.back();
          auto iter = chunk.Allocate(std::forward<Args>(args)...);
          std::cout << "Allocated ptr\n";
          new_obj = iter.GetPointer();
          std::cout << "Assigned ptr\n";
          if (new_obj != nullptr) {
            Pointer<Tobj> ret(iter.GetPointer());
            ret.cnt_ctrlr_ = iter.GetCntCtrl();
            std::cout << "Returning ptr\n";
            return ret;
          }
        }
        catch (MemChunkException& e) {
          std::cout << "\tDid not find mem chunk\n";
          std::cout << "\tCreating new mem chunk\n";
          std::cout << '\t' << e.what() << std::endl;
          if (new_obj == nullptr) {
#ifdef HEAP_SIZE
            if (MemoryObserver::Get().CanRequestMemory(sizeof(Tobj) * CHUNK_SIZE))
#endif
              chunk_list_.emplace_back();
            auto iter = chunk_list_.back().Allocate(std::forward<Args>(args)...);
            Pointer<Tobj> ret(iter.GetPointer());
            ret.cnt_ctrlr_ = iter.GetCntCtrl();
            return ret;
          }
          else {
            std::cerr << "Shouldn't be here\n";
            assert(false);
          }
        }
      }

    private:
      std::list<MemoryChunk<Tobj>> chunk_list_;

      auto FindNonFullChunk(void) -> MemoryChunk<Tobj>& {
        for (auto& chunk : chunk_list_) {
          if (!chunk.IsFull())
            return chunk;
        }

      }

      MemoryManager() {
        chunk_list_.emplace_back();
        MemoryObserver::Get().RegisterObserver(
          [this]() {
            return CHUNK_SIZE * sizeof(Tobj) * chunk_list_.size();
          }
        );
        MemoryObserver::Get().RegisterSweeper(
          [this]() {
            for (auto& chunk : chunk_list_)
              chunk.SweepManagedMem();
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
    Pointer(Tobj* obj = nullptr) : ptr_(obj) {}
    Pointer(const Pointer& _obj) {
      ptr_ = _obj.ptr_;
      _obj.cnt_ctrlr_(1);
      cnt_ctrlr_ = _obj.cnt_ctrlr_;
    }
    Pointer(Pointer&& _obj) {
      ptr_ = _obj.ptr_;
      cnt_ctrlr_ = _obj.cnt_ctrlr_;
    }
    ~Pointer() {
      cnt_ctrlr_(-1);
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
    using CountCtrl = std::function<void(int)>;
    Tobj* ptr_;
    CountCtrl cnt_ctrlr_;
  };

  /**
   * @brief Allocates memory for the object and returns a pointer to it through a wrapper.
   *
   * @tparam Tobj Type of object to be allocated.
   * @tparam Args Constructor arguments for the object.
   * @param args Constructor arguments.
   * @return Pointer<Tobj> The Wrapper containing the allocated pointer.
   */
  template<typename Tobj, typename... Args>
  auto make_pointer(Args&&... args) -> Pointer<Tobj> { return MemoryManager<Tobj>::Get().New(std::forward<Args>(args)...); }

  /**
   * @brief Orders a sweep of the memory.
   *
   */
  void sweep_memory(void) { MemoryObserver::Get().SweepMemory(); }



} // namespace memman
