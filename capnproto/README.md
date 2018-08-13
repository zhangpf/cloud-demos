# A simple example on Capnproto
---------

## Introduction

Using [capnproto](https://capnproto.org/) to implement a key-value storage service. The interface between the service and client supports three different kinds of operations:
  * `get(key)`: get the value *(in this demo, the value is some blog text)* by key.
  * `store(key, blog)`: store the key and blog pair.
  * `remove(key)`: remove the key and corresponding blog from the storage.

Moreover, we pipeline `get` and `store` operation to implement the `copy(key1, key2)` operation, without changing the capnp interface file. That is, the original procedure:
```
value = get(key1);
store(key2, value);
```
is changed to something looks like:
```
getPromise = get(key1);
storePromise = store(key2, getPromise);
storePromise.then(...);
```
There is no data transfer in the intermidiate step.

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

The below table shows the average operation latency on the c3.large instance in AWS US East (Virginia), and the network condition is moderate.

| Operation          | Get   | Store | Remove | Copy  |
| :----------------: | :---: | :---: | :----: | :---: |
| Unix domain socket | 207µs | 161µs | 152µs  | 232µs |
| Loopback device    | 246µs | 163µs | 152µs  | 267µs |
| Local network      | 446µs | 372µs | 301µs  | 381µs |

The time for `copy` is comparable with that of `get`.
Moreover, `copy` is much smaller than the sum of `get` and `store` in all cases.

## Note
Some of the code is adopted from [offical samples](https://github.com/capnproto/capnproto/blob/master/c%2B%2B/samples).
