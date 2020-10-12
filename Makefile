libscout.so: scout.c
	cc scout.c -g -o libscout.so -shared -Wall -Wextra -Wpedantic -D__LIBSCOUT_INTERNAL__ -D__LIBSCOUT_TYPEDEF__
example: example.c libscout.so
	cc example.c -g -o example -lGL -lglfw -lGLEW -lm -lscout -L. -Wl,-rpath,`pwd` -Wall -Wextra -Wpedantic -D__LIBSCOUT_TYPEDEF__
all: libscout.so example
install: libscout.so
	mv libscout.so /usr/lib/
	mv scout.h /usr/include/
