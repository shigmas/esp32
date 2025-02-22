VERSION=v5.4

SHELL := /bin/bash

# If we were running on the same host as the container, assuming the group ids might be okay,
# but since we have different distributions of linux, we should at least do this: get the
# group of the first serial device, then get the group id of that group. (Maybe a better way
# to do this?)
serialDeviceGroup := $(shell ls -l /dev/ttyS0 | awk '{print $$4}')
serialDeviceGid := $(shell getent group "$(serialDeviceGroup)" | awk -F : '{print $$3}')

help:
	@printf "Builder for esp-idf.\n"

# This will only be runnable on the build host (or a linux of the same distribution if you're
# lucky)
build_esp_idf_image:
	DOCKER_BUILDKIT=1 docker build -t esp_idf --build-arg DIALOUT_GID=$(serialDeviceGid) --build-arg ESP_IDF_VERSION=$(VERSION) -f Dockerfile.esp-idf .

run_esp_idf_image:
	docker run --rm -it --privileged -v /dev:/dev -v $(PWD)/output:/output esp_idf /bin/bash

