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

#define REGEX "^\\([0-9][0-9]*\\) \\([0-9][0-9]*\\) \\([0-9][0-9]*\\) \\(/.*\\)"

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
    printf("REPORT %lu: %d%% (%s)\n", task_id, completion, msg);
}

gint8 compute_percents(int pass_cur, int pass_total, int val_cur, int val_total)
{
    int perc;
    int one_pass;
    /* first get a percentage in the current pass/stage */
    perc = (val_cur * 100) / val_total;

    /* now map it to the total progress, splitting the stages equally */
    one_pass = 100 / pass_total;
    perc = ((pass_cur - 1) * one_pass) + (perc / pass_total);

    return perc;
}

/**
 * Filter one line - decide what to do with it.
 * return the percentage, -1 if not a percentage, -2 on an error
 */
gint8 filter_line(const char * line)
{
    static regex_t regex;
    static int reginit = 0;
    int reti;
    regmatch_t match[5];
    gint8 perc = -1;
    char regerr[500];

    if (!reginit) {
        /* Compile regular expression that matches to e2fsck progress output */
        reti = regcomp(&regex, "^\\([0-9][0-9]*\\) \\([0-9][0-9]*\\) \\([0-9][0-9]*\\) \\(/.*\\)", 0);
        if (reti) {
            fprintf(stderr, "Could not compile regex '%s'\n", REGEX);
            return -2;
        }
        reginit = 1;
    }
    
    /* Execute regular expression */
    reti = regexec(&regex, line, 4, match, 0);
    if (!reti) {
        char pass[50];
        char val_cur[50];
        char val_total[50];
        long pass_l;
        long val_cur_l;
        long val_total_l;

        strncpy(pass, &line[match[1].rm_so], match[1].rm_eo - match[1].rm_so);
        strncpy(val_cur, &line[match[2].rm_so], match[2].rm_eo - match[2].rm_so);
        strncpy(val_total, &line[match[3].rm_so], match[3].rm_eo - match[3].rm_so);

        /* The regex ensures we have a number in these matches, so we can skip
         * tests for conversion errors.
         */
        pass_l = strtol(pass, (char **)NULL, 10);
        val_cur_l = strtol(val_cur, (char **)NULL, 10);
        val_total_l = strtol(val_total, (char **)NULL, 10);

        perc = compute_percents(pass_l, 5, val_cur_l, val_total_l);
    } else if (reti == REG_NOMATCH){
        perc = -1;
    } else {
        regerror(reti, &regex, regerr, sizeof(regerr));
        fprintf(stderr, "Regex match failed: %s\n", line);
        return -2;
    }
    return perc;
}

/**
 * Filter one line - decide what to do with it.
 * return the percentage, -1 if not a percentage
 */
gint8 filter_line2(const char * line)
{
    /* 
     * a static variable to know if we were printing percents on the last run
     * or not. If yes, but we are not doing it this run, print a new line.
     */
    static int printing_percents = 0;
    static gint8 last_perc = -1;
    gint8 perc;

    /* Execute regular expression */
    perc = filter_line(line);
    if (perc != -1) {
        if (perc != last_perc) {
            printing_percents = 1;
            printf("\rProgress [%d%%]    ", perc);
            fflush(stdout);
            last_perc = perc;
        }
    } else {
        if (printing_percents) {
            putc('\n', stdout);
            printing_percents = 0;
        }
        printf("%s", line);
    }
    return perc;
}

gboolean prog_extract(const gchar *line, guint8 *completion)
{
    gint8 perc;

    perc = filter_line2(line);
    if (perc == -1)
        return 0;

    //printf("XXXX %d%% XXXXX: %s", perc, line);
    *completion = perc;
    return !!perc;
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
    //bd_utils_init_prog_reporting(prog_report, &error);

    /* create extra args to pass the file descriptor */
    BDExtraArg label_arg = {"-C", fd_str};
    const BDExtraArg *extra_args[2] = {&label_arg, NULL};

    /* run fsck */
    //bd_fs_ext4_check (fs, extra_args, &error);
    bd_fs_ext4_check_progress (fs, extra_args, &error, prog_extract);
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