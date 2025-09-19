path_rot = ../
path_tts = ${path_rot}/tts-lz
path_mer = ${path_rot}/merlin-c
path_spt = ${path_mer}/sptk
path_world = ${path_mer}/world
objs = $(patsubst %.cpp, %.o, $(wildcard ./*.cpp))

CXXFLAGS = -O1 -g -Wall -fPIC

synth: ${objs}
	g++ ${CXXFLAGS} ${objs} -o synth -lm

%.o: %.cpp
	g++ ${CXXFLAGS} -Isrc -lstdc++ -c $< -o $@ -I ${path_world} -lm