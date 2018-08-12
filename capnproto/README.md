# A simple example on Capnproto
---------

## Introduction

Using [capnproto](https://capnproto.org/) to implement a key-value storage service. The interface between the service and client supports three different kinds of operations:
  * `get(key)`: get the value *(in this demo, valus is some blog text)* by key.
  * `store(key, blog)`: store the key and blog pair.
  * `remove(key)`: remove the key and cosponding blog from the storage.

Moreover, we pipeline `get` and `store` operation to implement the `copy(key1, key2)` operation, without changing the interface capnp file.


## Dependencies

Having been tested on Ubuntu 16.04 and Mac OS.

### Linux

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

### Mac OS
```
brew install capnp
```

## Run

Build it.
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

## Performance

The below table shown the average operation latency on c3.large instance in AWS US East (Virginia).

| Operation          | Get   | Store | Remove | Copy  |
| :----------------: | :---: | :---: | :----: | :---: |
| Unix domain socket | 247µs | 175µs | 359µs  | 267µs |
| Same host          | 267µs | 180µs | 363µs  | 318µs |
| Local network      | 344µs | 268µs | 483µs  | 338µs |

The time for `copy` is slightly larger than that of `get`, but when server and client are on different host, the `copy` is better than `get`, for there is no need to send back blog data in `copy`.

Moreover, `copy` is much smaller than the sum of `get` and `store` in all cases.

## Note
Some of the code is adopted from [offical samples](https://github.com/capnproto/capnproto/blob/master/c%2B%2B/samples).
