package main

import (
	"fmt"
	"runtime"
	//"runtime/pprof"
	//"os"
)

type Node struct {
	l, r int
	mask uint64
}

type Task struct {
	start, end int
	nodes      []Node
}

type State struct {
	cur   int
	state uint64
}

func main() {
	runtime.GOMAXPROCS(2 * runtime.NumCPU())
	//f, err := os.Create("cpuprof")
	//if err != nil {
	//panic(err)
	//}

	var tests int
	mustr(fmt.Scan(&tests))
	ch := make([]chan interface{}, tests)

	//pprof.StartCPUProfile(f)
	//defer pprof.StopCPUProfile()
	for i := 1; i <= tests; i++ {
		t := readTask()
		ch[i-1] = make(chan interface{}, 1)
		go solve(t, ch[i-1])
	}

	for i, c := range ch {
		r := <-c
		fmt.Printf("Case #%d: %v\n", i+1, r)
	}
}

func solve(t Task, c chan interface{}) {
	//fmt.Println(t)
	r := checkConnectivity(t)
	if r != nil {
		c <- r
		return
	}

	t = reorder(t, findReordering(t))
	ct := makeCachedTask(t)
	c <- ct.brent()
	//return brent(t)
}

func findReordering(t Task) []int {
	// first we need to find most visited node
	v := make([]int, t.end+1) // include entry for final node here -- its easier this way
	for _, n := range t.nodes {
		v[n.l]++
		v[n.r]++
	}
	mi := 0
	for i := range v[1:t.end] {
		if v[i] > v[mi] {
			mi = i
		}
	}

	// now that we know that node mi is most visited node in
	// the graph we will use it as a starting point and for bfs
	order := make([]int, 0)
	vmap := make([]bool, t.end)
	vmap[mi] = true
	for layer := []int{mi}; len(layer) != 0; layer = bfs(&t, layer, vmap) {
		order = append(order, layer...)
	}
	// there might be some nodes which aren't reachable from mi
	for i := range t.nodes {
		if !vmap[i] {
			order = append(order, i)
		}
	}

	return order
}

func bfs(t *Task, nodes []int, vmap []bool) []int {
	children := make([]int, 0)
	for _, ni := range nodes {
		n := &t.nodes[ni]
		if n.l < t.end && !vmap[n.l] {
			children = append(children, n.l)
			vmap[n.l] = true
		}
		if n.r < t.end && !vmap[n.r] {
			children = append(children, n.r)
			vmap[n.r] = true
		}
	}
	return children
}

func reorder(t Task, order []int) Task {
	// we are renumbering it so that t.end stays where it was
	m := make(map[int]int)
	m[t.end] = t.end
	for i, v := range order {
		m[v] = i
	}

	nodes := make([]Node, len(t.nodes))
	for i, n := range t.nodes {
		nodes[m[i]] = Node{m[n.l], m[n.r], uint64(1) << uint(m[i])}
	}
	return Task{m[t.start], t.end, nodes}
}

func checkConnectivity(t Task) interface{} {
	vmap := make([]bool, len(t.nodes))
	path := make([]int, len(t.nodes))
	if !t.dfs(0, vmap, path, 0) {
		return "Unreachable"
	}
	//fmt.Println(path)

	// seems like it might be solvable
	return nil
}

func (t *Task) dfs(node int, vmap []bool, path []int, s int) bool {
	if node == t.end {
		return true
	}
	if vmap[node] {
		return false
	}

	vmap[node] = true
	path[s] = node
	defer func() { vmap[node] = false }()
	n := &t.nodes[node]
	return t.dfs(n.l, vmap, path, s+1) || t.dfs(n.r, vmap, path, s+1)
}

func brent(t Task) interface{} {
	tort, hare := t.Start(), t.Start()
	var path, pow, thr uint64 = 0, 1, 1

	for hare.cur != t.end {
		path++
		t.moveT(&hare)
		if hare == tort {
			return fmt.Sprintf("Infinity %d", path)
		}
		if path == thr {
			tort = hare
			pow *= 2
			thr += pow
		}
	}
	return path
}

func tortoiseAndHare(t Task) interface{} {
	hare, tort := t.Start(), t.Start()
	path := uint64(0)

	for {
		//t.log(path, hare)
		path++
		t.moveT(&hare)
		if hare.cur == t.end || hare == tort {
			break
		}
		//t.log(path, hare)
		path++
		t.moveT(&hare)
		if hare.cur == t.end || hare == tort {
			break
		}
		t.moveT(&tort)
	}

	if hare == tort {
		return fmt.Sprintf("Infinity %d", path)
	}
	return path
}

func (t Task) log(path uint64, s State) {
	n := t.nodes[s.cur]
	d := "L"
	if s.state&n.mask != 0 {
		d = "R"
	}
	fmt.Println(path, s.cur, d, s.state, n.l, n.r)
}

func (t Task) String() string {
	s := "Task{\n"
	s = s + fmt.Sprintf("start: %d\n", t.start)
	s = s + fmt.Sprintf("end: %d\n", t.end)
	for i := 0; i < len(t.nodes); i++ {
		s = s + fmt.Sprintf("%d: %d %d\n", i, t.nodes[i].l, t.nodes[i].r)
	}
	s = s + "}"

	return s
}

func (t *Task) moveT(s *State) {
	n := &t.nodes[s.cur]
	if s.state&n.mask == 0 {
		s.cur = n.l
	} else {
		s.cur = n.r
	}
	s.state ^= n.mask
}

func readTask() Task {
	var targ int
	nodes := make([]Node, 0)

	mustr(fmt.Scan(&targ))
	for i := 1; i < targ; i++ {
		var l, r int
		mustr(fmt.Scan(&l, &r))
		nodes = append(nodes, Node{l - 1, r - 1, (uint64(1) << uint(i-1))})
	}

	return Task{0, targ - 1, nodes}
}

func (t *Task) Start() State {
	return State{t.start, 0}
}

func mustr(v interface{}, e error) {
	if e != nil {
		panic(e)
	}
}
