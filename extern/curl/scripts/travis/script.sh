#!/bin/bash
set -eo pipefail

./buildconf

if [ "$T" = "coverage" ]; then
  ./configure --enable-debug --disable-shared --disable-threaded-resolver --enable-code-coverage --enable-werror --enable-alt-svc --with-libssh2
  make
  make TFLAGS=-n test-nonflaky
  make "TFLAGS=-n -e" test-nonflaky
  tests="1 200 300 500 700 800 900 1000 1100 1200 1302 1400 1502 3000"
  make "TFLAGS=-n -t $tests" test-nonflaky
  coveralls --gcov /usr/bin/gcov-8 --gcov-options '\-lp' -i src -e lib -e tests -e docs -b $PWD/src
  coveralls --gcov /usr/bin/gcov-8 --gcov-options '\-lp' -e src -i lib -e tests -e docs -b $PWD/lib
fi

if [ "$T" = "torture" ]; then
  ./configure --enable-debug --disable-shared --disable-threaded-resolver --enable-code-coverage --enable-werror --enable-alt-svc --with-libssh2
  make
  make TFLAGS=-n test-nonflaky
  make "TFLAGS=-n -e" test-nonflaky
  tests="1 200 300 500 700 800 900 1000 1100 1200 1302 1400 1502 3000"
  make "TFLAGS=-n --shallow=40 -t $tests" test-nonflaky
fi

if [ "$T" = "debug" ]; then
  ./configure --enable-debug --enable-werror $C
  make
  make examples
  if [ -z $NOTESTS ]; then
    if [ "$TRAVIS_ARCH" = "aarch64" ] ; then
      # TODO: find out why this test is failing on arm64
      make "TFLAGS=-n !323" test-nonflaky
    else
      make TFLAGS=-n test-nonflaky
    fi
  fi
fi

if [ "$T" = "debug-wolfssl" ]; then
  ./configure --enable-debug --enable-werror $C
  make
  make "TFLAGS=-n !313" test-nonflaky
fi

if [ "$T" = "debug-mesalink" ]; then
  ./configure --enable-debug --enable-werror $C
  make
  make "TFLAGS=-n !313 !3001" test-nonflaky
fi

if [ "$T" = "novalgrind" ]; then
  ./configure --enable-werror $C
  make
  make examples
  make TFLAGS=-n test-nonflaky
fi

if [ "$T" = "normal" ]; then
  if [ $TRAVIS_OS_NAME = linux ]; then
    # Remove system curl to make sure we don't rely on it.
    # Only done on Linux since we're not permitted to on mac.
    sudo rm -f /usr/bin/curl
  fi
  ./configure --enable-warnings --enable-werror $C
  make
  make examples
  if [ -z $NOTESTS ]; then
    make test-nonflaky
  fi
  if [ -n $CHECKSRC ]; then
    echo "enable COPYRIGHTYEAR" > ./docs/examples/.checksrc
    echo "enable COPYRIGHTYEAR" > ./include/curl/.checksrc
    make checksrc
  fi
fi

if [ "$T" = "tidy" ]; then
  ./configure --enable-warnings --enable-werror $C
  make
  make tidy
fi

if [ "$T" = "iconv" ]; then
  source scripts/travis/iconv-env.sh
  ./configure --enable-debug --enable-werror $C
  make
  make examples
  make test-nonflaky
fi

if [ "$T" = "cmake" ]; then
  if [ $TRAVIS_OS_NAME = linux ]; then
    cmake -H. -Bbuild -DCURL_WERROR=ON
    cmake --build build
  else
    cmake -H. -Bbuild -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DCURL_DISABLE_LDAP=ON -DCURL_DISABLE_LDAPS=ON
    cmake --build build
  fi
fi

if [ "$T" = "distcheck" ]; then
  # find BOM markers and exit if we do
  ! git grep `printf '\xef\xbb\xbf'`
  ./configure
  make
  ./maketgz 99.98.97
  # verify in-tree build - and install it
  tar xf curl-99.98.97.tar.gz
  cd curl-99.98.97
  ./configure --prefix=$HOME/temp
  make
  make TFLAGS=1 test
  make install
  # basic check of the installed files
  cd ..
  bash scripts/installcheck.sh $HOME/temp
  rm -rf curl-99.98.97
  # verify out-of-tree build
  tar xf curl-99.98.97.tar.gz
  touch curl-99.98.97/docs/{cmdline-opts,libcurl}/Makefile.inc
  mkdir build
  cd build
  ../curl-99.98.97/configure
  make
  make TFLAGS='-p 1 1139' test
  # verify cmake build
  cd ..
  rm -rf curl-99.98.97
  tar xf curl-99.98.97.tar.gz
  cd curl-99.98.97
  mkdir build
  cd build
  cmake ..
  make
  cd ../..
fi

if [ "$T" = "fuzzer" ]; then
  # Download the fuzzer to a temporary folder
  ./tests/fuzz/download_fuzzer.sh /tmp/curl_fuzzer

  export CURLSRC=$PWD

  # Run the mainline fuzzer test
  pushd /tmp/curl_fuzzer
  ./mainline.sh ${CURLSRC}
  popd
fi

if [ "$T" = "scan-build" ]; then
  scan-build ./configure --enable-debug --enable-werror $C
  scan-build --status-bugs make
  scan-build --status-bugs make examples
fi
