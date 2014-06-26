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

#ifndef IOHUB_FILE_H
#define IOHUB_FILE_H

#include <fuse.h>
#include <unistd.h> // for size_t
#include <sys/types.h> // for mode_t, dev_t

int hub_fgetattr(const char *path, struct stat *stat,
                        struct fuse_file_info *info);
int hub_create(const char *path, mode_t mode,
                      struct fuse_file_info *info);
int hub_open(const char *path, struct fuse_file_info *info);
int hub_read(const char *, char *, size_t, off_t, struct fuse_file_info *);
int hub_write(const char *, const char *, size_t, off_t, struct fuse_file_info *);
int hub_flush(const char *path, struct fuse_file_info *info);
int hub_release(const char *path, struct fuse_file_info *info);
int hub_fsync(const char *path, int datasync, struct fuse_file_info *info);
int hub_ftruncate(const char *path, off_t len,
                         struct fuse_file_info *info);
int hub_fallocate(const char *path, int mode, off_t offset,
                         off_t len, struct fuse_file_info *info);

#endif

// vim: ts=4:sw=4:et
