/**
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

#ifndef IOHUB_META_H
#define IOHUB_META_H

#include <fuse.h>
#include <unistd.h> // for size_t
#include <sys/types.h> // for mode_t, dev_t

struct stat;
struct statvfs;
struct utimbuf;

int hub_getattr(const char *path, struct stat *stbuf);
int hub_readlink(const char *path, char *buf, size_t size);
int hub_mknod(const char *path, mode_t mode, dev_t dev);
int hub_mkdir(const char *path, mode_t mode);
int hub_unlink(const char *path);
int hub_rmdir(const char *path);
int hub_symlink(const char *oldpath, const char *newpath);
int hub_rename(const char *oldpath, const char *newpath);
int hub_link(const char *oldpath, const char *newpath);
int hub_chmod(const char *path, mode_t mode);
int hub_chown(const char *path, uid_t uid, gid_t gid);
int hub_truncate(const char *path, off_t off);
int hub_utime(const char *path, struct utimbuf *buf);
int hub_statfs(const char *path, struct statvfs *vfs);
int hub_setxattr(const char *path, const char *name, const char *value,
                        size_t size, int flags);
int hub_getxattr(const char *path, const char *name, char *value, size_t size);
int hub_listxattr(const char *path, char *list, size_t size);
int hub_removexattr(const char *path, const char *name);
int hub_opendir(const char *path, struct fuse_file_info *info);
int hub_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                   off_t offset, struct fuse_file_info *info);
int hub_releasedir(const char *path, struct fuse_file_info *info);
int hub_fsyncdir(const char *path, int datasync,
                        struct fuse_file_info *info);
int hub_utimens(const char *path, const struct timespec tv[2]);

#endif

// vim: ts=4:sw=4:et
