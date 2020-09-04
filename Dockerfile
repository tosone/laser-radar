FROM buildpack-deps:stable-scm

RUN sed -i "s/deb.debian.org/mirrors.aliyun.com/g" /etc/apt/sources.list && \
  sed -i "s/security.debian.org/mirrors.aliyun.com/g" /etc/apt/sources.list && \
  apt-get update && apt-get upgrade -y && \
  apt-get install -y --no-install-recommends \
  bc tree vim bash-completion apt-utils man-db \
  build-essential locales doxygen ntp && \
  echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen && \
  echo "zh_CN.UTF-8 UTF-8" >> /etc/locale.gen && locale-gen && \
  rm -f /etc/localtime && ln -s /usr/share/zoneinfo/Asia/Shanghai /etc/localtime

WORKDIR /app

VOLUME /app
