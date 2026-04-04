#include <iostream>
#include <vector>
#include <ranges>
#include <algorithm>

int main()
{
    std::vector<int> v{5, 3, 1, 4, 2};
    std::ranges::sort(v);

    std::cout << "toolchain OK — sorted: ";
    for (auto x : v) std::cout << x << ' ';
    std::cout << '\n';
}
