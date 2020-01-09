CXX=clang++
CXXFLAGS=-g -Wall -pedantic -MT -static -Xclang -flto-visibility-public-std

SRC=wiiurpxtool.cpp
OBJ=$(SRC:.cpp=.o)
BIN=wiiurpxtool.exe

all: $(ZLIBTARGET) $(BIN)
	rm -f *.o
	@echo Done

$(BIN): $(OBJ)
	$(CXX) -o $(BIN) $^ -lzlibstat

clean:
	rm -f *.o
	rm $(BIN)
