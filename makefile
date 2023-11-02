LIBS=-lncursesw

build: src/*.cc
	g++ -g -Og -D DEBUG src/*.cc $(LIBS) -o av

release: src/*.cc
	g++ -O3 src/*.cc $(LIBS) -o av

install: av
	mkdir -p /usr/bin
	mv av /usr/bin/av

clean:
	rm -f av
