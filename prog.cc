#include <iostream>
#include <utility>
#include <cstdint>
#include <vector>

using namespace std;

class walker
{
public:
    // current node, state map
    typedef std::pair<int, uint64_t> state;

    void next(state &cur) const
    {
        uint64_t m = uint64_t(1) << cur.first;
        uint64_t v = cur.second & m;
        cur.first = (v == 0 ? lefts : rights)[cur.first];
        cur.second ^= m;
    }

    walker(std::istream& is)
    {
        is >> target;

        for (int i = 1; i < target; ++i)
        {
            int left, right;
            is >> left >> right;
            lefts.push_back(left-1);
            rights.push_back(right-1);
        }

        target -= 1;

        //std::cout << "READ: " << target << "  " << lefts.size() << "  " << rights.size() << "\n";
    }

    void log(uint64_t path, const state& s) const 
    {
        uint64_t m = uint64_t(1) << s.first;
        std::cout
            << path << " "
            << s.first << " "
            << ((s.second & m) == 0 ? "L" : "R")
            << " " << s.second
            << " " << lefts[s.first] << " " << rights[s.first]
            << "\n";
    }

    void solve(state pos) const
    {
        state tortoise = pos;
        state hare = pos;
        uint64_t steps = 0;

        while(true)
        {
            //if (steps % 100000 == 0)
                //log(steps, hare);
            //log(steps, hare);
            next(hare);
            ++ steps;
            if (hare == tortoise || hare.first == target)
                break;
            //log(steps, hare);
            next(hare);
            ++ steps;
            if (hare == tortoise || hare.first == target)
                break;

            next(tortoise);
        }

        if (hare == tortoise)
            std::cout << "Infinity " << steps << "\n";
        else
            std::cout << steps << "\n";
    }

    int target;
    std::vector<int> lefts;
    std::vector<int> rights;
};


int main()
{
    int tests;
    std::cin >> tests;

    for (int i = 1; i <= tests; ++i)
    {
        walker w(std::cin);
        std::cout << "Case #" << i << ": ";
        w.solve(std::make_pair(0, 0));
        if (i > 2)
            ;//break;
    }
}
