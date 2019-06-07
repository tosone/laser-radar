FROM ros:lunar-perception

RUN sed -i 's/archive.ubuntu.com/mirrors.163.com/g' /etc/apt/sources.list && \
  sed -i 's/security.ubuntu.com/mirrors.163.com/g' /etc/apt/sources.list

RUN apt-get update && apt-get upgrade -y && \
  apt-get install -y --no-install-recommends \
  bc tree vim bash-completion apt-utils man-db \
  build-essential locales doxygen ntp

RUN echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen && \
  echo "zh_CN.UTF-8 UTF-8" >> /etc/locale.gen && locale-gen

RUN rm -f /etc/localtime && ln -s /usr/share/zoneinfo/Asia/Shanghai /etc/localtime

WORKDIR /app

VOLUME /app
