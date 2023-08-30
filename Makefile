sources = $(wildcard src/*.cpp)
includes = $(wildcard include/*.hpp)

CXXFLAGS = -g -O2
CPPFLAGS = -D LINUX
# LDFLAGS = -lmetis -lGKlib -lSDL2 -ldl
LDFLAGS = -lmetis -lSDL2 -ldl
CXX = g++


clod: $(sources) $(includes)
	$(CXX) $(CPPFLAGS) -o clod $(CXXFLAGS) $(sources) -Iinclude \
		$(LDFLAGS)
