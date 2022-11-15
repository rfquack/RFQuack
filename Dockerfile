FROM python:3.9-slim
LABEL maintainer "RFQuack"

ARG WORKDIR=/tmp/RFQuack
ENV WORKDIR=${WORKDIR}

VOLUME ${WORKDIR}

WORKDIR ${WORKDIR}

COPY . .

RUN /bin/sh docker/deps/ubuntu/packages.sh \
  && /bin/sh docker/deps/python/packages.sh