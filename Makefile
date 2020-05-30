MODULES=now_playing_component \
			  disc_component \
				cdplayer	 \
				disc		   \
				track		   \
				cddb		   \
				cd_drive 	 \
				main
OBJECTS=$(foreach MODULE, ${MODULES}, build/${MODULE}.o)
CFLAGS  = -std=c++17 -O2 -Wall `pkg-config --cflags libcdio_paranoia portaudio-2.0 libcddb gtkmm-3.0` -g
LDFLAGS = `pkg-config --libs libcdio_paranoia portaudio-2.0 libcddb gtkmm-3.0`
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

