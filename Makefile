
st: *.go
	go build

stg: *.go
	go build -compiler gccgo -gccgoflags -O3
	mv st stg

stc: prog.cc
	g++ --std=c++11 -O3 -o stc prog.cc
