# pintos

## About
Pintos is a simple operating system framework for the 80x86 architecture. It supports kernel threads, loading and running user programs, and a file system, but it implements all of these in a very simple way. Stanford provides related documentation at [Pintos Projects](https://web.stanford.edu/class/cs140/projects/pintos/pintos.html#SEC_Top).

## Source Code
Source code is adopted from the open source project at [Pintos](https://pintos-os.org/), which is updated from the source code provided in the documentation, especially the patch for the new version of bochs, and bochs-2.2.6 is no longer available for the latest gcc compiler. 


## Installation
Refer to the official documentation Appendix G Installing Pintos for installation.

**1. Downloading pintos and bochs**
```bash
git clone git://pintos-os.org/pintos-anon
wget https://sourceforge.net/projects/bochs/files/bochs/2.6.2/bochs-2.6.2.tar.gz
```

**2. Installing dependencies**
* GUN binutils
```bash
wget http://ftpmirror.gnu.org/binutils/binutils-2.21.1.tar.bz2
tar xjf binutils-2.21.1.tar.bz2
cd binutils-2.21.1
./configure --prefix=/usr/local --target=i386-elf --disable-werror
make
sudo make install 
cd ..
```

* Other dependencies
```bash
sudo apt-get update 
sudo apt-get -y install build-essential # -y 
sudo apt-get -y install xorg-dev
sudo apt-get -y install bison
sudo apt-get -y install libgtk2.0-dev
sudo apt-get -y install libc6:i386 libgcc1:i386 libstdc++5:i386 libstdc++6:i386
sudo apt-get -y install libncurses5:i386
sudo apt-get -y install g++-multilib
```

**3. Installing bochs**

There is a sample script under`pintos-anon/src/misc`, add the **user path** for `SRCDIR`,`PINTOSDIR`, `DSTDIR`.

Run the script to install bochs.
```bash
cd pintos-anon/src/misc
sudo ./bochs-2.6.2-build.sh
```
**4. Installing pintos**

The bochs executable and the numerous script files for pintos need to be added to the environment variables.
```bash
export PATH="~/bin:$PATH"
export PATH="~/pintos-anon/src/utils:$PATH"
```
Or put them under the `usr/bin` path.

Make sure these script files have executable permissions.

**5. Compiling pintos**
```bash
cd ~/pintos-anon/src/utils
```
A little modification is needed here.
* Makefile. change `LDFLAGS` to `LDLIBS`.
* squish*.c. Comment out the <stropts.h> library and related codes.
```bash
make
```
Also squish* need to be added to the environment variables.

**6. Testing**
```bash
cd ~/pintos-anon/src/threads
make
cd build
```
The official documentation suggests using the following code for testing.
```bash
pintos -- run alarm-multiple
```
A more refined test can be performed using the following command.
```bash
cd ~/pintos-anon/src/threads
make check
```







