VARIANT ?= amd64

PKGDIR    := sockact-example
BUILD_DIR := $(PKGDIR)/build

PRESETS := amd64-Release amd64-Debug arm64-Release arm64-Debug

# Default target when no args given
.DEFAULT_GOAL := all

.PHONY: all debbuild amd64-debbuild arm64-debbuild container $(PRESETS)

ifeq ($(INSIDE_CONTAINER),1)

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
	@if [ "$(CLEAN)" = "1" ]; then \
		echo ">>> Cleaning $(BUILD_DIR)/$@"; \
		rm -rf "$(BUILD_DIR)/$@"; \
	fi
	cmake --preset=$@ -S $(PKGDIR) -B $(BUILD_DIR)/$@
	cmake --build $(BUILD_DIR)/$@ --parallel --target install
	cd $(PKGDIR) && ctest --preset=$@

else

$(PRESETS) amd64-debbuild arm64-debbuild debbuild:
	@echo ">>> Running inside builder container: $@ (CLEAN=$(CLEAN))"
	podman-compose run --rm builder \
		$@ $(if $(CLEAN),CLEAN=$(CLEAN))

builder:
	podman-compose build builder

endif
