# Copyright (C) 2007 Andriy Lesyuk; All rights reserved.

GCC = gcc
CFLAGS = -g -Wall -D_GNU_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CDINDEX_FLAGS = `pkg-config --cflags MagickWand` `pkg-config --cflags libavformat`

CDILIBS = -lm -larchive -lraw -lffmpegthumbnailer `pkg-config --libs MagickWand` `pkg-config --libs libavformat` `pkg-config --libs libavcodec` `pkg-config --libs libavutil`

cdindex: bin bin/cdindex bin/cdbrowse bin/cdfind bin/cdupgrade

bin:
	mkdir bin

bin/cdindex: bin/main.o bin/index.o bin/base.o bin/plugin.o bin/archive.o bin/external.o bin/extract.o bin/audio.o bin/image.o bin/video.o bin/rawimage.o
	$(GCC) $(CDILIBS) -o bin/cdindex bin/main.o bin/index.o bin/base.o \
	bin/plugin.o bin/archive.o bin/external.o bin/extract.o bin/audio.o \
	bin/image.o bin/video.o bin/rawimage.o

bin/main.o: src/main.c src/index.h src/cdindex.h src/base.h src/plugin.h
	$(GCC) -c $(CFLAGS) $(CDINDEX_FLAGS) -o bin/main.o src/main.c

bin/index.o: src/index.c src/index.h src/data.h src/cdindex.h src/plugin.h
	$(GCC) -c $(CFLAGS) $(CDINDEX_FLAGS) -o bin/index.o src/index.c

bin/base.o: src/base.c src/base.h src/data.h src/cdindex.h
	$(GCC) -c $(CFLAGS) $(CDINDEX_FLAGS) -o bin/base.o src/base.c

bin/plugin.o: src/plugin.c src/plugin.h
	$(GCC) -c $(CFLAGS) $(CDINDEX_FLAGS) -o bin/plugin.o src/plugin.c

bin/archive.o: src/archive.c src/plugin.h src/cdindex.h
	$(GCC) -c $(CFLAGS) $(CDINDEX_FLAGS) -o bin/archive.o src/archive.c

bin/external.o: src/external.c src/plugin.h src/cdindex.h
	$(GCC) -c $(CFLAGS) $(CDINDEX_FLAGS) -o bin/external.o src/external.c

bin/extract.o: src/extract.c src/extract.h src/cdindex.h src/base.h
	$(GCC) -c $(CFLAGS) $(CDINDEX_FLAGS) -o bin/extract.o src/extract.c

bin/audio.o: src/audio.c src/audio.h src/extract.h src/cdindex.h
	$(GCC) -c $(CFLAGS) $(CDINDEX_FLAGS) -o bin/audio.o src/audio.c

bin/image.o: src/image.c src/image.h
	$(GCC) -c $(CFLAGS) $(CDINDEX_FLAGS) -o bin/image.o src/image.c

bin/video.o: src/video.c src/video.h
	$(GCC) -c $(CFLAGS) $(CDINDEX_FLAGS) -o bin/video.o src/video.c

bin/rawimage.o: src/rawimage.c src/image.h src/extract.h
	$(GCC) -c $(CFLAGS) $(CDINDEX_FLAGS) -o bin/rawimage.o src/rawimage.c

bin/cdbrowse: bin/browse.o
	$(GCC) -o bin/cdbrowse bin/browse.o

bin/browse.o: src/browse.c src/data.h src/cdindex.h src/audio.h
	$(GCC) -c $(CFLAGS) -o bin/browse.o src/browse.c

bin/cdfind: bin/find.o bin/search.o
	$(GCC) -o bin/cdfind bin/find.o bin/search.o

bin/find.o: src/find.c src/find.h src/data.h src/search.h src/cdindex.h
	$(GCC) -c $(CFLAGS) -o bin/find.o src/find.c

bin/search.o: src/search.c src/search.h src/data.h src/cdindex.h
	$(GCC) -c $(CFLAGS) -o bin/search.o src/search.c

bin/cdupgrade: bin/upgrade.o
	$(GCC) -o bin/cdupgrade bin/upgrade.o

bin/upgrade.o: src/upgrade.c
	$(GCC) -c $(CFLAGS) -o bin/upgrade.o src/upgrade.c

clean:
	rm -rf bin

unkate:
	find -name '*~' -exec rm {} \;
