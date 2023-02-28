#include "src/3rd-party/catch.hpp"
#include "src/xbt/utils/iter/LazyKSubsets.hpp"
#include "src/xbt/utils/iter/LazyPowerset.hpp"

#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace simgrid::xbt;

// This is used in a number of tests below
// to verify that counts of elements for example.
static unsigned integer_power(unsigned x, unsigned p)
{
  unsigned i = 1;
  for (unsigned j = 1; j <= p; j++)
    i *= x;
  return i;
}

TEST_CASE("simgrid::xbt::subsets_iterator: Iteration General Properties")
{
  std::vector<int> example_vec{0, 1, 2, 3, 4, 5, 6, 7};

  SECTION("Each element of each subset is distinct and appears half of the time")
  {
    for (unsigned k = 0; static_cast<size_t>(k) < example_vec.size(); k++) {
      for (auto& subset : make_k_subsets_iter(k, example_vec)) {
        // Each subset must have size `k`
        REQUIRE(subset.size() == k);

        // Each subset must be comprised only of distinct elements
        std::unordered_set<int> elements_seen(k);
        for (const auto& element_ptr : subset) {
          const auto& element = *element_ptr;
          REQUIRE(elements_seen.find(element) == elements_seen.end());
          elements_seen.insert(element);
        }
      }
    }
  }
}

TEST_CASE("simgrid::xbt::powerset_iterator: Iteration General Properties")
{
  std::vector<int> example_vec{0, 1, 2, 3, 4, 5, 6, 7};

  SECTION("Each element of each subset is distinct and appears half of the time in all subsets iteration")
  {
    // Each element is expected to be found in half of the sets
    const unsigned k         = static_cast<unsigned>(example_vec.size());
    const int expected_count = integer_power(2, k - 1);

    std::unordered_map<int, int> element_counts(k);

    for (auto& subset : make_powerset_iter(example_vec)) {
      // Each subset must be comprised only of distinct elements
      std::unordered_set<int> elements_seen(k);
      for (const auto& element_ptr : subset) {
        const auto& element = *element_ptr;
        REQUIRE(elements_seen.find(element) == elements_seen.end());
        elements_seen.insert(element);
        element_counts[element]++;
      }
    }

    for (const auto& iter : element_counts) {
      REQUIRE(iter.second == expected_count);
    }
  }
}