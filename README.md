linux-steam-integration
-----------------------

Linux Steam* Integration is a helper system to make the Steam Client and Steam
games run better on Linux. In a nutshell, LSI automatically applies various workarounds
to get games working, and fixes long standing bugs in both games and the client.

![screenshot](https://raw.githubusercontent.com/solus-project/linux-steam-integration/master/.github/LSI_Settings.png)

In many cases this will involve controlling which libraries are allowed to be used
at any given time, and these libraries may be overriden for any of the following
reasons:

 - Security
 - Compatibility
 - Performance


This project, and by extension Solus, is not officially endorsed by, or affiliated with, Steam, or its parent company, Valve*.

Linux Steam Integration is a [Solus project](https://getsol.us/)

![logo](https://build.solus-project.com/logo.png)

### Linking compatibility

With LSI, you don't need to worry any more about manually mangling your Steam installation
just to make the open source drivers work, or manually creating links and installing
unsupported libraries. LSI is designed to take care of all of this for you.

Many library names are redirected through the main "intercept" module, which ensures
games will (where appropriate) use the updated system libraries. Additionally the
module can override how games and the Steam client are allowed to make use of
vendored libraries. This will help with many launch failures involving outdated
libraries, or indeed the infamous `libstdc++.so.6` vendoring which breaks open
source graphics drivers on systems compiled with the new GCC C++ ABI.


### Apply path based hotfixes to games

The redirect module contains some profiles to allow us to dynamically fix some
issues that would otherwise require new builds of the games to see those issues
resolved.

 - Project Highrise: Ensure we don't `mmap` a directory as a file (fixes invalid prefs path)
 - ARK Survival Evolved: Use the correct shader asset from TheCenter DLC to fix broken water surfaces.

### Unity3D Black Screen Of Nope

Older builds of Unity3D had (long since fixed) issues with launching to a black
screen when defaulting to full screen mode. This is commonly addressed by launching the
games with `-screen-fullscreen 0`, and is due to an invalid internal condition clamping
the renderer size to 0x0 after setting the fullscreen (borderless) window size.

Note - updating these games to newer versions of Unity will fix this bug on Linux, however
LSI currently ships a workaround. This workaround will abstract access to the configuration
file in `$XDG_CONFIG_HOME/.config/unity3d/*/prefs` through the Linux `/dev/shm` system,
and will provide initial game configuration whilst also masking the harmful fullscreen
setting.

Net result - all Unity3D games using this pref path (the older generation) will start
in windowed mode always. They can be fullscreened from inside the game, and this will
help with making sure games **actually launch**.

### Notes

Note that LSI will not modify your Steam installation, and instead makes use of two
modules, `liblsi-redirect.so` and `liblsi-intercept.so`, to dynamically apply all of the
workarounds at runtime, which in turn is set up by the main LSI `shim` binary.

For a more in depth view of what LSI is, and how to integrate it into your distribution,
please check the [technical README document](https://github.com/solus-project/linux-steam-integration/blob/master/TECHNICAL.md).

Be advised that we intend to remove the need for distributions to integrate LSI in the near future, by providing
the LSI system as a [snap](https://snapcraft.io/) package for all supported distributions.

## Getting LSI

LSI may not yet be available for your distribution, however some community maintained repositories do exist.

 - [Arch Linux](https://aur.archlinux.org/packages/linux-steam-integration/)
 - [Fedora](https://copr.fedorainfracloud.org/coprs/alunux/linux-steam-integration/)
 - Solus uses LSI by default, just install the `steam` package.

To install the experimental snaps (`snapd` >= 2.29.4):

    sudo snap install --edge solus-runtime-gaming
    sudo snap install --devmode --edge linux-steam-integration

## Snap Support

LSI is currently undergoing work to become a universal package for all Linux distributions!

Items left to implement:

 - [x] Add basic library support for snapd environment
 - [x] Make `shim` support `$SNAP` style environmental variables for local Steam
 - [x] Add support to LSI + `snapd` for NVIDIA+Vulkan
 - [ ] Verify "normal" games work again
 - [x] Publish snaps (edge)


## License

Copyright © 2016-2017 Ikey Doherty

linux-steam-integration is available under the terms of the `LGPL-2.1` license.

See the accompanying `LICENSE.LGPL2.1` file for more details


`data/lsi-steam.desktop`:

        This file borrows translations from the official `steam.desktop` launcher.
        These are copyright of Valve*.

`* Some names may be claimed as the property of others.`
