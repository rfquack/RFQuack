FROM python:3.8-slim

LABEL maintainer "RFQuack"
ENV PROTOBUF_URL="https://github.com/protocolbuffers/protobuf/releases/download/v3.11.3/protoc-3.11.3-linux-x86_64.zip"
ENV NANOPB_URL="https://github.com/nanopb/nanopb/archive/nanopb-0.3.9.2.zip"
ENV RADIOLIB_URL="https://github.com/rfquack/RadioLib.git"

# Install stuff
RUN apt-get update \
  && apt-get install -y --no-install-recommends git wget unzip make \
  && apt-get purge -y --auto-remove \
  && rm -rf /var/lib/apt/lists/* \
  && pip install -U platformio==4.1.0 \
  && platformio platform install espressif32 \
  && pip install j2cli \
  && pip install google==2.0.3 \
  && pip install protobuf==3.11.3

WORKDIR /tmp

# Install protoc
RUN wget -O pb.zip ${PROTOBUF_URL}  \
  && unzip -p pb.zip bin/protoc > /usr/bin/protoc \
  && chmod +x /usr/bin/protoc

# Install RFQuack as library.
COPY . RFQuack
# NOTE: platformio will **COPY** the library to its private folder.
RUN platformio lib -g install file:///tmp/RFQuack \
  # Compile protobuf
  && cd $HOME/.platformio/lib/RFQuack/ \
  # Build protobuf than examples:
  && make

# Add project files
WORKDIR /quack
COPY docker/project .

COPY docker/my-entrypoint.sh /my-entrypoint.sh

ENTRYPOINT ["/bin/sh"]
CMD ["/my-entrypoint.sh"]
