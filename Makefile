spdlog_version = 1.3.1

GXX     = g++
CFLAGS  = -I. -I./spdlog-$(spdlog_version)/include -O2
LDFLAGS = -std=c++11 -lstdc++ -lpthread -ldl
TAGET   = radar

app_name                := laser
docker_name             := $(app_name)
docker_tag              := dev
docker_container        := $(app_name)

.PHONY: all
all: clean
	$(GXX) -c -fPIC $(CFLAGS) radar.cpp $(LDFLAGS) -o $(TAGET).o
	$(GXX) -shared radar.o -o lib$(TAGET).so

.PHONY: test
test: clean
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

.PHONY: clean
clean:
	@$(RM) *.so *.o test
