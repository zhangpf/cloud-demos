
.PHONY : clean all

CAPNP_DEPS := $(shell pkg-config --cflags --libs capnp-rpc)

all: server client

blogstore.capnp.h blogstore.capnp.c++: blogstore.capnp
	capnpc -oc++ $<

client: client.cpp blogstore.capnp.c++ blogstore.capnp.h
	g++ -std=c++14 -Wall client.cpp blogstore.capnp.c++ $(CAPNP_DEPS) -o $@

server: server.cpp blogstore.capnp.c++ blogstore.capnp.h
	g++ -std=c++14 -Wall server.cpp blogstore.capnp.c++ $(CAPNP_DEPS) -o $@

clean:
	rm -f client server blogstore.capnp.c++ blogstore.capnp.h