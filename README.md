A demonstration of issue with passing a file descriptor to libblockdev for fsck.
Bellow is an example run.
~~~~
$ make && sudo ./progress -g -b /file.img
[sudo] password for user:
Running libblockdev fsck.
FD is 3
LOG 6: Running [1] e2fsck -f -n /file.img -C 3 ...
LOG 6: stdout[1]:
LOG 6: stderr[1]: Error validating file descriptor 3: Bad file descriptor
e2fsck: Invalid completion information file descriptor

LOG 6: ...done [1] (exit code: 8)
Size of the file with progress info: 0 B

Running glib fsck.
FD is 4
running command: e2fsck -f -n /file.img -C 3
STDOUT:
Pass 1: Checking inodes, blocks, and sizes
Pass 2: Checking directory structure
Pass 3: Checking directory connectivity
Pass 4: Checking reference counts
Pass 5: Checking group summary information
/file.img: 399488/577088 files (0.2% non-contiguous), 2106027/2304000 blocks

STDERR:
e2fsck 1.43.4 (31-Jan-2017)

Size of the file with progress info: 3508702 B
~~~~