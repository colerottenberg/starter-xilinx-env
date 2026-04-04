#include <print>  // C++23
#include <vector>
#include <ranges>
#include <algorithm>

int main()
{
    std::vector<int> v{5, 3, 1, 4, 2};
    std::ranges::sort(v);

    std::print("toolchain OK — sorted: ");
    for (auto x : v) std::print("{} ", x);
    std::println("");
}
