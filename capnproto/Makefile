

all: client server

blogstore.capnp.h blogstore.capnp.c++: blogstore.capnp
	@capnpc -oc++ $<

client: client.cpp blogstore.capnp.c++ blogstore.capnp.h
	@g++ -std=c++14 -Wall client.cpp blogstore.capnp.c++ \
    	$(pkg-config --cflags --libs capnp-rpc) -o $@

server: server.cpp blogstore.capnp.c++ blogstore.capnp.h
	@g++ -std=c++14 -Wall server.cpp blogstore.capnp.c++ \
    	$(pkg-config --cflags --libs capnp-rpc) -o $@

clean:
	@rm -f clean server blogstore.capnp.c++ blogstore.capnp.h