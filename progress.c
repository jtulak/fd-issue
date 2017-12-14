#include <glib.h>
#include <blockdev/blockdev.h>
#include <blockdev/fs.h>
#include <blockdev/exec.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/wait.h>
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
gint8 get_progress(int pass_cur, int pass_total, int val_cur, int val_total)
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
 * return the percentage
 */
gint8 filter_line_return(regex_t * regex, char * line)
{
    /* 
     * a static variable to know if we were printing percents on the last run
     * or not. If yes, but we are not doing it this run, print a new line.
     */
    static int printing_percents = 0;

    int reti;
    regmatch_t match[5];

    if (strcmp(line, "### KILL CHILD ###\n") == 0) {
        return 0;
    }

    /* Execute regular expression */
    reti = regexec(regex, line, 4, match, 0);
    if (!reti) {
        printing_percents = 1;
        char pass[50];
        char val_cur[50];
        char val_total[50];
        strncpy(pass, &line[match[1].rm_so], match[1].rm_eo - match[1].rm_so);
        strncpy(val_cur, &line[match[2].rm_so], match[2].rm_eo - match[2].rm_so);
        strncpy(val_total, &line[match[3].rm_so], match[3].rm_eo - match[3].rm_so);
        // match
        print_progress(atoi(pass), 5, atoi(val_cur), atoi(val_total));
    } else if (reti == REG_NOMATCH) {
        if (printing_percents) {
            putc('\n', stdout);
            printing_percents = 0;
        }
        printf("%s", line);
    }
    else {
        regerror(reti, regex, line, sizeof(line));
        fprintf(stderr, "Regex match failed: %s\n", line);
        exit(EXIT_FAILURE);
    }
    return 1;
}

void progress_callback (guint64 task_id, BDUtilsProgStatus status, guint8 completion, gchar *msg)
{
    printf("task %lu: [%d] %s\n", task_id, completion, msg);
}

gboolean prog_extract(const gchar *line, guint8 *completion)
{
    static regex_t regex;
    static int reginit = 0;
    int ret;

    if (!reginit) {
        /* Compile regular expression that matches to e2fsck progress output */
        ret = regcomp(&regex, REGEX, 0);
        if (ret) {
            fprintf(stderr, "Could not compile regex '%s'\n", REGEX);
            exit(EXIT_FAILURE);
        }
        reginit = 1;
    }

    printf("XXXXXXXXX: %s", line);
    *completion = 13;
    return 1;
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
    ret = bd_ensure_init (plugins, &logprint, &error);
    if (!ret) {
        g_print ("Error initializing libblockdev library: %s (%s, %d)\n",
             error->message, g_quark_to_string (error->domain), error->code);
        return 1;
    }
    //bd_utils_init_prog_reporting(progress_callback, &error);

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

void print_progress(int pass_cur, int pass_total, int val_cur, int val_total)
{
    int perc;
    perc = (val_cur * 100) / val_total;
    printf("\rPass %d/%d [%d%%]", pass_cur, pass_total, perc);
    fflush(stdout);
}

/**
 * Filter one line - decide what to do with it.
 * If the line is a kill message, then return 0, otherwise 1.
 */
int filter_line(regex_t * regex, char * line)
{
    /* 
     * a static variable to know if we were printing percents on the last run
     * or not. If yes, but we are not doing it this run, print a new line.
     */
    static int printing_percents = 0;

    int reti;
    regmatch_t match[5];

    if (strcmp(line, "### KILL CHILD ###\n") == 0) {
        return 0;
    }

    /* Execute regular expression */
    reti = regexec(regex, line, 4, match, 0);
    if (!reti) {
        printing_percents = 1;
        char pass[50];
        char val_cur[50];
        char val_total[50];
        strncpy(pass, &line[match[1].rm_so], match[1].rm_eo - match[1].rm_so);
        strncpy(val_cur, &line[match[2].rm_so], match[2].rm_eo - match[2].rm_so);
        strncpy(val_total, &line[match[3].rm_so], match[3].rm_eo - match[3].rm_so);
        // match
        print_progress(atoi(pass), 5, atoi(val_cur), atoi(val_total));
    } else if (reti == REG_NOMATCH) {
        if (printing_percents) {
            putc('\n', stdout);
            printing_percents = 0;
        }
        printf("%s", line);
    }
    else {
        regerror(reti, regex, line, sizeof(line));
        fprintf(stderr, "Regex match failed: %s\n", line);
        exit(EXIT_FAILURE);
    }
    return 1;
}

void filter_stdout(const int shmid, const int pipe_fds[])
{
    int *count = shmat(shmid, 0, 0);
    char c;
    int i = 0;
    char line[1000];
    regex_t regex;
    int reti;

    *count = 0;
    dup2(pipe_fds[0], 0);      // redirect pipe to child's stdin
    setvbuf(stdout, 0, _IONBF, 0);

    /* Compile regular expression that matches to e2fsck progress output */
    reti = regcomp(&regex, REGEX, 0);
    if (reti) {
        fprintf(stderr, "Could not compile regex\n");
        exit(EXIT_FAILURE);
    }


    while (read(0, &c, 1) == 1 ) {
        (*count)++;
        if (i > 1000) {
            fprintf(stderr, "Error, too long line\n");
        } else {
            line[i++] = c;
        }

        // end of line/input, so print the line out
        if (c == '\n' || c == 0) {
            line[i] = 0;
            if (!filter_line(&regex, line))
                break;
            i = 0;
        }
    }
    regfree(&regex);
}

/**
 * A function similar to unix "tee" command.
 * Forks the process and redirects everything through a filter.
 * 
 * 
 * Returns the value of fork().
 * the number of captured chars.
 */
int create_filter(int **captured)
{
    int pipe_fds[2];
    int pid;

    int shmid = shmget(IPC_PRIVATE, sizeof(size_t), 0660 | IPC_CREAT);
    pipe(pipe_fds);

    switch ((pid = fork())) {
    case -1:                      // = error
        perror("fork");
        exit(EXIT_FAILURE);
    case 0:                      // = child
        filter_stdout(shmid, pipe_fds);
    default:                      // = parent
        dup2(pipe_fds[1], 1);      // replace stdout with output to child
        setvbuf(stdout, 0, _IONBF, 0);
        *captured = shmat(shmid, 0, 0); 
    }
    return pid;
}

int
main(int argc, char **argv)
{
    int opt;
    char *devicename;
    int saved_stdout;

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

    saved_stdout = dup(1);
    int *captured;
    int pid = create_filter(&captured);
    if (pid) {
        /* run the fsck checks */
        fsck_blockdev(devicename, "1");
        printf("\n");
        printf("### KILL CHILD ###\n");
        dup2(saved_stdout, 1);
        wait(NULL);
        printf("intercepted: %d\n", *captured);
    }

    return EXIT_SUCCESS;
}