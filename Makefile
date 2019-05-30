GXX     = g++
CFLAGS  = -I. -O2
LDFLAGS = -std=c++11 -lstdc++ -lpthread -ldl

app_name                := laser
docker_name             := $(app_name)
docker_tag              := dev
docker_container        := $(app_name)

.PHONY: test
test:
	$(GXX) $(CFLAGS) test.cpp radar.cpp loguru.cpp $(LDFLAGS) -o $@

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
