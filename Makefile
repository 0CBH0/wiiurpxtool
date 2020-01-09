DEPPATH=./dep
CXX=clang++
CXXFLAGS=-g -Wall -pedantic -MT -static -Xclang -flto-visibility-public-std -I $(DEPPATH)

SRC=wiiurpxtool.cpp
OBJ=$(SRC:.cpp=.o)
BIN=wiiurpxtool.exe

all: $(ZLIBTARGET) $(BIN)
	rm -f *.o
	@echo Done

$(BIN): $(OBJ)
	$(CXX) -o $(BIN) $^ -L $(DEPPATH) -lzlibstat

clean:
	rm -f *.o
	rm $(BIN)
