# Windows User-Mode Scheduling
----------

## Introduction

The [UMS (User-Mode Scheduling)](https://docs.microsoft.com/en-us/windows/desktop/procthread/user-mode-scheduling) is a feature added to Windows 7 and later, but only works in 64bit versions. It enable applications to schedule their own threads in their own custom policy. In this way, it provides the capability of fiber/coroutine, but in the form of native thread.

We compare the performance of UMS with that of native thread with a very simple implementation. 


## Build 

1. Install [CMake](https://cmake.org/download/) and [Visual Studio](https://visualstudio.microsoft.com/) in 64bit Windows (7 or later).

2. In cmd, generate the VS project.
```
mkdir build
cd build
cmake ..
```

3. Open `ALL_BUILD.vcxproj` in Visual Studio.

4. Choose `Release` + `x64` configuration, and `Generate Solutions`

## Run

1. Run UMS version

```
Release\ms-example.exe <number-of-threads> <num-of-yields>
```

2. Run Native thread version

```
Release\thread-example.exe <number-of-threads> <num-of-yields>
```


## Performance comparison

On my laptop (Lenovo Thinkpad X270 with Intel i5-6200U and 8G memory), The average execution time for each yield is:

1. **10**-threads concurrency:

| Number of yields   | 100   | 1000 | 10000 | 100000  |
| :----------------: | :---: | :---: | :----: | :---: |
| Native thread | 1201ns | 633ns  | 640ns | 632ns | 
| UMS    | 2752ns | 400ns | 148ns  | 118ns |

2. **100**-threads concurrency:

| Number of yields   | 100   | 1000 | 10000 | 100000  |
| :----------------: | :---: | :---: | :----: | :---: |
| Native thread | 769ns | 610ns  | 601ns | 591ns | 
| UMS    | 1428µs | 245µs | 152µs  | 128µs |

3. **1000**-threads concurrency:

| Number of yields   | 100   | 1000 | 10000 | 100000  |
| :----------------: | :---: | :---: | :----: | :---: |
| Native thread | 941ns | 790ns  | 793ns | 785ns | 
| UMS    | 1400ns | 276µs | 146ns  | 127ns |



## Notes
Some of the code is adopted from [pervognsen's gist](https://gist.github.com/pervognsen/8cbde6ea71da8256865e05bf4fcdfa7d).
