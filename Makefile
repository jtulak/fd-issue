GCC=@gcc
CFLAGS=-Wall -pedantic `pkg-config --cflags blockdev glib-2.0`
CLIBS=`pkg-config --libs blockdev glib-2.0` -lm
RM=@rm -rf

.PHONY: progress

default: progress

clean:
	$(RM) demo-1-libblockdev progress

progress:
	$(GCC) $(CFLAGS) $(CLIBS) -o progress progress.c

demo-1-libblockdev:
	$(GCC) $(CFLAGS) $(CLIBS) -o demo-1-libblockdev demo-1-libblockdev.c

