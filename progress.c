#include <glib.h>
#include <blockdev/blockdev.h>
#include <blockdev/fs.h>
#include <blockdev/exec.h>
#include <blockdev/utils.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>

// shrink - max 11810111488 min 11010111488

/**
 * A log handler for libblockdev
 */
void
logprint (gint level, const gchar *msg)
{
    printf("LOG %d: %s\n", level, msg);
}

void prog_report(guint64 task_id, BDUtilsProgStatus status, guint8 completion, gchar *msg)
{
    static gint8 last_perc = -1;

    if (msg == NULL && completion != last_perc) {
        printf("Progress: %d%%\n", completion);
        //printf("\rProgress: %d%%", completion);
        fflush(stdout);
        last_perc = completion;
    }
    if (msg != NULL) {
        printf("\n%s\n", msg);
    }
}

/**
 * Run fsck using a libblockdev's function.
 */
int fsck_blockdev(char *fs, char *fd_str)
{
    gint ret;
    g_autoptr(GError) error = NULL;
    BDPluginSpec fs_plugin = {BD_PLUGIN_FS, NULL};
    BDPluginSpec *plugins[] = {&fs_plugin, NULL};

    /* init */
    //ret = bd_ensure_init (plugins, &logprint, &error);
    ret = bd_ensure_init (plugins, NULL, &error);
    if (!ret) {
        g_print ("Error initializing libblockdev library: %s (%s, %d)\n",
             error->message, g_quark_to_string (error->domain), error->code);
        return 1;
    }
    bd_utils_init_prog_reporting(prog_report, &error);

    /* create extra args to pass the file descriptor */
    BDExtraArg label_arg = {"-C", fd_str};
    const BDExtraArg *extra_args[2] = {&label_arg, NULL};

    /* run fsck */
    bd_fs_ext4_check (fs, extra_args, &error);
    //bd_fs_ext4_check_progress (fs, extra_args, &error, prog_extract);
    return 0;
}

void print_usage(char *name)
{
    fprintf(stderr, "Usage: %s device\n"
                    "  device   Path to ext4 device/image to fsck.\n",
            name);
}

int
main(int argc, char **argv)
{
    int opt;
    char *devicename;

    devicename = NULL;

    /* argparse */
    while ((opt = getopt(argc, argv, "")) != -1) {
        switch (opt) {
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

    /* won't work without root */
    if (getuid () != 0) {
        g_print ("Requires to be run as root!\n");
        exit(EXIT_FAILURE);
    }

    /* run the fsck checks */
    fsck_blockdev(devicename, "1");
    printf("\n");

    return EXIT_SUCCESS;
}