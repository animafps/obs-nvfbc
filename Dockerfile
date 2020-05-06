FROM debian:bullseye

RUN apt update \
 && apt install -y gcc meson ninja-build libobs-dev \
 && rm -rf /var/lib/apt/lists/*
