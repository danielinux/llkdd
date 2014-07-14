## Learning Linux Kernel Device Drivers

Last update: 2014.07.14 (Mo) 15:53:16 (UTC +0200 CEST)

### What is this about ?

This is a way to learn how to write/programm device drivers for the Linux Kernel.
The primary goal is to learn to use the available API (3.7.x or newer) and the
pertinent concepts and write correct code and document it, in order to reduce the learning
curve. Optimizations may be included/use if appropriate. Correctness comes first.

This is a working in progress, and therefore bugs/errors may be found. Suggestions,
critiques, contributions are welcome. :)

### Pre-requisites

Before starting, it is import to have already experience with C programming.
C is the Linux Kernel "Lingua Franca". Almost 97% the source code is written
C and a small part in Assembly. A good knowledge about hardware standards and
how it works is strongly recommended, specially for device drivers, as well the
operating system theory and concetps.

### Coding Style

The  Linux Kernel coding style is followed. You can find it here:

[Linux Kernel Coding Style](https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/Documentation/CodingStyle)


### Getting the Linux Kernel Code

The Linux Stable tree is used in this project. Here is how to get the code:

```sh
mkdir ~/src
cd ~/src
git clone git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
```

I suggest to use a tagged version instead of the master tree, because you know
where you are working on.

```
cd linux-stable
git co v3.14
```

One can replace the tag (in this case v3.14) by  any other one.
Then I suggest to make a symbolic link to your git tree. It makes your life easier
if you need to change to other trees and one keeps a conssitent name:

```
cd ~/src
ln -s linux-stable linux
```

### Compiling the kernel

This part is partially dependent from the Linus distro one uses. It is recommended
to take a look at one's distro documentation for more details.


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

### Install the llkdd udev rules file

Copy the udev rules file, as root, to the udev configuration directory:

```sh
sudo cp -v 10-llkdd.rules /etc/udev/rules.d
```

and update the udev configuration by running:

```sh
sudo udevadm control --reload-rules
```


### Current Status

Here is a short summary about the current drivers beeing developed and its status.


|    driver     |   code   |  documentation  |
|---------------|----------|-----------------|
|  helloworld   |   100%   |       100%      |
|    one        |   100%   |       100%      |
|    intn       |   100%   |       50%       |
|    intn2      |   100%   |       10%       |
|   keylogger   |    60%   |       10%       |

The code status columns means how many of the planned features were implemented, it does not mean the code has no bugs !

See more infos at the project [wiki page](https://github.com/rafaelnp/llkdd/wiki).
