CPPFLAGS = -flto -Wno-unknown-pragmas -std=c++11 -Wall -pedantic -g3 -pthread -mavx2

DEPFILES = $(wildcard *.h) $(wildcard *.hpp)
CPPFILES = $(wildcard *.cpp)
OBJFILES = $(patsubst %, %.o, $(CPPFILES))

LDLIBS   = 
TARGETS  = cse221_o0 cse221_o1 cse221_o2

ifeq ($(slink), 1)
	CPPFLAGS += -static -static-libstdc++
	LDLIBS += -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
endif

default: cse221_o1

all: $(TARGETS)

cse221_o0: $(CPPFILES) $(DEPFILES)
	g++ $(CPPFLAGS) -O0 -o $@ $(CPPFILES) $(LDLIBS)

cse221_o1: $(CPPFILES) $(DEPFILES)
	g++ $(CPPFLAGS) -O1 -o $@ $(CPPFILES) $(LDLIBS)

cse221_o2: $(CPPFILES) $(DEPFILES)
	g++ $(CPPFLAGS) -O2 -o $@ $(CPPFILES) $(LDLIBS)

.PHONY: clean

clean:
	rm -f $(TARGETS) $(OBJFILES)