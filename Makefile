.PHONY: build
build: application.fam *.c *.png
	[ -d build ] || git clone --recursive https://github.com/flipperdevices/flipperzero-firmware.git build
	cd build && git pull && git submodule update --recursive
	rsync -av --exclude='build' --exclude='.git' --exclude='.github' . build/applications_user/project_app/
	cd build && ./fbt fap_dist

.PHONY: clean
clean:
	rm -rf build
