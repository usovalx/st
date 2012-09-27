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
        start = 0;
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

    std::string solve() const
    {
        state startState(start, 0);
        auto r = check_connectivity(startState);
        if (r != "")
            return r;
        return brent(startState);
    }

    std::string brent(state pos) const
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

    std::string check_connectivity(state start) const
    {
        std::vector<bool> vmap(nodes.size(), false);
        if (!dfs(start.first, vmap))
            return "Unreachable";
        else
            return "";
    }

    bool dfs(int n, std::vector<bool>& vmap) const
    {
        if (n == target)
            return true;
        if (vmap[n])
            return false;

        vmap[n] = true;
        bool r = dfs(nodes[n].l, vmap) || dfs(nodes[n].r, vmap);
        vmap[n] = false;
        return r;
    }

    int start;
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
            auto r = w.solve();
            std::cout << "Case #" << i << " " << r << "\n";
        }
    }
}
