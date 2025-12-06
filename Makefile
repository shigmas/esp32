# FYI: kind of dangerous, but 5.4 didn't work. 94cfe39 works if something breaks before we
# land on a release
VERSION=master

SHELL := /bin/bash

# If we were running on the same host as the container, assuming the group ids might be okay,
# but since we have different distributions of linux, we should at least do this: get the
# group of the first serial device, then get the group id of that group. (Maybe a better way
# to do this?)
serialDeviceGroup := $(shell ls -l /dev/ttyS0 | awk '{print $$4}')
serialDeviceGid := $(shell getent group "$(serialDeviceGroup)" | awk -F : '{print $$3}')
sourceDir := src
help:
	@printf "Builder for esp-idf.\n"

# This will only be runnable on the build host (or a likely linux of the same distribution.
# Also, you have be a member of the group. This is a requirement for flashing, so it's a good
# early check.
build_esp_idf_image:
	mkdir -p $(sourceDir) && sudo chgrp $(serialDeviceGroup) $(sourceDir)
	DOCKER_BUILDKIT=1 docker build -t esp_idf --build-arg DIALOUT_GID=$(serialDeviceGid) --build-arg ESP_IDF_VERSION=$(VERSION) -f Dockerfile.esp-idf .

run_esp_idf_image:
	docker run -v "${SSH_AUTH_SOCK}":/run/host-services/ssh-auth.sock \
	  -e SSH_AUTH_SOCK=/run/host-services/ssh-auth.sock \
	--rm -it --privileged -v /dev:/dev -v $(PWD)/$(sourceDir):/$(sourceDir) \
	esp_idf /bin/bash

