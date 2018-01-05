Example of progress report
==========================
A demonstration of libblockdev and progress report for e2fsck. The script
uses sudo to run fsck, but the compilation is done under your user account.

Bellow is an example run.
~~~
$ ./run.sh /dev/sdb1
gcc -Wall -g -pedantic `pkg-config --cflags glib-2.0 blockdev ` `pkg-config --libs glib-2.0 blockdev` -lbd_utils -lm -o progress progress.c

Started 'e2fsck -f -n /dev/sdb1 -C 1'
Progress: 0%
Progress: 1%
Progress: 2%
Progress: 3%
...
Progress: 98%
Progress: 99%
Progress: 100%

Completed

real 2.51
user 0.40
sys 0.14
~~~~