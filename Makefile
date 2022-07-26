all: build

# windows get current path
ifeq ($(OS),Windows_NT)
  SHELL=cmd
  SRC_PATH=$(shell echo %cd%)
  OUTPUT_PATH=$(shell echo %cd%)\build
  DOCKERFILE=.\res\qt.Dockerfile
else
  SRC_PATH=$(CURDIR)
  OUTPUT_PATH=$(CURDIR)/build
  DOCKERFILE=./res/qt.Dockerfile
  DESTINATION_PATH=$(SRC_PATH)
endif

DOCKER_TAG_NAME=advantech/qt-builder-hardknott

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
	ln -s -r $(DESTINATION_PATH)/lib/systemd/system/ydotool.service $(DESTINATION_PATH)/etc/systemd/system/graphical.target.wants/ydotool.service

clean:
	rm -rf build/ydotool*

clean-all:
	rm -rf build/*
