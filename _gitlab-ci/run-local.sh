#!/bin/bash

# Helper script to run CI test locally without a gitlab runner. This files should be in sync with the .gitlab-ci.yml contents.

#exit immediately if any command fails
set -e

VERSION=$(git log -1 --pretty=%h)

docker build \
  -t base:$VERSION --target base --build-arg VERSION=$VERSION .
  
docker build \
  -t build:$VERSION --target build --build-arg VERSION=$VERSION .

#docker run -v `pwd`/.:/test -i -t falcon-test bash
#docker run -v `pwd`/.:/test -i -t falcon-test /test/testscripts/./test_all.sh /falcon/build /tmp
#docker run -i -t build:$VERSION /bin/bash -c "cd c-mnalib/build; make test"
docker run -i -t build:$VERSION /bin/bash -c "c-mnalib/build/src/cmnalib/./testlib"

#docker run -v `pwd`/.:/test -i -t falcon-test /bin/bash -c "/test/testscripts/./test_all.sh /falcon/build /tmp; bash"
