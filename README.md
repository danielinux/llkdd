## Learning Linux Kernel Device Drivers

Last update: 2014.05.13 (Di) 14:27:32 (UTC +0200 CEST)

### What is this about ?

This is a way to learn how to write/programm device drivers for the Linux Kernel.
The primary goal is to learn to use the available API (3.x) and the pertinent
concepts and write correct code and document it, in order to reduce the learning
curve. Optimizations may be included/use if appropriate. Correctness comes first.

This is a working in progress, and therefore bugs/errors may be found. Suggestions,
critiques, contributions are welcome. :)

### Pre-requisites

Before starting, it is import to have already experience with C programming.
C is the Linux Kernel "Lingua Franca". Almost 97% the source code is written
C and a very part in Assembly. A good knowledge about hardware standards and
how it works is strongly recommended, specially for device drivers.

### Compile the code

The default path for the current Kernel source code points to `$HOME/src/linux`.
Put the kernel source code in this direcory or create a symlink to it. The next
step is change to one of the driver's directory and then execute make:

```sh
cd helloworld
make
```

and the corresponding device driver will be compiled.

To use the compiled driver:

```sh
sudo insmod drivername.ko
```
And it is loaded in memory. To remove it just type:

```sh
sudo  rmmod drivername.ko
```

In order to see the driver's messages in the kernel execute in a terminal:

```sh
journalctl -f _TRANSPORT=kernel
```
if you use systemd or:

```sh
tail -f /var/log/kern.log
```

if you use syslog-ng.

See more infos at the project [wiki page](https://github.com/rafaelnp/llkdd/wiki).
