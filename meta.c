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
#include "log.h"
#include "meta.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
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

// TODO: remove PATH_MAX hacks

int hub_getattr(const char *path, struct stat *stbuf)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    int ret = 0;
    char bpath[PATH_MAX];

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (stat(bpath, stbuf) < 0) {
        ret = -errno;
    }
    DEBUG("hub_getattr(path=%s, bpath=%s) = %d (%s)\n",
          path, bpath, ret, terror(-ret));
    return ret;
}

int hub_readlink(const char *path, char *buf, size_t size)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    ssize_t res;
    char bpath[PATH_MAX];
    int ret = 0;

    // POSIX semantics are a bit different than FUSE semantics... POSIX doesn't
    // require NULL-termination, but FUSE does.  POSIX also returns the length
    // of the text we fetched, but FUSE does not.
    if (size <= 0) {
        ret = 0;
        goto done;
    }
    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    res = readlink(bpath, buf, size);
    if (res < 0) {
        ret = -errno;
        goto done;
    }
    buf[res] = '\0';
    ret = 0;

done:
    DEBUG("hub_readlink(path=%s, bpath=%s) = %d (%s)\n",
          path, bpath, ret, terror(-ret));
    return ret;
}

int hub_mknod(const char *path, mode_t mode, dev_t dev)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    // note: we assume that FUSE has already taken care of umask.
    if (mknod(bpath, mode, dev) < 0) {
        ret = -errno;
    }
    DEBUG("hub_mknod(path=%s, bpath=%s, mode=%04o, dev=%"PRId64") = %d\n",
          path, bpath, mode, (int64_t)dev, ret);
    return ret;
}

int hub_mkdir(const char *path, mode_t mode)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    // note: we assume that FUSE has already taken care of umask.
    if (mkdir(bpath, mode) < 0) {
        ret = -errno;
    }
    DEBUG("hub_mkdir(path=%s, bpath=%s, mode=%04o) = %d\n",
          path, bpath, mode, ret);
    return ret;
}

int hub_unlink(const char *path)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (unlink(bpath) < 0) {
        ret = -errno;
    }
    DEBUG("hub_unlink(path=%s, bpath=%s) = %d (%s)\n",
          path, bpath, ret, terror(-ret));
    return ret;
}

int hub_rmdir(const char *path)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (rmdir(bpath) < 0) {
        ret = -errno;
    }
    DEBUG("hub_rmdir(path=%s, bpath=%s) = %d (%s)\n",
          path, bpath, ret, terror(-ret));
    return ret;
}

int hub_symlink(const char *oldpath, const char *newpath)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char boldpath[PATH_MAX], bnewpath[PATH_MAX];
    int ret = 0;

    snprintf(boldpath, sizeof(boldpath), "%s%s", fs->root, oldpath);
    snprintf(bnewpath, sizeof(bnewpath), "%s%s", fs->root, newpath);
    if (symlink(boldpath, bnewpath) < 0) {
        ret = -errno;
    }
    DEBUG("hub_symlink(oldpath=%s, boldpath=%s, newpath=%s, bnewpath=%s) = "
          "%d (%s)\n", oldpath, boldpath, newpath, bnewpath, ret,
          terror(-ret));
    return ret;
}

int hub_rename(const char *oldpath, const char *newpath) 
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char boldpath[PATH_MAX], bnewpath[PATH_MAX];
    int ret = 0;

    snprintf(boldpath, sizeof(boldpath), "%s%s", fs->root, oldpath);
    snprintf(bnewpath, sizeof(bnewpath), "%s%s", fs->root, newpath);
    if (rename(boldpath, bnewpath) < 0) {
        ret = -errno;
    }
    DEBUG("hub_rename(oldpath=%s, boldpath=%s, newpath=%s, bnewpath=%s) = "
          "%d (%s)\n", oldpath, boldpath, newpath, bnewpath, ret,
          terror(-ret));
    return 0;
}

int hub_link(const char *oldpath, const char *newpath) 
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char boldpath[PATH_MAX], bnewpath[PATH_MAX];
    int ret = 0;

    snprintf(boldpath, sizeof(boldpath), "%s%s", fs->root, oldpath);
    snprintf(bnewpath, sizeof(bnewpath), "%s%s", fs->root, newpath);
    if (link(boldpath, bnewpath) < 0) {
        ret = -errno;
    }
    DEBUG("hub_link(oldpath=%s, boldpath=%s, newpath=%s, bnewpath=%s) = "
          "%d (%s)\n", oldpath, boldpath, newpath, bnewpath, ret,
          terror(-ret));
    return ret;
}

int hub_chmod(const char *path, mode_t mode) 
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (chmod(bpath, mode) < 0) {
        ret = -errno;
    }
    DEBUG("hub_chmod(path=%s, bpath=%s) = %d (%s)\n",
          path, bpath, ret, terror(-ret));
    return ret;
}

int hub_chown(const char *path, uid_t uid, gid_t gid)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (chown(bpath, uid, gid) < 0) {
        ret = -errno;
    }
    DEBUG("hub_chown(path=%s, bpath=%s) = %d (%s)\n",
          path, bpath, ret, terror(-ret));
    return ret;
}

int hub_truncate(const char *path, off_t off)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (truncate(bpath, off) < 0) {
        ret = -errno;
    }
    DEBUG("hub_truncate(path=%s, bpath=%s, off=%"PRId64") = %d (%s)\n",
          path, bpath, (int64_t)off, ret, terror(-ret));
    return 0;
}

int hub_utime(const char *path, struct utimbuf *buf)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (utime(bpath, buf) < 0) {
        ret = -errno;
    }
    if (!buf) {
        // If buf == NULL, we attempted to set the times to the current time.
        DEBUG("hub_utime(path=%s, bpath=%s, actime=NULL, "
              "modtime=NULL) = %d (%s)\n", path, bpath, ret, terror(-ret));
    } else {
        DEBUG("hub_utime(path=%s, bpath=%s, actime=%"PRId64", "
              "modtime=%"PRId64") = %d (%s)\n", path, bpath,
              (int64_t)buf->actime, (int64_t)buf->modtime, ret, terror(-ret));
    }
    return ret;
}

int hub_statfs(const char *path, struct statvfs *vfs)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (statvfs(bpath, vfs) < 0) {
        ret = -errno;
    }
    DEBUG("hub_statfs(path=%s, bpath=%s) = %d (%s)\n",
          path, bpath, ret, terror(-ret));
    return ret;
}

char *alloc_zterm_xattr(const char *str, size_t len)
{
    char *nstr;

    if (!str) {
        return strdup("(NULL)");
    } else if (len <= 0) {
        return strdup("(empty)");
    }
    nstr = malloc(len + 1);
    if (!nstr) {
        return NULL;
    }
    memcpy(nstr, str, len);
    nstr[len] = '\0';
    return nstr;
}

int hub_setxattr(const char *path, const char *name, const char *value,
                        size_t size, int flags)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX], *nvalue = NULL;
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (setxattr(bpath, name, value, size, flags) < 0) {
        ret = -errno;
    }
#ifdef DEBUG_ENABLED
    nvalue = alloc_zterm_xattr(value, size);
    if (!nvalue) {
        ret = -ENOMEM;
        goto done;
    }
    DEBUG("hub_setxattr(path=%s, bpath=%s, value=%s) = %d (%s)\n",
          path, bpath, nvalue, ret, terror(-ret));
done:
#endif
    free(nvalue);
    return ret;
}

int hub_getxattr(const char *path, const char *name, char *value, size_t size)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX], *nvalue = NULL;
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (getxattr(bpath, name, value, size) < 0) {
        ret = -errno;
    }
#ifdef DEBUG_ENABLED
    if (ret == 0) {
        nvalue = alloc_zterm_xattr(value, size);
        if (!nvalue) {
            ret = -ENOMEM;
            goto done;
        }
        DEBUG("hub_getxattr(path=%s, bpath=%s, name=%s, value=%s) = 0\n",
              path, bpath, name, nvalue);
    } else {
        DEBUG("hub_getxattr(path=%s, bpath=%s, name=%s) = %d (%s)\n",
              path, bpath, name, ret, terror(-ret));
    }
done:
#endif
    free(nvalue);
    return ret;
}

int hub_listxattr(const char *path, char *list, size_t size)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (listxattr(bpath, list, size) < 0) {
        ret = -errno;
    }
    DEBUG("hub_listxattr(path=%s, bpath=%s, list=%s, size=%zd) = %d (%s)\n",
          path, bpath, list, size, ret, terror(-ret));
    return ret;
}

int hub_removexattr(const char *path, const char *name)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (removexattr(bpath, name) < 0) {
        ret = -errno;
    }
    DEBUG("hub_removexattr(path=%s, bpath=%s, name=%s) = %d (%s)\n",
          path, bpath, name, ret, terror(-ret));
    return 0;
}

int hub_opendir(const char *path, struct fuse_file_info *info)
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    DIR *dp;
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    dp = opendir(bpath);
    if (!dp) {
        ret = -errno;
    } else {
        info->fh = (uintptr_t)dp;
    }
    DEBUG("hub_opendir(path=%s, bpath=%s) = %d (%s)\n",
          path, bpath, ret, terror(-ret));
    return ret;
}

int hub_readdir(const char *path, void *buf,
                fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *info)
{
    DIR *dp = (DIR*)(uintptr_t)info->fh;
    struct dirent *de;
    int ret = 0;

    DEBUG("hub_readdir(path=%s, offset=%"PRId64") begin\n",
          path, (int64_t)offset);
    errno = 0;
    if (offset != 0) {
        seekdir(dp, (long)offset);
    }
    while (1) {
        // TODO: portability: this is thread-safe on Linux, but maybe not elsewhere
        de = readdir(dp); 
        if (!de) {
            if (errno) {
                ret = -errno;
                break;
            }
            ret = 0;
            break;
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
            ret = 1;
            break;
        }
    }
#ifdef DEBUG_ENABLED
    {
        const char *exit_reason;

        if (ret < 0) {
            exit_reason = terror(-ret);
        } else if (ret == 1) {
            exit_reason = "filler satisified";
        } else {
            exit_reason = "no more entries";
        }
        DEBUG("hub_readdir(path=%s, offset=%"PRId64"): %s\n",
              path, (int64_t)offset, exit_reason);
    }
#endif
    if (ret > 0) {
        return 0;
    }
    return ret;
}

int hub_releasedir(const char *path, struct fuse_file_info *info)
{
    DIR *dp = (DIR*)(uintptr_t)info->fh;
    int ret = 0;

    if (closedir(dp) < 0) {
        ret = -errno;
    }
    DEBUG("hub_releasedir(path=%s) = %d (%s)\n",
          path, ret, terror(-ret));
    return ret;
}

int hub_fsyncdir(const char *path, int datasync, struct fuse_file_info *info)
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
    DEBUG("hub_fsyncdir(path=%s, datasync=%d) = %d (%s)\n",
          path, datasync, ret, terror(-ret));
    return ret;
}

int hub_utimens(const char *path, const struct timespec tv[2])
{
    struct hub_fs *fs = fuse_get_context()->private_data;
    char bpath[PATH_MAX];
    int ret = 0;

    snprintf(bpath, sizeof(bpath), "%s%s", fs->root, path);
    if (utimensat(AT_FDCWD, bpath, tv, 0) < 0) {
        ret = -errno;
    }
    DEBUG("hub_utimens(path=%s, atime.tv_sec=%"PRId64", "
        "atime.tv_nsec=%"PRId64", mtime.tv_sec=%"PRId64", "
        "mtime.tv_nsec=%"PRId64") = %d (%s)\n",
        path, tv[0].tv_sec, tv[0].tv_nsec, tv[1].tv_sec, tv[1].tv_nsec,
        ret, terror(-ret));
    return ret;
}

// vim: ts=4:sw=4:tw=79:et
