spdlog_version = 1.3.1

GXX     = g++
CFLAGS  = -I. -I./spdlog-$(spdlog_version)/include -O2
LDFLAGS = -std=c++11 -lstdc++ -lpthread -ldl

app_name                := laser
docker_name             := $(app_name)
docker_tag              := dev
docker_container        := $(app_name)

.PHONY: all
all:
	$(GXX) -c -fPIC $(CFLAGS) radar.cpp $(LDFLAGS) -o radar.o
	$(GXX) -shared radar.o -o radar.so

.PHONY: test
test:
	$(GXX) $(CFLAGS) test.cpp radar.cpp $(LDFLAGS) -o $@

.PHONY: upgrade
upgrade:
	docker pull buildpack-deps:stable-scm

.PHONY: build
build:
	docker build -t $(docker_name):$(docker_tag) .

.PHONY: run
run:
	docker-compose up --force-recreate --remove-orphans --build -d

.PHONY: exec
exec:
	docker exec -e COLUMNS="`tput cols`" -e LINES="`tput lines`" -it $(docker_container) /usr/bin/fish
