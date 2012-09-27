#include <algorithm>
#include <cstdint>
#include <iostream>
#include <set>
#include <map>
#include <unordered_map>
#include <sstream>
#include <utility>
#include <vector>

#include <unistd.h>

using namespace std;

class Task
{
public:
    // current node, state map
    typedef std::pair<int, uint64_t> state;
    struct Node {
        int l;
        int r;
        // uint64_t mask;    // it's a bit strange, but C++ version is about 10% slower with it
    };

    Task(std::istream& is)
    {
        start = 0;
        is >> end;

        for (int i = 1; i < end; ++i)
        {
            int left, right;
            is >> left >> right;
            nodes.push_back(Node{left-1, right-1});
        }
        end -= 1;
    }

    // build from other task by renumbering nodes
    Task(const Task& t, const std::vector<int>& order)
    {
        std::map<int, int> m;
        int i = 0;
        for (int n : order)
        {
            m[n] = i++;
        }
        m[t.end] = t.end;

        start = m[t.start];
        end = t.end;
        nodes.resize(t.nodes.size());
        i = 0;
        for (const Node& on : t.nodes)
        {
            Node& n = nodes[m[i++]];
            n.l = m[on.l];
            n.r = m[on.r];
        }
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

    state startState() const
    {
        return state(start, 0);
    }

    std::string brent() const
    {
        state tort, hare;
        uint64_t path, pow, thr;

        tort = hare = startState();
        path = 0;
        pow = thr = 1;

        while (hare.first != end)
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
        if (hare.first == end)
            os << path;
        else
            os << "Infinity " << path;
        return os.str();
    }

    uint64_t travel(int nmin, int nmax, state& s)
    {
        state tort = s;
        uint64_t path, pow, thr;
        path = 0;
        pow = thr = 1;

        while (s.first >= nmin && s.first < nmax)
        {
            ++path;
            move(s);
            if (s == tort)
                throw path;
            if (path == thr)
            {
                tort = s;
                pow *= 2;
                thr += pow;
            }
        }

        return path;
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
        if (n == end)
            return true;
        if (vmap[n])
            return false;

        vmap[n] = true;
        bool r = dfs(nodes[n].l, vmap) || dfs(nodes[n].r, vmap);
        vmap[n] = false;
        return r;
    }

    int start;
    int end;
    std::vector<Node> nodes;
};

struct CacheStats {
    int hits, misses;
    uint64_t hpath, mpath;
};

std::ostream& operator<<(std::ostream& os, const CacheStats& stats)
{
    os << "{" << stats.hits
        << " " << stats.hpath
        << " " << stats.misses
        << " " << stats.mpath
        << "}";
    return os;
}

template<typename T, typename U>
struct mhash
{
    size_t operator()(const std::pair<T,U>& p) const noexcept
    {
        return std::hash<T>()(p.first) ^ std::hash<U>()(p.second);
    }
};

class CacheSlice
{
public:
    typedef std::pair<int, uint64_t> Key;
    struct Ent {
        int path, node;
        uint64_t state;
    };

    CacheSlice(Task& t, int nFrom, int nTo)
        : task(t), nMin(nFrom), nMax(nTo), mask(0)
    {
        for (int i = nMin; i < nMax; ++i)
        {
            mask |= uint64_t(1) << i;
        }
    }

    uint64_t moveS(Task::state& s, CacheStats& stats)
    {
        Key k(s.first, s.second & mask);
        auto ent = cache.find(k);
        if (ent != cache.end())
        {
            // cache hit!!!!
            s.first = ent->second.node;
            s.second = (s.second & ~mask) | ent->second.state;
            stats.hits++;
            stats.hpath += ent->second.path;
            return ent->second.path;
        }

        uint64_t path = task.travel(nMin, nMax, s);
        cache[k] = Ent{int(path), s.first, s.second & mask};
        stats.misses++;
        stats.mpath += path;
        return path;
    }

    Task &task;
    int nMin, nMax;
    uint64_t mask;
    std::unordered_map<Key, Ent, mhash<int, uint64_t>> cache;
};

class CachedTask
{
public:
    CachedTask(Task t)
    :
        task(t),
        border(t.end/2 + 2),
        c1(task, 0, border),
        c2(task, border, t.end)
    {
    }

    std::string brent()
    {
        Task::state tort, hare;
        uint64_t path, pow, thr;
        CacheStats stats = {0, 0, 0, 0};

        tort = hare = task.startState();
        path = 0;
        pow = thr = 1;

        try
        {
            while (hare.first != task.end)
            {
                path += moveC(hare, stats);
                if (hare == tort)
                    break;
                if (path >= thr)
                {
                    tort = hare;
                    while (path >= thr)
                    {
                        pow *= 2;
                        thr += pow;
                    }
                }
            }
        }
        catch(uint64_t p)
        {
            std::ostringstream os;
            os << "Infinity " << path+p << " (stats: " << stats << ") (via THROW)";
            return os.str();
        }

        std::ostringstream os;
        if (hare.first == task.end)
            os << path << " (stats: " << stats << ")";
        else
            os << "Infinity " << path << " (stats: " << stats << ")";
        return os.str();

    }

    uint64_t moveC(Task::state &s, CacheStats& stats)
    {
        if (s.first == task.end)
            return 0;
        return (s.first <  border ? c1 : c2).moveS(s, stats);
    }

    Task task;
    int border;
    CacheSlice c1;
    CacheSlice c2;
};

std::vector<int> bfs(const Task& t, const std::vector<int>& ns, std::vector<bool>& vmap)
{
    std::vector<int> r;
    for (int i : ns)
    {
        const Task::Node& n = t.nodes[i];
        if (n.l < t.end && !vmap[n.l])
        {
            r.push_back(n.l);
            vmap[n.l] = true;
        }
        if (n.r < t.end && !vmap[n.r])
        {
            r.push_back(n.r);
            vmap[n.r] = true;
        }
    }
    return r;
}

std::vector<int> find_reordering(const Task& t)
{
    // find most visited node
    std::vector<int> v(t.end+1, 0);
    for (const Task::Node& n : t.nodes)
    {
        ++v[n.l];
        ++v[n.r];
    }

    int mi = std::distance(v.begin(), std::max_element(v.begin(), v.end()-1));

    // now do bfs search from node mi to find new order of the nodes
    std::vector<int> order;
    std::vector<bool> vmap(t.nodes.size(), false);
    vmap[mi] = true;
    for (auto layer = std::vector<int>{mi}; layer.size() != 0; layer = bfs(t, layer, vmap))
    {
        order.insert(order.end(), layer.begin(), layer.end());
    }

    // add all leftover nodes to the end
    for (int i = 0; i < t.end; ++i)
    {
        if (!vmap[i])
            order.push_back(i);
    }

    return order;
}

std::string solve(Task t, bool cached)
{
    auto r = t.check_connectivity(t.startState());
    if (r != "")
        return r;

    if (cached)
    {
        Task t2(t, find_reordering(t));
        CachedTask ct(t2);
        return ct.brent();
    }
    else
    {
        return t.brent();
    }
}


int main(int argc, char **argv)
{
    bool doLimit = false;
    bool doCached = false;
    std::set<int> limit;

    const char *opts = "cl:";
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
            case 'c':
                doCached = true;
                break;
            default:
                return 1;
        }

    int tests;
    std::cin >> tests;

    for (int i = 1; i <= tests; ++i)
    {
        Task t(std::cin);
        if (!doLimit || limit.count(i))
        {
            auto r = solve(t, doCached);
            std::cout << "Case #" << i << ": " << r << "\n";
        }
    }
}
