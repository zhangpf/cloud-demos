# A simple example to use Capnproto
---------

## Introduction

Using [capnproto](https://capnproto.org/) to implement a key-value storage service. The interface between the service and client supports three different kinds of operations:
  * `get(key)`: get the value *(in this demo, valus is some blog text)* by key.
  * `store(key, blog)`: store the key and blog pair.
  * `remove(key)`: remove the key and cosponding blog from the storage.

Moreover, we pipeline `get` and `store` operation to implement the `copy(key1, key2)` operation, without changing the interface.


## Build and Run

Build and pass the test on Ubuntu 16.04.

Install dependencies:
```
sudo apt update
sudo apt install make g++ pkg-config -y
```

Install latest capnproto:
```
curl -O https://capnproto.org/capnproto-c++-0.6.1.tar.gz
tar zxf capnproto-c++-0.6.1.tar.gz
cd capnproto-c++-0.6.1
./configure
make -j6 check
sudo make install
sudo ldconfig
```

Build:
```
make all
```

Run with IP address and port. For exmaple:

```
./server *:9527
# Other host
./client <the ip of server>:9527
```

Or locally, using unix domain socket:

```
rm -f /tmp/capnp-$$
./server unix:/tmp/capnp-$$
# Other terminal
./client unix:/tmp/capnp-$$
```

Enjoy it!

## Note
Some of the code is adopted from [offical samples](https://github.com/capnproto/capnproto/blob/master/c%2B%2B/samples).