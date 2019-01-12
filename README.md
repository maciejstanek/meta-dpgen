Digital Pattern Generator
=========================

This is a layer for Yocto Project which provides a simple digital pattern
generator (`dpgen`) for Intel Galileo.

Dependencies
------------

Aside from all the `poky` stuff, this layers depends on
`meta-openembedded/meta-oe` which provides `mraa` package. This setup was based
on `thud` branch.

Moreover, `meta-intel` layer is required to provide a support for Intel Galileo
(`intel-quark`). For development only, one can use `qemux86` to speed things up
in case no dev board is accessible at the moment - [Intel Quark][1] is a variant of x86
anyway.

Prerequisites
-------------

The project was build on Ubuntu 18.04 LTS. About 50GB of free space is required
to store all the necessary build files.

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

*TODO*: Describe SD card image generation and processing.

Troubleshooting
---------------

In case something breaks in the build process, there are a options to consider.
Firstly, one can build it without Intel Quark meta and debug it on `qemu`.
```
echo 'MACHINE = "qemux86"' >> conf/layer.conf
echo 'COMPATIBLE_MACHINE_quemux86 = "qemux86"' >> conf/layer.conf
bitbake-layers remove-layer ../meta-intel
runqemu qemux86
```

Alternatively, `dpgen` utility can be removed.
```
bitbake-layers remove-layer ../meta-dpgen
```

[1]: https://en.wikipedia.org/wiki/Intel_Quark
