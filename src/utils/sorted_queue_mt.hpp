#pragma once
#include <tuple>
#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T, typename Compare>
class sorted_queue_mt
{
  std::priority_queue<T, std::vector<T>, Compare> m_items;
  mutable std::mutex m_mutex;
  std::condition_variable m_cv_full;
  std::condition_variable m_cv_empty;
  bool m_stop;
  const int m_maxsize;

public:
  explicit sorted_queue_mt(int _maxsize = 0): m_stop(false), m_maxsize(_maxsize) {}
  sorted_queue_mt(const sorted_queue_mt& o) = delete;

  void push(const T& t)
  {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_cv_full.wait(lk, [this] {
      return m_stop || m_maxsize <= 0 || m_items.size() < m_maxsize;
    });
    if (m_stop) return;
    m_items.push(t);
    m_cv_empty.notify_all();
  }

  template<typename Pred>
  T pop(const Pred& pred) {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_cv_empty.wait(lk, [this,&pred] {
      return m_stop || (!m_items.empty() && pred(m_items.top()));
    });
    if (m_stop) return T();
    T item = std::move(m_items.top());
    m_items.pop();
    m_cv_full.notify_all();
    return item;
  }

  size_t size() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_items.size();
  }

  bool full() const {
    if (m_maxsize <= 0) return false;
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_items.size() >= m_maxsize;
  }

  void stop() {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_stop = true;
    m_cv_full.notify_all();
    m_cv_empty.notify_all();
  }
};


