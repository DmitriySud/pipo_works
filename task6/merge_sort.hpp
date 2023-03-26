//#pragma once

#include <common/thread_pool.hpp>

#include <functional>


namespace sort {

namespace impl {

using ThreadPool =
    thread_pool::ThreadPool<std::function<void()>>;
using ThreadPoolTask = typename ThreadPool::TaskPtr;

// This class is needed only to save context (tp_, compare_)
template <class RandomIt, class RandomIt2, class Compare>
class MergeSortWrapper {
public:
  MergeSortWrapper(size_t threads, Compare compare)
      : tp_(threads, thread_pool::OverflowPolicy::kAllow), compare_(compare) {}

  typename ThreadPool::TaskPtr MergeSort(RandomIt, RandomIt, RandomIt2,
                                         RandomIt2);

private:
  ThreadPool tp_;
  Compare compare_;
};

template <class RandomIt, class RandomIt2, class Compare>
ThreadPoolTask MergeSortWrapper<RandomIt, RandomIt2, Compare>::MergeSort(
    RandomIt first, RandomIt last, RandomIt2 extra_first,
    RandomIt2 extra_last) {

  size_t delta = std::distance(first, last);
  if (delta < 2) {
    return nullptr;
  }

  RandomIt mid = first + delta / 2;
  RandomIt2 extra_mid = extra_first + delta / 2;

  auto task1_ptr = MergeSort(first, mid, extra_first, extra_mid);
  auto task2_ptr = MergeSort(mid, last, extra_mid, extra_last);

  if (task1_ptr) { task1_ptr->Wait(); }
  if (task2_ptr) { task2_ptr->Wait(); }

  // We need to place completable tasks first, because tasks in ThreadPool 
  // can not reschedule, because I'm too lazy to make context swithching
  return tp_.AddTask([=, this]() {
    // or it can be...
    //std::inplace_merge(first, mid, last, compare_);

    std::copy(first, last, extra_first);

    RandomIt cur = first;
    RandomIt2 left = extra_first;
    RandomIt2 right = extra_mid;

    while (left != extra_mid || right != extra_last) {
      if (left == extra_mid) {
        *cur = *right++;
      } else if (right == extra_last) {
        *cur = *left++;
      } else {
        *cur = compare_(*left, *right) ? *left++ : *right++;
      }

      cur++;
    }
  });
}

} // namespace impl

template <class RandomIt, class Compare>
void MergeSort(RandomIt first, RandomIt last, Compare comparer,
               size_t threads) {
  std::vector extra_data(first, last);
  // one thread less because main thread already used
  impl::MergeSortWrapper<RandomIt, decltype(extra_data.begin()), Compare> wrap(
      threads - 1, comparer);
  auto task = wrap.MergeSort(first, last, extra_data.begin(), extra_data.end());
}

} // namespace sort

