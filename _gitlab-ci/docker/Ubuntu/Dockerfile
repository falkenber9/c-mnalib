ARG OS_VERSION=latest
FROM ubuntu:$OS_VERSION as ubuntu_base

# Provide command add-apt-repository and apt-utils
RUN apt-get update \
  && apt-get install -y software-properties-common apt-utils

# General dependencies
RUN apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get install -y curl tzdata build-essential git cmake


FROM ubuntu_base as ubuntu_deps
  
# C-mnalib dependencies
WORKDIR / 
RUN apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get install -y libglib2.0-dev libudev-dev libcurl4-gnutls-dev
#  \
#  && git clone --branch falcon_v0.1 https://github.com/falkenber9/c-mnalib.git \
#  && mkdir /c-mnalib/build \
#  && cd /c-mnalib/build \
#  && cmake ../ \
#  && make -j install


ENTRYPOINT [ "stdbuf", "-o", "L" ]
