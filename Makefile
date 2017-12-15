GCC=gcc
CFLAGS=-Wall -g -pedantic `pkg-config --cflags glib-2.0 blockdev `
CLIBS=`pkg-config --libs glib-2.0 blockdev` -lbd_utils -lm
RM=@rm -rf

.PHONY: progress

default: progress

clean:
	$(RM) demo-1-libblockdev progress

progress:
	$(GCC) $(CFLAGS) $(CLIBS) -o progress progress.c

demo-1-libblockdev:
	$(GCC) $(CFLAGS) $(CLIBS) -o demo-1-libblockdev demo-1-libblockdev.c

