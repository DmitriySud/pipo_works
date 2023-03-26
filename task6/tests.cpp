#include "merge_sort.hpp"

#include <cassert>


int main() {
  // Case 1
  std::vector<int> data{3, 2, 1};

  sort::MergeSort(data.begin(), data.end(), std::less<int>{}, 3);

  assert((data == std::vector<int>{1, 2, 3}));

  std::cout << "Case 1 completed" << std::endl;


  // Case 2
  int array_data[]{39, 24, 90, 38, 34, 53, 84, 19, 6, 70};
  size_t array_len = sizeof(array_data) / sizeof(int);
  sort::MergeSort(std::begin(array_data), std::end(array_data),
                  std::greater<int>{}, 4);

  // Don't really know, how to compare arrays properly
  int sorted_array[]{ 90, 84, 70, 53, 39, 38, 34, 24, 19, 6};
  for (size_t i = 0; i < array_len; ++i){
    assert((array_data[i] == sorted_array[i]));
  }

  std::cout << "Case 2 completed" << std::endl;

  // Case 3
  std::vector<int> big_array(100);
  for (auto& it : big_array){
    it = rand() % 1000;
  }

  sort::MergeSort(big_array.begin(), big_array.end(), std::less<int>{}, 3);
  for (size_t i = 0; i < big_array.size() - 1; ++i){
    assert((big_array[i] <= big_array[i+1]));
  }
  
  std::cout << "Case 3 completed" << std::endl;

  return 0;
}
