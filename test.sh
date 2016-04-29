#!/usr/bin/env bash

die() {
    echo $@
    exit 1
}

usage() {
    cat <<EOF
test.sh: test the iohub userspace scheduler.

usage:
    test.sh [test-name]

available tests:
    simple: test mounting followed by unmounting.
EOF
}

simple_test() {
    echo "*** Running simple_test..."
}

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
    *)  echo "Unknown test ${TEST_NAME}"
        echo
        usage;;
esac

exit 0
