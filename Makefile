SRC :=  mediaProcess.cpp star_echo.cpp DNSE_CH_params.cpp DNSE_BE_params.cpp DNSE_AuUp_params.cpp

CFLAGS = -O2 -g -pthread -std=c++17

# STATIC_LIBS = libswresample.a libavformat.a libavcodec.a libavutil.a -lboost_program_options ../../Desktop/fdk-aac/.libs/libfdk-aac.a -lmp3lame -lopus -lboost_program_options -DLIBSSH_STATIC -lssh -lcrypto -lsnappy  -lz -lpthread -static-libgcc -static-libstdc++
# $ sudo apt install libssh-dev libsnappy-dev libaom-dev libass-dev libfdk-aac-dev libmp3lame-dev libopus-dev libvorbis-dev libvpx-dev libx264-dev
# $ git clone https://github.com/FFmpeg/FFmpeg/
# $ ./configure --enable-pthreads --enable-gpl
# $ make 

#static:
#	g++ -static $(CFLAGS) -o star_echo mediaProcess.cpp DNSE_CH.cpp star_echo.cpp $(STATIC_LIBS) 

.PHONY: clean

SRC_OBJ := $(SRC:.cpp=.o)

.c.o: 
	g++ -c $(CFLAGS) $< -o $@

.cpp.o:
	g++ -c $(CFLAGS) $< -o $@

all:  star_echo 


star_echo: $(SRC_OBJ)
	g++ -o $@ $(SRC_OBJ) -lavformat -lavcodec -lavutil -lswresample -lboost_program_options -lpthread

clean: 
	rm -f star_echo *.o *.a
