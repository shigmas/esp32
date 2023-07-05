VERSION=v5.1

help:
	@printf "Builder for esp-idf.\n"

build_esp_idf_image:
	DOCKER_BUILDKIT=1 docker build -t esp_idf --build-arg ESP_IDF_VERSION=$(VERSION) -f Dockerfile.esp-idf .

run_esp_idf_image:
	docker run --rm -it -v $(PWD)/output:/output esp_idf /bin/bash

