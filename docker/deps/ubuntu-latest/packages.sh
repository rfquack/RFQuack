#! /bin/sh

apt-get update \
  && apt-get install -y --no-install-recommends \
    git \
    wget \
    unzip \
    make \
    protobuf-compiler \
  && apt-get purge -y --auto-remove \
  && rm -rf /var/lib/apt/lists/*