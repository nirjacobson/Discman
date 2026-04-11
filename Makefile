SHELL = /bin/bash

MODULES = application                     \
          album_art/album_art_provider    \
          album_art/last_fm               \
          album_art/spotify               \
          cd/cd_drive                     \
          cd/cd_ripper                    \
          component/component             \
          component/album_art_component   \
          component/bluetooth_component   \
          component/disc_component        \
          component/now_playing_component \
          drive_manager/drive_manager     \
          main                            \
          resources                       \

OBJECTS   = $(foreach MODULE, ${MODULES}, build/${MODULE}.o)
LIBS      = curlpp           \
            gtkmm-4.0        \
            glibmm-2.68      \
            jsoncpp          \
            libavformat      \
            libavcodec       \
            libavutil        \
            libcdio_paranoia \
            libswresample    \
            openssl          \
            portaudio-2.0    \
            stb

CFLAGS    = -std=c++20 -O2 -Wall -Isrc `pkg-config --cflags ${LIBS}` `curlpp-config --cflags` -g
LDFLAGS   = `pkg-config --libs ${LIBS}` -lstdc++fs `curlpp-config --libs` -ludisks2cc -lbluez -ldiscdb
EXEC      = discman
BIN_DIR   = /usr/bin
LINENOPAT = ^[^:]+:([^:]+):(.+)$

.PHONY: docs

all: build/ ${EXEC}

docs:
	rm -rf docs/
	doxygen doxygen.txt

install: ${EXEC}
	install -D $< -t ${DESTDIR}${BIN_DIR}

${EXEC}: ${OBJECTS}
	g++ $^ -o $@ ${LDFLAGS}

format:
	astyle -rnNCS *.{h,cc}

ui/discman.glade: ui/discman.3.glade
	LINES=
	while IFS= read -r line; do \
		LINES=`echo $$line | sed -nre 's/${LINENOPAT}/\1/p'`d\;$$LINES; \
	done < <(gtk4-builder-tool simplify --3to4 $< 2>&1 | grep "not found"); \
	sed -i "$$LINES" $<
	gtk4-builder-tool simplify --3to4 $< > $@

src/resources.cc: resources.xml ui/discman.glade
	glib-compile-resources --target=$@ --generate-source $<

build/%.o : src/%.cc
	g++ -c $< -o $@ ${CFLAGS}
build/album_art/%.o : src/album_art/%.cc
	mkdir -p build/album_art
	g++ -c $< -o $@ ${CFLAGS}
build/cd/%.o : src/cd/%.cc
	mkdir -p build/cd
	g++ -c $< -o $@ ${CFLAGS}
build/component/%.o : src/component/%.cc
	mkdir -p build/component
	g++ -c $< -o $@ ${CFLAGS}
build/drive_manager/%.o : src/drive_manager/%.cc
	mkdir -p build/drive_manager
	g++ -c $< -o $@ ${CFLAGS}

build/:
	mkdir -p build

clean:
	rm -rf build
	rm -f src/resources.cc
	rm -f ui/discman.glade
	rm -f ${EXEC}

distclean:
	rm -f /usr/bin/${EXEC}

