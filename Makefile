STATIC_ANALYSIS=

test:
	cd tests/build && make && ./erc-test --ascii

build:
	cd build && make

cmake:
	cd build && cmake ..
	cd tests/build && cmake ..

static:
	cd build && STATIC_ANALYSIS=1 cmake ..
	cd tests/build && STATIC_ANALYSIS=1 cmake ..

clean:
	cd build && make clean
	cd tests/build && make clean

.PHONY: test build cmake static clean
