#include <cstdint>
#include <iostream>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

#include <unistd.h>

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

    std::string solve(state pos) const
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

        std::ostringstream os;
        if (hare == tortoise)
            os << "Infinity " << steps;
        else
            os << steps;
        return os.str();
    }

    int target;
    std::vector<int> lefts;
    std::vector<int> rights;
};


int main(int argc, char **argv)
{
    bool doLimit = false;
    std::set<int> limit;
    const char *opts = "l:";
    int opt;
    while ((opt = getopt(argc, argv, opts)) != -1)
        switch (opt)
        {
            case 'l':
                {
                    doLimit = true;
                    // parse
                    std::istringstream is(optarg);
                    int i;
                    while (is >> i)
                    {
                        limit.insert(i);
                        if (is.peek() == ',')
                            is.ignore();
                    }
                    break;
                }
            default:
                return 1;
        }

    int tests;
    std::cin >> tests;

    for (int i = 1; i <= tests; ++i)
    {
        walker w(std::cin);
        if (!doLimit || limit.count(i))
        {
            auto r = w.solve(std::make_pair(0, 0));
            std::cout << "Case #" << i << " " << r << "\n";
        }
    }
}
