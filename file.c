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
#include "log.h"
#include "throttle.h"
#include "util.h"

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

int hub_fgetattr(const char *path, struct stat *stat,
                        struct fuse_file_info *info)
{
    struct hub_file *file = (struct hub_file*)(uintptr_t)info->fh;

    if (fstat(file->fd, stat) < 0) {
        int err = errno;
        DEBUG("hub_fgetattr(path=%s, fd=%d) = %d (%s)\n",
              path, file->fd, err, terror(err));
        return -errno;
    }
    DEBUG("hub_fgetattr(path=%s, fd=%d) = 0\n", path, file->fd);
    return 0;
}

static int hub_open_impl(const char *path, int addflags,
            mode_t mode, struct fuse_file_info *info)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    int flags = 0, ret = 0;
    char bpath[PATH_MAX] = { 0 };
    struct hub_file *file = NULL;

    file = calloc(1, sizeof(struct hub_file));
    if (!file) {
        ret = -ENOMEM;
        goto error;
    }
    file->fd = -1;
    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    // note: we assume that FUSE has already taken care of umask.
    flags = addflags;
    flags |= info->flags;
    if ((flags & O_ACCMODE) == 0)  {
        flags |= O_RDONLY;
    }
    file->fd = open(bpath, flags, mode);
    if (file->fd < 0) {
        ret = -errno;
        goto error;
    }
    info->fh = (uintptr_t)(void*)file;

error:
#ifdef DEBUG_ENABLED
    {
        char addflags_str[128] = { 0 };
        char flags_str[128] = { 0 };
        open_flags_to_str(addflags, addflags_str, sizeof(addflags_str));
        open_flags_to_str(info->flags, flags_str, sizeof(addflags_str));
        DEBUG("hub_open_impl(path=%s, bpath=%s, addflags=%s, info->flags=%s, "
              "mode=%04o) = %d\n", path, bpath, addflags_str, flags_str,
              mode, ret);
    }
#endif
    if (ret == 0) {
        return 0;
    }
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
    DEBUG("hub_create(path=%s, mode=%04o): begin...\n", path, mode);
    return hub_open_impl(path, O_CREAT, mode, info);
}

int hub_open(const char *path, struct fuse_file_info *info)
{
    DEBUG("hub_open(path=%s): begin...\n", path);
    return hub_open_impl(path, 0, 0, info);
}

int hub_read(const char *path, char *buf, size_t size,
             off_t offset, struct fuse_file_info *info)
{
    int ret;
    uint32_t uid;
    struct hub_file *file = (struct hub_file*)(uintptr_t)info->fh;

    uid = fuse_get_context()->uid;
    DEBUG("hub_read(path=%s, size=%zd, offset=%" PRId64", uid=%"PRId32"): "
          "begin\n", path, size, (int64_t)offset, uid);
    throttle(uid, size);
    ret = pread(file->fd, buf, size, offset);
    // We're using the direct_io mount option, so we return the number of bytes
    // read (unless there is an error, in which case we return the negative
    // error code.) 
    if (ret < 0) {
        ret = -errno;
    }
    DEBUG("hub_read(path=%s, size=%zd, offset=%" PRId64", uid=%"PRId32") "
          "= %d\n", path, size, (int64_t)offset, uid, ret);
    return ret;
}

int hub_write(const char *path, const char *buf, size_t size,
              off_t offset, struct fuse_file_info *info)
{
    int ret;
    uint32_t uid;
    struct hub_file *file = (struct hub_file*)(uintptr_t)info->fh;

    uid = fuse_get_context()->uid;
    DEBUG("hub_write(path=%s, size=%zd, offset=%" PRId64", uid=%"PRId32"): "
          "throttling...\n", path, size, (int64_t)offset, uid);
    //fprintf(stderr, "size = %zd\n", size);
    throttle(fuse_get_context()->uid, size);
    ret = pwrite(file->fd, buf, size, offset);
    // We're using the direct_io mount option, so we return the number of bytes
    // written (unless there is an error, in which case we return the negative
    // error code.)
    if (ret < 0) {
        ret = -errno;
    }
    DEBUG("hub_write(path=%s, size=%zd, offset=%" PRId64", uid=%"PRId32") "
          "=  %d\n", path, size, (int64_t)offset, uid, ret);
    return ret;
}

int hub_flush(const char *path,
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
    DEBUG("hub_flush(path=%s) = 0\n", path);
    return 0;
}

int hub_release(const char *path, struct fuse_file_info *info)
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
    file->fd = -1;
    DEBUG("hub_release(path=%s, file->fd=%d) = %d\n", path, file->fd, ret);
    free(file);
    return ret;
}

int hub_fsync(const char *path, int datasync, struct fuse_file_info *info)
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
    DEBUG("hub_fsync(path=%s, file->fd=%d, datasync=%d) = %d\n",
          path, file->fd, datasync, ret);
    return ret;
}

int hub_ftruncate(const char *path, off_t len,
                         struct fuse_file_info *info)
{
    struct hub_file *file = (struct hub_file*)(uintptr_t)info->fh;
    int ret = 0;

    if (ftruncate(file->fd, len) < 0) {
        ret = -errno;
    }
    DEBUG("hub_ftruncate(path=%s, len=%"PRId64", file->fd=%d) = %d\n",
          path, (int64_t)len, file->fd, ret);
    return ret;
}

int hub_fallocate(const char *path, int mode,
                  off_t offset, off_t len, struct fuse_file_info *info)
{
    struct hub_file *file = (struct hub_file*)(uintptr_t)info->fh;
    int ret = 0;

    if (fallocate(file->fd, mode, offset, len) < 0) {
        ret = -errno;
    }
    DEBUG("hub_fallocate(path=%s, mode=%04o, offset=%"PRId64
          "len=%"PRId64", file->fd=%d) = %d\n", path, mode,
          (int64_t)offset, (int64_t)len, file->fd, ret);
    return ret;
}

// vim: ts=4:sw=4:tw=79:et
