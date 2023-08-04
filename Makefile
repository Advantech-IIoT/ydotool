all: build

YOCTO_VERSION=hardknott

SRC_PATH=$(CURDIR)
OUTPUT_PATH=$(CURDIR)/build
DESTINATION_PATH=$(SRC_PATH)

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
