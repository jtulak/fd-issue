A demonstration of libblockdev and progress report for e2fsck. Edit run.sh
script and change the IO bandwidth limit to get a reasonable run time,
depending on the size of the fscked device, by default it is 10M. The script
uses sudo to run fsck, but the compilation is done under your user account.

At this moment, libblockdev buffers output of fsck until it is finished, so
the progress report is not useful (it gets printed out at once after fsck
finished).

Bellow is an example run.
~~~~
$ ./run.sh /dev/sdb1
Running scope as unit: run-r9399cad904024a77bc6a7830f64d9835.scope
LOG 6: Running [1] e2fsck -f -n /dev/sdb1 -C 1 ...
LOG 6: stdout[1]: Pass 1: Checking inodes, blocks, and sizes
Pass 1/5 [100%]
Pass 2: Checking directory structure
Pass 2/5 [100%]
Pass 3: Checking directory connectivity
Pass 3/5 [100%]
Pass 4: Checking reference counts
Pass 4/5 [100%]
Pass 5: Checking group summary information
Pass 5/5 [100%]
/dev/sdb1: 113606/720896 files (0.7% non-contiguous), 2879232/2883328 blocks

LOG 6: stderr[1]: e2fsck 1.43.4 (31-Jan-2017)

LOG 6: ...done [1] (exit code: 0)

intercepted: 323619
real 7.51
user 0.51
sys 0.38
~~~~