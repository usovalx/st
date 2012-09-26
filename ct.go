package main

import (
	"fmt"
)

type CachedTask struct {
	t      *Task       // original task
	border int         // node index where we split task into 2 regions
	c1, c2 *CacheSlice // 2 cache slices
}

type CacheStats struct {
	hits   int
	hpath  int64
	misses int
	mpath  int64
}

func makeCachedTask(t Task) *CachedTask {
	border := t.end/2 + 2
	return &CachedTask{&t, border, makeSlice(&t, 0, border), makeSlice(&t, border, t.end)}
}

func makeSlice(t *Task, nMin, nMax int) *CacheSlice {
	mask := uint64(0)
	for i := nMin; i < nMax; i++ {
		mask |= uint64(1) << uint(i)
	}
	return &CacheSlice{t, nMin, nMax, mask, make(map[CacheKey]CacheEnt)}
}

func (c *CachedTask) brent() (res interface{}) {
	tort, hare := c.t.Start(), c.t.Start()
	var path, pow, thr uint64 = 0, 1, 1
	var stats CacheStats

	// handling of cycles inside cache is somewhat sloppy
	// it will raise a panic to signal it
	defer func() {
		if e := recover(); e != nil {
			if v, ok := e.(TravelCycle); ok {
				res = fmt.Sprintf("Infinity %d (stats: %v) (via panic)", path+v.path, stats)
			}
		}
	}()

	for hare.cur != c.t.end {
		path += c.moveC(&hare, &stats)
		if hare == tort {
			return fmt.Sprintf("Infinity %d (stats: %v)", path, stats)
		}
		if path >= thr {
			tort = hare
			for path >= thr {
				pow *= 2
				thr += pow
			}
		}
	}
	return fmt.Sprintf("%d (stats: %v)", path, stats)
}

func (c *CachedTask) moveC(s *State, stats *CacheStats) uint64 {
	if s.cur == c.t.end {
		return 0
	}
	slice := c.c1
	if s.cur >= c.border {
		slice = c.c2
	}

	return slice.moveS(s, stats)
}

type CacheEnt struct {
	path  int    // how long have we traveleld inside cached slice
	node  int    // output node
	state uint64 // state on the exit (only cache part of it)
}

type CacheKey struct {
	node  int    // node through which we have entered cache slice
	state uint64 // state on the entry (only cache part of it)
}

type CacheSlice struct {
	t          *Task
	nMin, nMax int                   // semi-open range of nodes which belong to this slice
	mask       uint64                // mask for this slice
	cache      map[CacheKey]CacheEnt // cache itself
}

func (c *CacheSlice) moveS(s *State, stats *CacheStats) uint64 {
	k := CacheKey{s.cur, s.state & c.mask}
	if r, f := c.cache[k]; f {
		// YES! cache hit
		s.cur = r.node
		s.state = (s.state &^ c.mask) | r.state
		stats.hits++
		stats.hpath += int64(r.path)
		return uint64(r.path)
	}

	path := c.t.travel(c.nMin, c.nMax, s)
	// save it to cache
	if path > (2 << 30) {
		panic("Wooooh. How the hell we have travelled so far inside cached region?")
	}
	c.cache[k] = CacheEnt{int(path), s.cur, (s.state & c.mask)}
	stats.misses++
	stats.mpath += int64(path)
	return path
}

type TravelCycle struct {
	path uint64
}

func (t *Task) travel(nmin, nmax int, s *State) uint64 {
	// we need to implement brent algo here as well
	tort := *s
	var path, pow, thr uint64 = 0, 1, 1
	for s.cur >= nmin && s.cur < nmax {
		path++
		t.moveT(s)
		if *s == tort {
			panic(TravelCycle{path})
		}
		if path == thr {
			tort = *s
			pow *= 2
			thr += pow
		}
	}
	return path
}
