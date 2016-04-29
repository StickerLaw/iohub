iohub
==================================
IoHub is a userspace I/O scheduler, implemented as a FUSE filesystem.

The goal is to allow allocating different amounts of disk I/O bandwidth to
different users.  For example, you might use this to run a low-priority (but
I/O intensive) process alongside a high priority one, without sacricing the
latency of the high-priority process.

IoHub is implemented using the Linux FUSE (filesystem in userspace) driver.  It
forwards all I/O requests down to "underlying filesystems" (aka underfs
instances).

This is just a prototype, not intended for production.  For example, a
production system would not hard-code user quotas, but make them configurable.
A production system might also feature a more efficient quota enforcement
mechanism.

Supported Platforms
-----
Currently, only Linux is supported, mainly because fuse is a Linux-specific
library.  With a little more effort, other POSIX platforms could be supported
if they have a fuse-like library available.

How to Build
-----
Before attempting to compile iohub, be sure to install the fuse and fuse-devel
packages for your operating system.

```bash
git clone https://github.com/cmccabe/iohub.git
mkdir build
cd build
cmake ..
make
```

Usage
-----
Example usage

```bash
mkdir -p /tmp/overfs /tmp/underfs
sudo ./iohub /tmp/overfs /tmp/underfs
```

License
-----
IoHub is licensed under the Apache 2.0 license.  See LICENSE.txt for more
details.

Colin P. McCabe
