#!/usr/bin/env bash

die() {
    echo "${@}"
    exit 1
}

do_or_die() {
    echo "${@}"
    "${@}" || die "failed."
}

usage() {
    cat <<EOF
test.sh: test the iohub userspace scheduler.

usage:
    test.sh [test-name]

available tests:
    simple: test mounting followed by unmounting.
    fs_test: start iohub and run fs_test on the overfs.
EOF
}

setup_fuse() {
    # Locate the iohub binary.  It should be in our current test script
    # directory, because CMake puts it there.
    SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    IOHUB_BIN="${SCRIPT_DIR}/iohub"
    [ -x "${IOHUB_BIN}" ] || \
        die "failed to find iohub binary in the ${SCRIPT_DIR} directory."

    # Initialize constants
    UNDERFS="/dev/shm/underfs"
    OVERFS="/dev/shm/overfs"

    # Clean up any issues with previous script runs.
    /usr/bin/fusermount -u "${OVERFS}"
    rm -rf "${UNDERFS}" "${OVERFS}"
    mkdir -p "${UNDERFS}" "${OVERFS}" || \
        die "failed to mkdir under or over fs mount points."
    "${IOHUB_BIN}" -f "${UNDERFS}" "${OVERFS}" &
    FUSE_PID=$!
}

kill_fuse() {
    kill "${FUSE_PID}"
    wait
}

simple_test() {
    echo "*** Running simple_test..."
    setup_fuse
    sleep 1
    kill_fuse
}

fs_test() {
    echo "*** Running fs_test..."
    setup_fuse
    SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    FS_TEST_BIN="${SCRIPT_DIR}/fs_test"
    [ -x "${FS_TEST_BIN}" ] || \
        die "failed to find fs_test binary in the ${SCRIPT_DIR} directory."
    do_or_die "${FS_TEST_BIN}" "${OVERFS}"
    kill_fuse
}

### Main
if [ $# -lt 1 ]; then
    usage
    exit 1
fi
TEST_NAME="$1"
shift
case "${TEST_NAME}" in
    -h) usage; exit 0;;
    --help) usage; exit 0;;
    simple) simple_test;;
    fs_test) fs_test;;
    *)  echo "Unknown test ${TEST_NAME}"
        echo
        usage;;
esac

exit 0
