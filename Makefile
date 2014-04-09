all:	libsif.so

libsif.so: sif-io.c sif-io.h
	gcc -fPIC -D_USE_FILE_OFFSET64 -D_POSIX_SOURCE -D_GNU_SOURCE \
                  -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE \
                  -DHAVE_LONG_LONG -Wpadded \
                  -D_USE_LARGEFILE64 -g -Wall -std=c99 -c sif-io.c  -o sif-io.o
	gcc -lm -shared -W1,-soname,libsif.so -o libsif.so sif-io.o

doc: doc/index.dox sif-io.h doc/doxygen.sty doc/header.tex Doxyfile
	doxygen
	cp doc/doxygen.sty latex
	cd latex && make pdf && cd ..

clean:
	rm -f libsif.so
