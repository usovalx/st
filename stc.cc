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
    struct Node {
        int l;
        int r;
        // uint64_t mask;    // it's a bit strange, but C++ version is about 10% slower with it
    };

    walker(std::istream& is)
    {
        is >> target;

        for (int i = 1; i < target; ++i)
        {
            int left, right;
            is >> left >> right;
            nodes.push_back(Node{left-1, right-1});
        }
        target -= 1;
    }

    void log(uint64_t path, const state& s) const 
    {
        uint64_t m = uint64_t(1) << s.first;
        std::cout
            << path << " "
            << s.first << " "
            << ((s.second & m) == 0 ? "L" : "R")
            << " " << s.second
            << " " << nodes[s.first].l << " " << nodes[s.first].r
            << "\n";
    }

    std::string solve(state pos) const
    {
        state tort, hare;
        uint64_t path, pow, thr;

        tort = hare = pos;
        path = 0;
        pow = thr = 1;

        while (hare.first != target)
        {
            ++path;
            move(hare);
            if (hare == tort)
                break;
            if (path == thr)
            {
                tort = hare;
                pow *= 2;
                thr += pow;
            }
        }

        std::ostringstream os;
        if (hare.first == target)
            os << path;
        else
            os << "Infinity " << path;
        return os.str();
    }

    void move(state &cur) const
    {
        const Node& n = nodes[cur.first];
        uint64_t m = uint64_t(1) << cur.first;
        uint64_t v = cur.second & m;
        cur.first = (v == 0 ? n.l : n.r);
        cur.second ^= m;
    }

    int target;
    std::vector<Node> nodes;
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
