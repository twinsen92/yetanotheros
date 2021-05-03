.PHONY: all clean headers iso

all: headers iso

clean:
	./clean.sh

headers:
	./headers.sh

iso:
	./iso.sh

