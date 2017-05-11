# taken from: https://raw.githubusercontent.com/dockerfile/ubuntu/master/Dockerfile
# 
# Ubuntu Dockerfile: https://github.com/dockerfile/ubuntu

FROM ubuntu:14.04

SHELL ["/bin/bash", "-c"]
ENV DEBIAN_FRONTEND noninteractive
ENV CLICOLOR=1
ENV LSCOLORS=GxFxCxDxBxegedabagaced
ENV GREP_OPTIONS='--color=auto'

RUN echo America/Los_Angeles | sudo tee /etc/timezone && \
  sudo dpkg-reconfigure --frontend noninteractive tzdata

RUN \
  apt-get update -qq -y && \
  apt-get upgrade -qq -y

RUN apt-get install -qq -y  \
  build-essential \
  software-properties-common \
  byobu \
  curl \
  git \
  htop \
  man \
  unzip \
  vim \
  wget

RUN apt-get install -qq -y \
    gcc \
    gcc-arm-none-eabi \
    cmake \
    cppcheck \
    vera++ \
    python

RUN apt-get install -qq -y \
  gawk \
  bc \
  sed \
  findutils

ENV HOME /root
ENV JERRYSCRIPT_BUILD /root/builds/jerryscript

RUN mkdir -p $JERRYSCRIPT_BUILD
WORKDIR $JERRYSCRIPT_BUILD
COPY . .

RUN ls
RUN python ./tools/build.py \
  --debug \
  --lto=off

RUN python ./tools/run-tests.py \
  # --check-cppcheck \
  --check-vera \
  --check-license \
  --buildoption-test \
  --jerry-tests \
  --jerry-test-suite \
  --unittests \
  # --precommit \
  --test262; exit 0

CMD  ./build/bin/jerry --version; /bin/bash