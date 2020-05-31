MODULES=last_fm		\
				now_playing_component \
			  disc_component \
				cdplayer	 \
				disc		   \
				track		   \
				cddb		   \
				cd_drive 	 \
				main
OBJECTS=$(foreach MODULE, ${MODULES}, build/${MODULE}.o)
LIBS		= libcdio_paranoia portaudio-2.0 libcddb gtkmm-3.0 jsoncpp
CFLAGS  = -std=c++17 -O2 -Wall `pkg-config --cflags ${LIBS}` `curlpp-config --cflags` -g
LDFLAGS = `pkg-config --libs ${LIBS}` `curlpp-config --libs`
EXEC=cdplayer

all: build ${EXEC}

${EXEC}: ${OBJECTS}
	g++ $^ -o $@ ${LDFLAGS}

build/%.o : src/%.cc
	g++ -c $< -o $@ ${CFLAGS}

build:
	mkdir -p build

clean:
	rm -rf build
	rm ${EXEC}

