/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "file.h"
#include "fs.h"
#include "meta.h"
#include "throttle.h"
#include "util.h"

#include <ctype.h>
#include <errno.h>
#include <fuse.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <unistd.h>

static struct fuse_operations hub_oper;

/**
 * Some FUSE options which we always set.
 *
 * default_permissions
 *      This option tells FUSE to do permission checking for us based on the
 *      permissions we report for inodes.
 *
 * allow_other
 *      Allow all users to access the mount.
 *
 * direct_io
 *      Don't cache data in FUSE.  Because the data will be cached by the page
 *      cache anyway, we want this to avoid double-caching.
 *
 * hard_remove
 *      Normally, when an open file is unlinked, FUSE translates that into a
 *      rename to a file named something like .fuse_hiddenXXX.  We don't need
 *      or want this, since POSIX handles reads and writes to unlinked files
 *      for us anyway.
 */
static const char const *MANDATORY_OPTIONS[] = {
    "-odefault_permissions",
    "-oallow_other",
    "-odirect_io",
    "-ohard_remove",
};

#define NUM_MANDATORY_OPTIONS \
    (int)(sizeof(MANDATORY_OPTIONS)/sizeof(MANDATORY_OPTIONS[0]))

static void *hub_init(struct fuse_conn_info *conn)
{
    conn->want = FUSE_CAP_ASYNC_READ |
        FUSE_CAP_ATOMIC_O_TRUNC	|
        FUSE_CAP_BIG_WRITES	|
        FUSE_CAP_SPLICE_WRITE |
        FUSE_CAP_SPLICE_MOVE |
        FUSE_CAP_SPLICE_READ;
    return fuse_get_context()->private_data;
}

static void hub_destroy(void *userdata __attribute__((unused)))
{
}

static void hub_usage(const char *argv0)
{
    fprintf(stderr, "\
usage:  %s [FUSE and mount options] <root> <mount_point>\n", argv0);
}

/**
 * Set up the fuse arguments we want to pass.
 *
 * We start with something like this:
 *     ./myapp [fuse-options] <root> <mount-point>
 *
 * and end up with this:
 *     ./myapp [mandatory-fuse-options] [fuse-options] <mount-point>
 */
static int setup_hub_args(int argc, char **argv, struct fuse_args *args,
                          char **mount_point)
{
    int i, ret = 0;

    *mount_point = NULL;
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-')) {
        hub_usage(argv[0]);
        ret = EINVAL;
        goto error;
    }
    *mount_point = strdup(strdup(argv[argc - 1]));
    if (!*mount_point) {
        fprintf(stderr, "setup_hub_args: OOM\n");
        ret = ENOMEM;
        goto error;
    }
    args->argc = NUM_MANDATORY_OPTIONS + argc - 2;
    args->argv = calloc(sizeof(char*), args->argc + 1);
    if (!args->argv) {
        fprintf(stderr, "setup_hub_args: OOM\n");
        ret = ENOMEM;
        goto error;
    }
    args->argv[0] = argv[0];
    for (i = 0; i < NUM_MANDATORY_OPTIONS; i++) {
        args->argv[i + 1] = (char*)MANDATORY_OPTIONS[i];
    }
    for (i = 1; i < argc - 2; i++) {
        args->argv[NUM_MANDATORY_OPTIONS - 1 + i] = argv[i];
    }
    args->argv[args->argc - 1] = argv[argc - 2];
    args->argv[args->argc] = NULL;
    fprintf(stderr, "running fuse_main with: ");
    for (i = 0; i < args->argc; i++) {
        fprintf(stderr, "%s ", args->argv[i]);
    }
    fprintf(stderr, "\n ");
    return 0;

error:
    if (*mount_point) {
        free(*mount_point);
        *mount_point = NULL;
    }
    if (args->argv) {
        free(args->argv);
        args->argv = NULL;
    }
    args->argc = 0;
    return ret;
}

static const struct uid_config woot = {
    .next = NULL,
    .uid = 1015,
    .full = 5242880LL,
};

static const struct uid_config cmccabe = {
    .next = &woot,
    .uid = 1014,
    .full = 262144000LL,
};

static const struct uid_config uid_config_list = {
    .next = &cmccabe,
    .uid = UNKNOWN_UID,
    .full = 5242880LL,
};

int main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    struct hub_fs *fs = NULL;
    struct fuse_args args;

    memset(&args, 0, sizeof(args));

    throttle_init(&uid_config_list);

    if (chdir("/") < 0) {
        perror("hub_main: failed to change directory to /");
        goto done;
    }

    fs = calloc(1, sizeof(*fs));
    if (!fs) {
        fprintf(stderr, "hub_main: OOM\n");
        goto done;
    }

    /*
     * We set our process umask to 0 so that we can create inodes with any
     * permissions we want.  However, we need to be careful to honor the umask
     * of the process we're performing our requests on behalf of.
     *
     * Note: The umask(2) call cannot fail.
     */
    umask(0);

    /* Ignore SIGPIPE because it is annoying. */
    if (signal(SIGPIPE, SIG_DFL) == SIG_ERR) {
        fprintf(stderr, "hub_main: failed to set the disposition of EPIPE "
                "to SIG_DFL (ignored.)\n"); 
        goto done;
    }

    /* Set up mandatory arguments. */
    if (setup_hub_args(argc, argv, &args, &fs->root)) {
        goto done;
    }

    if (access(fs->root, R_OK) < 0) {
        fprintf(stderr, "Bad root argument %s ", fs->root);
        perror("");
        goto done;
    }

    /* Run main FUSE loop. */
    ret = fuse_main(args.argc, args.argv, &hub_oper, fs);

done:
    if (fs) {
        free(fs->root);
        free(fs);
    }
    if (args.argv) {
        free(args.argv);
    }
    fprintf(stderr, "hub_main exiting with error code %d\n", ret);
    return ret;
}

static struct fuse_operations hub_oper = {
    .getattr = hub_getattr,
    .readlink = hub_readlink,
    .getdir = NULL, // deprecated
    .mknod = hub_mknod,
    .mkdir = hub_mkdir,
    .unlink = hub_unlink,
    .rmdir = hub_rmdir,
    .symlink = hub_symlink,
    .rename = hub_rename,
    .link = hub_link,
    .chmod = hub_chmod,
    .chown = hub_chown,
    .truncate = hub_truncate,
    .utime = hub_utime,
    .open = hub_open,
    .read = hub_read,
    .write = hub_write,
    .statfs = hub_statfs,
    .flush = hub_flush,
    .release = hub_release,
    .fsync = hub_fsync,
    .setxattr = hub_setxattr,
    .getxattr = hub_getxattr,
    .listxattr = hub_listxattr,
    .removexattr = hub_removexattr,
    .opendir = hub_opendir,
    .readdir = hub_readdir,
    .releasedir = hub_releasedir,
    .fsyncdir = hub_fsyncdir,
    .init = hub_init,
    .destroy = hub_destroy,
    .access = NULL, // never called because we use 'default_permissions'
    .create = hub_create,
    .ftruncate = hub_ftruncate,
    .fgetattr = hub_fgetattr,
    .lock = NULL, // delegate to kernel
    .flock = NULL, // delegate to kernel
	.utimens = hub_utimens,
    .bmap = NULL, // We are not a block-device-backed filesystem
	.fallocate = hub_fallocate,

    // Allow various operations to continue to work on unlinked files.
    .flag_nullpath_ok = 1,

    // Don't bother with paths for operations where we're using fds.
    .flag_nopath = 1,

    // Allow UTIME_OMIT and UTIME_NOW to be used with hub_utimens.
    .flag_utime_omit_ok = 1,
};

// vim: ts=4:sw=4:tw=79:et
