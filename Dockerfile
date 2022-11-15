FROM python:3.9-slim
LABEL maintainer "RFQuack"

ARG WORKDIR=/tmp/RFQuack
ARG OS=ubuntu-latest

ENV WORKDIR=${WORKDIR}
ENV OS=${OS}

VOLUME ${WORKDIR}

WORKDIR ${WORKDIR}

COPY . .

RUN /bin/sh docker/deps/${OS}/packages.sh \
  && /bin/sh docker/deps/python/packages.sh