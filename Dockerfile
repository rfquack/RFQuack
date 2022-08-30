FROM python:3.9-slim

VOLUME [ "/tmp/RFQuack" ]

LABEL maintainer "RFQuack"

WORKDIR /tmp

# Install RFQuack as library.
COPY . RFQuack

# Install stuff
RUN apt-get update \
  && apt-get install -y --no-install-recommends \
    git \
    wget \
    unzip \
    make \
    protobuf-compiler \
  && apt-get purge -y --auto-remove \
  && rm -rf /var/lib/apt/lists/* \
  && pip install -U pip \
  && pip install -r /tmp/RFQuack/requirements.pip \
  && pio pkg install -g --platform "platformio/espressif32@^5.1.0"

RUN platformio lib -g install file:///tmp/RFQuack \
  && cd $HOME/.platformio/lib/RFQuack/ \
  && make proto

# TODO add build examples as a simple regression test here

# Add project files
WORKDIR /quack
COPY docker/project .

COPY docker/my-entrypoint.sh /my-entrypoint.sh

ENTRYPOINT ["/bin/sh"]
CMD ["/my-entrypoint.sh"]
