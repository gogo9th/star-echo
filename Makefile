SRC :=  mediaProcess.cpp q2cathedral.cpp DNSE_CH.cpp

CFLAGS = -O2 -g -pthread -std=c++17

.PHONY: clean

SRC_OBJ := $(SRC:.cpp=.o)

.c.o:
	g++ -c $(CFLAGS) $< -o $@

.cpp.o:
	g++ -c $(CFLAGS) $< -o $@

all:  q2cathedral 

q2cathedral: $(SRC_OBJ)
	g++ -o $@ $(SRC_OBJ) -lavformat -lavcodec -lavutil -lswresample -lboost_program_options -lpthread
	#g++ -DLIBSSH_STATIC -static -static-libgcc -static-libstdc++ -o $@ $(SRC_OBJ) -lavformat -lavcodec -lavutil -lswresample -lboost_program_options -lpthread -ldl  

clean: 
	rm -f $(SRC_OBJ) ./q2cathedral *.o
