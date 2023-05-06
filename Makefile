SHELL = /bin/bash

MODULES = resources             \
          bluetooth_component   \
          last_fm               \
          now_playing_component \
          disc_component        \
          application           \
          cd_drive              \
          main
OBJECTS = $(foreach MODULE, ${MODULES}, build/${MODULE}.o)
LIBS		= libcdio_paranoia portaudio-2.0 gtkmm-4.0 glibmm-2.68 curlpp jsoncpp
CFLAGS  = -std=c++17 -O2 -Wall `pkg-config --cflags ${LIBS}` `curlpp-config --cflags` -g
LDFLAGS = `pkg-config --libs ${LIBS}` -lstdc++fs `curlpp-config --libs` -lbluez -ldiscdb
EXEC    = cdplayer

all: build/ ${EXEC}

install: ${EXEC}
	cp $< /usr/bin

${EXEC}: ${OBJECTS}
	g++ $^ -o $@ ${LDFLAGS}

format:
	astyle -rnNCS *.{h,cc}

src/resources.cc: resources.xml
	glib-compile-resources --target=$@ --generate-source $<

build/%.o : src/%.cc
	g++ -c $< -o $@ ${CFLAGS}

build/:
	mkdir -p build

clean:
	rm -rf build
	rm -f src/resources.cc
	rm -f ${EXEC}

distclean:
	rm -f /usr/bin/${EXEC}

