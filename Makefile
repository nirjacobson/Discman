SHELL = /bin/bash

MODULES = application           \
          album_art_provider    \
          album_art_component   \
          bluetooth_component   \
          cd_drive              \
          cd_ripper             \
          disc_component        \
          drive_manager         \
          last_fm               \
          main                  \
          now_playing_component \
          resources             \
          spotify

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

CFLAGS    = -std=c++20 -O2 -Wall `pkg-config --cflags ${LIBS}` `curlpp-config --cflags` -g
LDFLAGS   = `pkg-config --libs ${LIBS}` -lstdc++fs `curlpp-config --libs` -ludisks2cc -lbluez -ldiscdb
EXEC      = discman
BIN_DIR   = /usr/bin
LINENOPAT = ^[^:]+:([^:]+):(.+)$

all: build/ ${EXEC}

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

build/:
	mkdir -p build

clean:
	rm -rf build
	rm -f src/resources.cc
	rm -f ui/discman.glade
	rm -f ${EXEC}

distclean:
	rm -f /usr/bin/${EXEC}

