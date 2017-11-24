#include <glib.h>
#include <blockdev/blockdev.h>
#include <blockdev/fs.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define USE_BLK


#define FD_FILE "progress.fd"

void 
truncate_fd_file()
{
    FILE *f;
    f = fopen(FD_FILE, "w");
    fclose(f);
}

void
logprint (gint level, const gchar *msg)
{
    /* create/truncate the file with progress */
    printf("LOG %d: %s\n", level, msg);
}


int
main(int argc, char **argv)
{

    char *filename;
    int fd;
    int fd2;
    char fd_str[20];
    FILE *f;
    g_autoptr(GError) error = NULL;

    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    /* argparse */
    if (argc == 1 || strcmp("-h", argv[1]) == 0) {
        printf("Usage: %s FS\n", argv[0]);
        printf("FS is the ext4 filesystem to check.\n");
        printf("A new file 'progress.fd' will be created in $CURRENT_DIR.\n");
        return 0;
    }

    if (getuid () != 0) {
        g_print ("Requires to be run as root!\n");
        return 1;
    }

    /* initialize the output file and desriptor */

    filename = argv[1];
    truncate_fd_file();

    fd = open("progress.fd", O_RDWR);
    printf("FD is %d\n", fd);
    /* try to verify if it works, the same way libblockdev does it */
    fd2 = dup(fd);
    printf("FD2 is %d\n", fd2);
    /* get a FILE for other operations */
    f = fdopen(fd, "r+");
    sprintf(fd_str, "%d", fd);

#ifdef USE_BLK
    int ret;
    BDPluginSpec fs_plugin = {BD_PLUGIN_FS, NULL};
    BDPluginSpec *plugins[] = {&fs_plugin, NULL};
    /* initialize the library (if it isn't already initialized) and load
     * all required modules
     */
    ret = bd_ensure_init (plugins, &logprint, &error);
    if (!ret) {
        g_print ("Error initializing libblockdev library: %s (%s, %d)\n",
             error->message, g_quark_to_string (error->domain), error->code);
        return 1;
    }

    BDExtraArg label_arg = {"-C", fd_str};
    BDExtraArg label_arg2 = {"", NULL};
    const BDExtraArg *extra_args[3] = {&label_arg, &label_arg2, NULL};

    bd_fs_ext4_check (filename, extra_args, &error);

#else

    char *cmd;
    const char *cmd_template = "e2fsck -f -n %s -C %d &> /dev/null & \0";
    /* 
     * Prepare the space for the command - take the two known strings we are
     * joining and add some more space for the fd number.
     */
    cmd = malloc(sizeof(filename) + sizeof(cmd_template) + 20*sizeof(char));
    sprintf(cmd, cmd_template, filename, fd);
    printf("running command: %s\n", cmd);
    system(cmd);
    printf("In progress...\n");
#endif

    fpos_t last;
    fpos_t prelast;
    for (int i=0; i<33; i++) {
        int wrote = 0;
        usleep(300*1000);
        fgetpos(f, &last);
        fgetpos(f, &prelast);
        while ((read = getline(&line, &len, f)) != -1) {
            last = prelast;
            fgetpos(f, &prelast);
            wrote = 1;
            printf("# %s", line);
        }
        fsetpos(f, &last);
        if (!wrote)
            printf(".\n");
    }
    printf("\n");

    return 0;
}