# FYI: kind of dangerous, but 5.4 didn't work. 94cfe39 works if something breaks before we
# land on a release
# plain ESP_IDF version
IDF_VERSION=v6.0.1
# ESP_IDF version for ESP-WHO
WHO_IDF_VERSION=v5.5.4
# current master, as the latest release is 8-9 years ago. This is master as of now (2026-06-23) and
# there have been no changes for 5 months
ESP_WHO=1681a1c

SHELL := /bin/bash
HOST_UID = $(shell id -u)
HOST_GID = $(shell id -g)

# If we were running on the same host as the container, assuming the group ids might be okay,
# but since we have different distributions of linux, we should at least do this: get the
# group of the first serial device, then get the group id of that group. (Maybe a better way
# to do this?)
serialDeviceGroup := $(shell ls -l /dev/ttyS0 | awk '{print $$4}')
serialDeviceGid := $(shell getent group "$(serialDeviceGroup)" | awk -F : '{print $$3}')
sourceDir := src
help:
	@printf "Builder for esp-idf.\n"
	@printf "Targets for original, and esp-who.\n"

# This will only be runnable on the build host (or a likely linux of the same distribution.
# Also, you have be a member of the group. This is a requirement for flashing, so it's a good
# early check.
build_esp_idf_image:
	mkdir -p $(sourceDir) && sudo chgrp $(serialDeviceGroup) $(sourceDir)
	DOCKER_BUILDKIT=1 docker build -t esp_idf.$(IDF_VERSION) --build-arg DIALOUT_GID=$(serialDeviceGid) --build-arg ESP_IDF_VERSION=$(IDF_VERSION) -f Dockerfile.esp-idf .

run_esp_idf_image:
	docker run \
	-v ${SSH_AUTH_SOCK}:${SSH_AUTH_SOCK} \
	-e SSH_AUTH_SOCK=${SSH_AUTH_SOCK} \
	-v ${HOME}/.ssh:/home/ubuntu/.ssh \
	--rm -it --privileged -v /dev:/dev -v $(PWD)/$(sourceDir):/$(sourceDir) \
	esp_idf.$(IDF_VERSION) /bin/bash

build_esp_who_image:
	mkdir -p $(sourceDir) && sudo chgrp $(serialDeviceGroup) $(sourceDir)
	DOCKER_BUILDKIT=1 docker build -t esp_idf --build-arg DIALOUT_GID=$(serialDeviceGid) --build-arg ESP_IDF_VERSION=$(WHO_IDF_VERSION) -f Dockerfile.esp-idf .
	DOCKER_BUILDKIT=1 docker build -t esp_who --build-arg DIALOUT_GID=$(serialDeviceGid) --build-arg ESP_WHO_VERSION=$(ESP_WHO_VERSION) -f Dockerfile.esp-who .

run_esp_who_image:
	docker run \
	-v ${SSH_AUTH_SOCK}:${SSH_AUTH_SOCK} \
	-e SSH_AUTH_SOCK=${SSH_AUTH_SOCK} \
	-v ${HOME}/.ssh:/home/ubuntu/.ssh \
	--rm -it --privileged -v /dev:/dev -v $(PWD)/$(sourceDir):/$(sourceDir) \
	esp_who /bin/bash
