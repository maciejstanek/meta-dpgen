Digital Pattern Generator
=========================

This is a layer for Yocto Project which provides a simple digital pattern
generator (`dpgen`) for Intel Galileo.

![Intel Galileo](https://upload.wikimedia.org/wikipedia/commons/thumb/f/f8/IntelGalileoGen2.png/320px-IntelGalileoGen2.png)

Dependencies
------------

Aside from all the `poky` stuff, this layers depends on
`meta-openembedded/meta-oe` which provides `mraa` package. This setup was based
on `thud` branch.

Moreover, `meta-intel` layer is required to provide a support for Intel Galileo
(`intel-quark`). For development only, one can use `qemux86` to speed things up
in case no dev board is accessible at the moment - [Intel Quark][1] is
a variant of x86 anyway.

Prerequisites
-------------

The project was build on Ubuntu 18.04 LTS. About 40GB of free space is required
to store all the necessary build files. Build speed is heavily influenced by
the number of cores, so the more powerful CPU the better.

Tutorial
--------

First step is to clone all the necessary files.
```
git clone -b thud git://git.yoctoproject.org/poky && cd poky
git clone -b thud git://git.openembedded.org/meta-openembedded
git clone -b thud git://git.openembedded.org/meta-intel
git clone git@github.com:maciejstanek/meta-dpgen.git
```

Next, prepare the build configuration. This includes adding all the necessary
layers and modifying a config file appropriately.
```
. oe-init-build-env build
echo 'MACHINE = "intel-quark"' >> conf/layer.conf
echo 'REFERRED_PROVIDER_virtual/kernel = "linux-yocto-rt"' >> conf/layer.conf
echo 'COMPATIBLE_MACHINE_intel-quark = "intel-quark"' >> conf/layer.conf
bitbake-layers add-layer ../meta-openembedded/meta-oe
bitbake-layers add-layer ../meta-intel
bitbake-layers add-layer ../meta-dpgen
bitbake-layers show-layers
bitbake core-image-rt
```

**TODO**: Describe SD card image generation and processing.

Troubleshooting
---------------

In case something breaks in the build process, there are a few options to
consider. Firstly, one can build it without Intel Quark meta and debug it with
an emulator.
```
echo 'MACHINE = "qemux86"' >> conf/layer.conf
echo 'COMPATIBLE_MACHINE_quemux86 = "qemux86"' >> conf/layer.conf
bitbake-layers remove-layer ../meta-intel
bitbake core-image-rt
runqemu qemux86
```

Alternatively, `dpgen` utility can be removed.
```
bitbake-layers remove-layer ../meta-dpgen
bitbake core-image-rt
```

If everything else fails, one can rebuild the whole project. To save the time
spent on downloading Yocto modules, a `bitbake` cleanup command can be used.
```
bitbake -c cleansstate world
```

The recommended development enviroment consists of two TTYs. The first one is
used for building and emulating.
```
. oe-init-build-env build
bitbake dpgen && bitbake core-image-rt && runqemu qemux86
```
The second one is used for developing the C app.

[1]: https://en.wikipedia.org/wiki/Intel_Quark
