.PHONY: all clean build

all: build

build: @types

clean:
	rm -rf @types

@types:
	npx @ts-for-gir/cli \
		-g ../../builddir/src \
		-g /usr/share/gir-1.0 \
		generate \
		--modules Gxr-0.16
