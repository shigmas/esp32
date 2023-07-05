VERSION=v5.1

help:
	@printf "Builder for blink\n"

build_blink_image:
	DOCKER_BUILDKIT=1 docker build -t blink \
	--build-arg ESP_IDF_VERSION=$(VERSION) \
	--build-arg ESP32_PLATFORM=esp32-c3 \
	-f Dockerfile.blink .

run_blink_image:
	docker run --rm -it --privileged=true -v /dev:/dev -v $(PWD)/output:/output blink /bin/bash
