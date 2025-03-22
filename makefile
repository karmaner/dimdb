.PHONY: all clean

all:
	if [ -d build ]; then \
		rm -rf build; \
	fi

	mkdir -p build
	cd build && cmake .. && make -j8
	echo "make success"

clean:
	rm -rf build; \
	echo "clean success"

