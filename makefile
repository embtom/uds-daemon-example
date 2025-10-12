VARIANT ?= amd64

PKGDIR    := sockact-example
BUILD_DIR := $(PKGDIR)/build

PRESETS := amd64-Release amd64-Debug arm64-Release arm64-Debug

.PHONY: all debbuild debbuild-amd64 debbuild-arm64 $(PRESETS)

all: $(PRESETS)

debbuild:
	cd $(PKGDIR) && dpkg-buildpackage \
		--host-arch=$(VARIANT) \
		--unsigned-source \
		--unsigned-changes \
		--post-clean \
		--build=binary

amd64-debbuild:
	$(MAKE) debbuild VARIANT=amd64

arm64-debbuild:
	$(MAKE) debbuild VARIANT=arm64

$(PRESETS):
	cmake --preset=$@ -S $(PKGDIR) -B $(BUILD_DIR)/$@
	cmake --build $(BUILD_DIR)/$@ --parallel --target install
