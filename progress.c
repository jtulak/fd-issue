#include <glib.h>
#include <blockdev/blockdev.h>
#include <blockdev/fs.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>

/**
 * Truncate the progress file before we start writing to it.
 * We don't want the file to contain old data.
 */
void 
truncate_fd_file(char *fname)
{
    FILE *f;
    f = fopen(fname, "w");
    fclose(f);
}

/**
 * A log handler for libblockdev
 */
void
logprint (gint level, const gchar *msg)
{
    /* create/truncate the file with progress */
    printf("LOG %d: %s\n", level, msg);
}

/**
 * Run fsck using glib's spawn function.
 */
int
fsck_glib(char *fs, char *fd_str)
{
    gint ret;
    g_autoptr(GError) error = NULL;
    char *cmd[] = {"e2fsck", "-f", "-n", fs, "-C", fd_str, NULL};
    gchar *stdout = NULL;
    gchar *stderr = NULL;

    /* run the command */
    printf("running command: %s %s %s %s %s %s\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5]);
    g_spawn_sync(NULL, cmd, NULL, G_SPAWN_SEARCH_PATH|G_SPAWN_LEAVE_DESCRIPTORS_OPEN, NULL,
                          NULL, &stdout, &stderr, &ret, &error);
    if (error != NULL)
    {
        g_error ("Spawning child failed: %s", error->message);
        return 1;
    }

    if (stdout != NULL)
        printf("STDOUT:\n%s\n", stdout);
    if (stderr != NULL)
        printf("STDERR:\n%s\n", stderr);
    
    return ret;
}

/**
 * Run fsck using a libblockdev's function.
 */
int
fsck_blockdev(char *fs, char *fd_str)
{
    gint ret;
    g_autoptr(GError) error = NULL;
    BDPluginSpec fs_plugin = {BD_PLUGIN_FS, NULL};
    BDPluginSpec *plugins[] = {&fs_plugin, NULL};

    /* init */
    ret = bd_ensure_init (plugins, &logprint, &error);
    if (!ret) {
        g_print ("Error initializing libblockdev library: %s (%s, %d)\n",
             error->message, g_quark_to_string (error->domain), error->code);
        return 1;
    }

    /* create extra args to pass the file descriptor */
    BDExtraArg label_arg = {"-C", fd_str};
    const BDExtraArg *extra_args[2] = {&label_arg, NULL};

    /* run fsck */
    bd_fs_ext4_check (fs, extra_args, &error);
    return 0;
}

void
print_usage(char *name) {
    fprintf(stderr, "Usage: %s -b|-g device\n"
                    "  device   Path to ext4 device/image to fsck.\n"
                    "      -b   Use libblockdev method.\n"
                    "      -g   Use glib method.\n"
                    "The -b and -g flags can be used at the same time - then it will run both methods.\n"
                    "New files 'progress.b.out' and 'progress.g.out' will be created in $CURRENT_DIR.\n",
            name);
}

/**
 * create a file, open a descriptor to it and return the descriptor.
 */
int
create_fd(char *fname) {
    int fd;
    truncate_fd_file(fname);
    fd = open(fname, O_RDWR);
    printf("FD is %d\n", fd);
    return fd;
}

int
main(int argc, char **argv)
{
    int fd, opt, use_blk, use_glib;
    char fd_str[20];
    char *devicename;
    struct stat buf;

    devicename = NULL;
    use_blk = 0;
    use_glib = 0;

    /* argparse */
    while ((opt = getopt(argc, argv, "bg")) != -1) {
        switch (opt) {
        case 'b':
            use_blk = 1;
            break;
        case 'g':
            use_glib = 1;
            break;
        default: /* '?' */
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    /* test for mandatory device arg */
    if (optind >= argc) {
        fprintf(stderr, "Expected adevice/image path.\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    devicename = argv[optind];

    /* test for extra args */
    if (optind < argc-1) {
        fprintf(stderr, "Too many arguments.\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    /* at least one has to be selected */
    if (!use_blk && !use_glib) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    /* won't work without root */
    if (getuid () != 0) {
        g_print ("Requires to be run as root!\n");
        exit(EXIT_FAILURE);
    }

    /* run the fsck checks */
    if (use_blk) {
        printf("Running libblockdev fsck.\n");

        fd = create_fd("progress.b.fd");
        sprintf(fd_str, "%d", fd);
        fsck_blockdev(devicename, fd_str);

        fstat(fd, &buf);
        printf("Size of the file with progress info: %ld B\n", buf.st_size);
        printf("\n");
    }
    if (use_glib) {
        printf("Running glib fsck.\n");

        create_fd("progress.g.fd");
        sprintf(fd_str, "%d", fd);
        fsck_glib(devicename, fd_str);

        fstat(fd, &buf);
        printf("Size of the file with progress info: %ld B\n", buf.st_size);
        printf("\n");
    }

    return EXIT_SUCCESS;
}