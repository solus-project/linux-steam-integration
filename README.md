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

Additionally we apply dynamic workarounds to fix some filesystem path related bugs in Linux
ports (or even shader workarounds for Ark) through our redirect module.

With LSI, you don't need to worry any more about manually mangling your Steam installation
just to make the open source drivers work, or manually creating links and installing
unsupported libraries. LSI is designed to take care of all of this for you.

Note that LSI will not modify your Steam installation, and instead makes use of two
modules, `liblsi-redirect.so` and `liblsi-intercept.so`, to dynamically apply all of the
workarounds at runtime, which in turn is set up by the main LSI `shim` binary.

For a more in depth view of what LSI is, and how to integrate it into your distribution,
please check the [technical README document](https://github.com/solus-project/linux-steam-integration/blob/master/TECHNICAL.md).

Do note that we intend to remove the need for distributions to integrate LSI in the near future, by providing
the LSI system as a [snap](https://snapcraft.io/) package for all supported distributions.

This project, and by extension Solus, is not officially endorsed by, or affiliated with, Steam, or its parent company, Valve*.

Linux Steam Integration is a [Solus project](https://solus-project.com/)

![logo](https://build.solus-project.com/logo.png)

## Getting LSI

LSI may not yet be available for your distribution, however some community maintained repositories do exist.

 - [Arch Linux](https://aur.archlinux.org/packages/linux-steam-integration/)
 - [Fedora](https://copr.fedorainfracloud.org/coprs/alunux/linux-steam-integration/)
 - Solus uses LSI by default, just install the `steam` package.
 
## License

Copyright © 2016-2017 Ikey Doherty

linux-steam-integration is available under the terms of the `LGPL-2.1` license.

See the accompanying `LICENSE.LGPL2.1` file for more details


`data/lsi-steam.desktop`:

        This file borrows translations from the official `steam.desktop` launcher.
        These are copyright of Valve*.

* Some names may be claimed as the property of others.
