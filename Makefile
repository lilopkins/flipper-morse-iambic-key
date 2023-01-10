APP_ID = $(shell grep appid application.fam | awk -F '=' '{print $$2}' | tr -d '",')

.PHONY: build
build: application.fam *.c *.png
	mkdir -p build
	# If the firmware isn't cloned, lets get it...
	[ -d build/flipper ] || git clone --recursive https://github.com/flipperdevices/flipperzero-firmware.git build/flipper
	# ... and always check if it's up-to-date.
	cd build/flipper && git pull && git submodule update --recursive

	# Copy the application (except the build folder) into the firmware to build.
	rsync -av --exclude='build' --exclude='.git' --exclude='.github' . build/flipper/applications_user/$(APP_ID)/
	# Build the firmware if needed
	cd build/flipper && [ -d build/latest ] || ./fbt
	# Build the application
	cd build/flipper && ./fbt fap_$(APP_ID)
	# Copy the application out from the build environment
	cp build/flipper/build/latest/.extapps/$(APP_ID).fap build/

.PHONY: clean
clean:
	rm -rf build
