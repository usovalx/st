
st: *.go
	go build

stg: *.go
	go build -compiler gccgo -gccgoflags -O3
	mv st stg

stc: stc.cc
	g++ --std=c++11 -W -Wall -O3 -o stc stc.cc
