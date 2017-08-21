#!/usr/bin/env bash
set -e

JDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "$JDIR"/util.sh

set -x

git submodule init
git submodule sync
git submodule update

# Cleanup
sudo env "PATH=$PATH" ./waf --color=yes distclean

if [[ $JOB_NAME != *"code-coverage" && $JOB_NAME != *"limited-build" ]]; then
  # Configure/build in debug mode with tests
  ./waf --color=yes configure --with-tests --debug
  ./waf --color=yes build -j${WAF_JOBS:-1}

  # Cleanup
  sudo env "PATH=$PATH" ./waf --color=yes distclean

  # Configure/build in optimized mode without tests
  ./waf --color=yes configure
  ./waf --color=yes build -j${WAF_JOBS:-1}

  # Cleanup
  sudo env "PATH=$PATH" ./waf --color=yes distclean
fi

# Configure/build in optimized mode with tests
if [[ $JOB_NAME == *"code-coverage" ]]; then
    COVERAGE="--with-coverage"
elif [[ -n $BUILD_WITH_ASAN || -z $TRAVIS ]]; then
    ASAN="--with-sanitizer=address"
fi
./waf --color=yes configure --with-tests $COVERAGE $ASAN
./waf --color=yes build -j${WAF_JOBS:-1}

# (tests will be run against optimized version)

# Install
sudo env "PATH=$PATH" ./waf --color=yes install
