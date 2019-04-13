FROM ubuntu:bionic

RUN apt update \
 && apt install -y gcc meson ninja-build obs-studio \
 && rm -rf /var/lib/apt/lists/*
