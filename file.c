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
#include <sys/xattr.h>
#include <unistd.h>

struct hub_file {
    int fd;
};

int hub_fgetattr(const char *path __attribute__((unused)), struct stat *stat,
                        struct fuse_file_info *info)
{
    struct hub_file *file = (struct hub_file*)(uintptr_t)info->fh;

    if (fstat(file->fd, stat) < 0) {
        return -errno;
    }
    return 0;
}

static int hub_open_impl(const char *path, int addflags,
            mode_t mode, struct fuse_file_info *info)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    int flags = 0, ret = 0, fd = -1;
    char bpath[PATH_MAX];
    struct hub_file *file = NULL;

    file = calloc(1, sizeof(*file));
    if (!file) {
        ret = -ENOMEM;
        goto error;
    }
    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    // note: we assume that FUSE has already taken care of umask.
    flags = addflags;
    flags |= info->flags;
    fd = open(bpath, flags, mode);
    if (fd < 0) {
        ret = -errno;
        goto error;
    }
    info->fh = (uintptr_t)file;
    return 0;

error:
    if (file) {
        if (file->fd >= 0) {
            close(file->fd);
        }
        free(file);
    }
    return ret;
}

int hub_create(const char *path, mode_t mode,
                      struct fuse_file_info *info)
{
    return hub_open_impl(path, O_CREAT, mode, info);
}

int hub_open(const char *path, struct fuse_file_info *info)
{
    return hub_open_impl(path, 0, 0, info);
}

int hub_read(const char *path __attribute__((unused)), char *buf,
             size_t size, off_t offset, struct fuse_file_info *info)
{
    int ret;
    struct hub_file *file = (struct hub_file*)(uintptr_t)info->fh;

    ret = pread(file->fd, buf, size, offset);
    if (ret < 0) {
        return -errno;
    }
    // We're using the direct_io mount option, so we return the number of bytes
    // read.
    return ret;
}

int hub_write(const char *path __attribute__((unused)), const char *buf,
              size_t size, off_t offset, struct fuse_file_info *info)
{
    int ret;
    struct hub_file *file = (struct hub_file*)(uintptr_t)info->fh;

    ret = pwrite(file->fd, buf, size, offset);
    if (ret < 0) {
        return -errno;
    }
    // We're using the direct_io mount option, so we return the number of bytes
    // written.
    return ret;
}

int hub_flush(const char *path __attribute__((unused)),
              struct fuse_file_info *info __attribute__((unused)))
{
    /*
     * FUSE calls flush() each time close() is called on a file descriptor
     * implemented by FUSE.  Since multiple file descriptors may point to the
     * same file __description__, this may be called multiple times on the same
     * fuse_file_info.
     *
     * We don't do any caching in hubfs, since the kernel already caches for
     * us.  So there is nothing to flush here, and hence nothing to do.
     */
    return 0;
}

int hub_release(const char *path __attribute__((unused)),
                struct fuse_file_info *info)
{
    struct hub_file *file = (struct hub_file*)(uintptr_t)info->fh;
    int ret = 0;

    /*
     * FUSE calls release() when there are no remaining file descriptors
     * referencing this file description (aka fuse_file_info).
     * At this point, we close the backing file.
     */
    if (close(file->fd) < 0) {
        // Portability: HP/UX has "issues" where close will sometimes fail with
        // EINTR.  But we can't just retry because that would cause problems on
        // Linux.
        ret = -errno;
    }
    free(file);
    return ret;
}

int hub_fsync(const char *path __attribute__((unused)),
              int datasync, struct fuse_file_info *info)
{
    struct hub_file *file = (struct hub_file*)(uintptr_t)info->fh;
    int ret = 0;

    if (datasync) {
        if (fdatasync(file->fd) < 0) {
            ret = -errno;
        }
    } else {
        if (fsync(file->fd) < 0) {
            ret = -errno;
        }
    }
    return ret;
}

int hub_ftruncate(const char *path __attribute__((unused)), off_t len,
                         struct fuse_file_info *info)
{
    struct hub_file *file = (struct hub_file*)(uintptr_t)info->fh;

    if (ftruncate(file->fd, len) < 0) {
        return -errno;
    }
    return 0;
}

int hub_fallocate(const char *path __attribute__((unused)), int mode,
                  off_t offset, off_t len, struct fuse_file_info *info)
{
    struct hub_file *file = (struct hub_file*)(uintptr_t)info->fh;

    if (fallocate(file->fd, mode, offset, len) < 0) {
        return -errno;
    }
    return 0;
}

// vim: ts=4:sw=4:tw=79:et
