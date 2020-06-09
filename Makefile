SHELL=/bin/bash

MODULES=bluetooth_component   \
				last_fm		            \
				now_playing_component \
			  disc_component        \
				application	          \
				cd_drive 	            \
				main
OBJECTS=$(foreach MODULE, ${MODULES}, build/${MODULE}.o)
LIBS		= libcdio_paranoia portaudio-2.0 gtkmm-3.0 jsoncpp
CFLAGS  = -std=c++17 -O2 -Wall `pkg-config --cflags ${LIBS}` `curlpp-config --cflags` -I../libdiscdb/src -I../libbluez/src -g
LDFLAGS = `pkg-config --libs ${LIBS}` -lstdc++fs `curlpp-config --libs` -L../libdiscdb -ldiscdb -L../libbluez -lbluez
EXEC=cdplayer

all: ${EXEC}

${EXEC}: ${OBJECTS}
	g++ $^ -o $@ ${LDFLAGS}

format:
	astyle -rnCS *.{h,cc}

build/%.o : builddir src/%.cc
	g++ -c $(word 2, $^) -o $@ ${CFLAGS}

builddir:
	mkdir -p build

clean:
	rm -rf build
	rm ${EXEC}

