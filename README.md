## Learning Linux Kernel Device Drivers

Last update: 2014.04.01 (Di) 08:03:03 (UTC +0200 CEST)

### Prer-requisites

Before starting, it is import to have already experience with C programming.
C is the Linux Kernel "Lingua Franca". Almost 97% the source code is written
C and a very part in Assembly. A good knowledge about hardware standards and
how it works is strongly recommended, specially for device drivers.

## Compile the code

The default path for the current Kernel source code points to `$HOME/src/linux`.
Put the source code in this direcory or create a symlink to it. The next step is
change to one of the driver's directory and then execute make:

```sh
cd hello
make
```

and the corresponding device driver will be compiled.

See more infos at the project [wiki page](https://github.com/rafaelnp/llkdd/wiki).
