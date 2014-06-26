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

#include "fs.h"
#include "meta.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h> 
#include <sys/xattr.h>
#include <unistd.h>

int hub_getattr(const char *path, struct stat *stbuf)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    int ret = 0;
    char bpath[PATH_MAX];

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (stat(bpath, stbuf) < 0) {
        return -errno;
    }
    return ret;
}

int hub_readlink(const char *path, char *buf, size_t size)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    ssize_t res;
    char bpath[PATH_MAX];

    // POSIX semantics are a bit different than FUSE semantics... POSIX doesn't
    // require NULL-termination, but FUSE does.  POSIX also returns the length
    // of the text we fetched, but FUSE does not.
    if (size <= 0) {
        return 0;
    }
    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    res = readlink(bpath, buf, size);
    if (res < 0) {
        return -errno;
    }
    buf[res] = '\0';
    return 0;
}

int hub_mknod(const char *path, mode_t mode, dev_t dev)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    // note: we assume that FUSE has already taken care of umask.
    if (mknod(bpath, mode, dev) < 0) {
        return -errno;
    }
    return 0;
}

int hub_mkdir(const char *path, mode_t mode)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    // note: we assume that FUSE has already taken care of umask.
    if (mkdir(bpath, mode) < 0) {
        return -errno;
    }
    return 0;
}

int hub_unlink(const char *path)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (unlink(bpath) < 0) {
        return -errno;
    }
    return 0;
}

int hub_rmdir(const char *path)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (rmdir(bpath) < 0) {
        return -errno;
    }
    return 0;
}

int hub_symlink(const char *oldpath, const char *newpath)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char boldpath[PATH_MAX], bnewpath[PATH_MAX];

    snprintf(boldpath, sizeof(boldpath), "%s%s", fs->root, oldpath);
    snprintf(bnewpath, sizeof(bnewpath), "%s%s", fs->root, newpath);
    if (symlink(boldpath, bnewpath) < 0) {
        return -errno;
    }
    return 0;
}

int hub_rename(const char *oldpath, const char *newpath) 
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char boldpath[PATH_MAX], bnewpath[PATH_MAX];

    snprintf(boldpath, sizeof(boldpath), "%s%s", fs->root, oldpath);
    snprintf(bnewpath, sizeof(bnewpath), "%s%s", fs->root, newpath);
    if (rename(boldpath, bnewpath) < 0) {
        return -errno;
    }
    return 0;
}

int hub_link(const char *oldpath, const char *newpath) 
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char boldpath[PATH_MAX], bnewpath[PATH_MAX];

    snprintf(boldpath, sizeof(boldpath), "%s%s", fs->root, oldpath);
    snprintf(bnewpath, sizeof(bnewpath), "%s%s", fs->root, newpath);
    if (link(boldpath, bnewpath) < 0) {
        return -errno;
    }
    return 0;
}

int hub_chmod(const char *path, mode_t mode) 
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (chmod(bpath, mode) < 0) {
        return -errno;
    }
    return 0;
}

int hub_chown(const char *path, uid_t uid, gid_t gid)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (chown(bpath, uid, gid) < 0) {
        return -errno;
    }
    return 0;
}

int hub_truncate(const char *path, off_t off)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (truncate(bpath, off) < 0) {
        return -errno;
    }
    return 0;
}

int hub_utime(const char *path, struct utimbuf *buf)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (utime(bpath, buf) < 0) {
        return -errno;
    }
    return 0;
}

int hub_statfs(const char *path, struct statvfs *vfs)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (statvfs(bpath, vfs) < 0) {
        return -errno;
    }
    return 0;
}

int hub_setxattr(const char *path, const char *name, const char *value,
                        size_t size, int flags)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (setxattr(bpath, name, value, size, flags) < 0) {
        return -errno;
    }
    return 0;
}

int hub_getxattr(const char *path, const char *name, char *value, size_t size)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (getxattr(bpath, name, value, size) < 0) {
        return -errno;
    }
    return 0;
}

int hub_listxattr(const char *path, char *list, size_t size)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (listxattr(bpath, list, size) < 0) {
        return -errno;
    }
    return 0;
}

int hub_removexattr(const char *path, const char *name)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (removexattr(bpath, name) < 0) {
        return -errno;
    }
    return 0;
}

int hub_opendir(const char *path, struct fuse_file_info *info)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    DIR *dp;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    dp = opendir(bpath);
    if (!dp) {
        return -errno;
    }
    info->fh = (uintptr_t)dp;
    return 0;
}

int hub_readdir(const char *path __attribute__((unused)), void *buf,
                fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *info)
{
    DIR *dp = (DIR*)(uintptr_t)info->fh;
    struct dirent *de;

    errno = 0;
    if (offset != 0) {
        seekdir(dp, (long)offset);
    }
    while (1) {
        de = readdir(dp); // portability: thread-safe on Linux, maybe not elsewhere
        if (!de) {
            if (errno) {
                return -errno;
            }
            return 0;
        }
        if ((de->d_name[0] == '.') && ((de->d_name[1] == '\0') ||
             ((de->d_name[1] == '.') && (de->d_name[2] == '\0')))) {
            continue;
        }
        // We're using the directory filler API here.  This avoids the need to
        // fetch all the directory entries at once.
        // TODO: can we avoid calling telldir for every entry in the directory?
        offset = telldir(dp);
        if (filler(buf, de->d_name, NULL, offset)) {
            break;
        }
    }
    return 0;
}

int hub_releasedir(const char *path __attribute__((unused)),
                   struct fuse_file_info *info)
{
    DIR *dp = (DIR*)(uintptr_t)info->fh;

    if (closedir(dp) < 0) {
        return -errno;
    }
    return 0;
}

int hub_fsyncdir(const char *path __attribute__((unused)),
                 int datasync, struct fuse_file_info *info)
{
    int ret = 0;
    DIR *dp = (DIR *)info->fh;

    if (datasync) {
        if (fdatasync(dirfd(dp) < 0)) {
            ret = -errno;
        }
    } else {
        if (fsync(dirfd(dp)) < 0) {
            ret = -errno;
        }
    }
    return ret;
}

int hub_utimens(const char *path, const struct timespec tv[2])
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (utimensat(AT_FDCWD, bpath, tv, 0) < 0) {
        return -errno;
    }
    return 0;
}

// vim: ts=4:sw=4:tw=79:et
