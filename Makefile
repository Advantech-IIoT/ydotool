all: build

YOCTO_VERSION=hardknott

SRC_PATH=$(CURDIR)
OUTPUT_PATH=$(CURDIR)/build
DESTINATION_PATH=$(SRC_PATH)
VERSION=$(shell git describe --tags `git rev-list --tags --max-count=1`)

ifeq ($(YOCTO_VERSION),hardknott)
  DOCKERFILE=./res/hardknott_qt_builder.Dockerfile
  DOCKER_TAG_NAME=advantech/qt-builder-hardknott
else
  DOCKERFILE=./res/kirkstone_qt_builder.Dockerfile
  DOCKER_TAG_NAME=advantech/qt-builder-kirkstone
endif

build-image:
	docker buildx build --platform linux/arm64 -t $(DOCKER_TAG_NAME) -f $(DOCKERFILE) .

build: build-image generate-makefile
	@echo "build in docker"
	docker run --rm --platform linux/arm64 -v $(SRC_PATH):/src $(DOCKER_TAG_NAME) make linux-build

linux-build: clean
	@echo "make start"
	mkdir -p build/; cd build/; $(MAKE) -j `nproc`

linux-generate-makefile: clean-all
	@echo "generate makefile in builder environment"
	mkdir -p build/; cd build/; cmake ..

generate-makefile:
	@echo "generate makefile in docker"
	docker run --rm --platform linux/arm64 -v $(SRC_PATH):/src $(DOCKER_TAG_NAME) make linux-generate-makefile

install:
	@echo "install ydotool"
	cp build/ydotool $(DESTINATION_PATH)/usr/local/bin
	cp build/ydotoold $(DESTINATION_PATH)/usr/local/bin
	cp build/ydotool.service $(DESTINATION_PATH)/lib/systemd/system
	cp build/70-ydotool.rules $(DESTINATION_PATH)/etc/udev/rules.d
	rm -f $(DESTINATION_PATH)/etc/systemd/system/graphical.target.wants/ydotool.service
	ln -s -r $(DESTINATION_PATH)/lib/systemd/system/ydotool.service $(DESTINATION_PATH)/etc/systemd/system/graphical.target.wants/ydotool.service

clean:
	rm -rf build/ydotool*

clean-all:
	rm -rf build/*

clean-cmakefiles:
	rm -rf build/CMakeFiles
	rm -rf build/Daemon
	rm -rf build/cmake_install.cmake
	rm -rf build/CMakeCache.txt
	rm -rf build/Makefile

create-sbom: clean-sbom clean-cmakefiles
	cd build && sbom-tool generate -b . -bc . -pn ydotool -pv $(VERSION) -ps Advantech -nsb "https://github.com/Advantech-IIoT"
	cp build/_manifest/spdx_2.2/manifest.spdx.json $(DESTINATION_PATH)/../../scripts/out/sbom/ydotool.manifest.spdx.json

clean-sbom:
	rm -rf build/_manifest
