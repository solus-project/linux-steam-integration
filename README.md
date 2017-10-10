linux-steam-integration
-----------------------

Linux Steam Integration attempts to improve the Steam* gaming experience for users on Linux. In the default mode, it will disable the Steam runtime, and facilitate usage of the distro's own runtime for improved integration and performance. The Steam Runtime is a very old set of Ubuntu libraries, and as such provides a suboptimial experience for many Linux users. A key example of this is broken fontconfig, C++ ABI issues preventing loading, or even the broken theming of GTK widgets on non-Ubuntu distros. These woes have been compounded for users of the open source drivers.

The LSI tool also allows you to use Steam's runtime if you need to, as currently not all cases are covered. It will also take care to prepare the environment, ensuring that users can load Steam using it's own runtime without hitting any of the issues common to a modern distro, i.e. ABI issues and having to `LD_PRELOAD` libraries to even make Steam start.

More importantly, LSI achieves all of this by not having to **butcher your existing Steam installation**, as is common in some new tools.

This project, and by extension Solus, is not officially endorsed by, or affiliated with, Steam*, or its parent company, Valve*.


Linux Steam Integration is a [Solus project](https://solus-project.com/)

![logo](https://build.solus-project.com/logo.png)


Integrating LSI
===============

To correctly integrate LSI, your Steam package will require modification. LSI must provide the /usr/bin/steam binary, so your Steam package must move the main launcher to a shadow location.

For users who do not have access to LSI in their distribution, because it has not been integrated yet, please configure with `-Dwith-shim=co-exist` to preserve your existing Steam integrity. Then use the "LSI Steam" shortcut in your menu to launch Steam via LSI.

**Configuring LSI build**

There are a number of meson configure options you should be aware of when integrating LSI correctly. The prominent ones are explained here.

`-Dwith-shim= none | co-exist | replacement`
`--disable-replace-steam`

        Control behaviour of the shim build. `none` will disable building the main
        shim, required for actually launching Steam in the first place.
        
        `co-exist` will ensure this does not conflict with the `/usr/bin/steam`
        path, providing a new `lsi-steam` binary and desktop file.

        `replacement` will provide a drop-in replacement `/usr/bin/steam` binary
        shim responsible for bootstrapping Steam via LSI. This will require you
        to mask the original steam binary to a new location.

        The default value for this option is::

                replacement


`-Dwith-steam-binary=$PATH`

        Set the absolute path for the shadowed Steam binary.
        LSI will execv Steam after it has initialised the environment.
        Note that execv, not execvp, is used, to mask programs that may be
        in the `$PATH`, hence requiring an absolute path.

        The default value for this option is::

                /usr/lib/steam/steam

        This option is only applicable for distribution integrators, when
        Steam replacement is enabled.


`-Dwith-preload-libs=$LIBS`

        A colon separated list of libraries that are required to launch Steam
        when using its own runtime. LSI enables users to switch back to the Steam
        runtime, and in this instance we manage the LD_PRELOAD environmental variable.
        Ensure this is correct, and escape $LIB for correct Linux usage.

        The default value for this option is::

                /usr/\$LIB/libX11.so.6:/usr/\$LIB/libstdc++.so.6

`-Dwith-frontend=$boolean`

        A small UI application is shipped to enable configuration of LSI, which presents
        a simple GTK3 Dialog to the user. It is not enabled by default, it is up
        to the integrator to decide if they wish to employ lsi-settings, or implement
        an alternative.

        The lsi-settings application will only ever write new configurations to the
        **user** settings file, and requires no special permissions.

        The default value for this option is::

                false


**How LSI Works**

LSI provides a /usr/bin/steam binary to be used in place of the existing Steam script, which will then correctly set up the environment before swapping the process for the Steam process.

Configuration options can be placed in an INI-style configuration file in a series of locations, which are ordered by priority in a cascade::

        ~/.config/linux-steam-integration.conf
        /etc/linux-steam-integration.conf
        /usr/share/defaults/linux-steam-integration/linux-steam-integration.conf

The user configuration takes immediate priority. Secondly we have the system-wide configuration for affecting all accounts, and lastly the vendor configuration, which may be provided by the integrator.

Currently this INI file supports two options. The root section in this INI file must be `[Steam]`.

`use-native-runtime = $boolean`

        If set to a true boolean value, (yes/true/on), the host OS's native runtime
        will be used instead of the Steam provided runtime. If this is set to
        a false boolean value (no/false/off), then the startup will be modified
        to export the relevant `LD_PRELOAD` and `STEAM_RUNTIME` settings.

        The default value of this variable is true.

`force-32bit = $boolean`

        If set to a true boolean value (yes/true/on), the shadowed Steam binary will
        be run via the `linux32` command. This will force the `steam` process
        and all children to believe they are running on a 32-bit system. This
        may be useful for 64-bit games that are buggy only on 64-bit.

        If this is set to a false boolean value (no/false/off), then the
        shadowed Steam binary will be directly executed. Note that on 32-bit
        OSs this setting is ignored.

        The default value of this variable is false.


Common issues
=============

**Missing tray icon for Steam client using native OS runtime**

Ensure you have the 32-bit version of libappindicator installed. This is required for the tray icon. libappindicator falls back to a standard X11 tray icon in the absence of desktop appindicator support.

Related issue: [Steam tray icon missing #2](https://github.com/solus-project/linux-steam-integration/issues/2)


License
-------

`src/shim src/frontend src/lsi src/intercept`:

        Copyright © 2016-2017 Ikey Doherty

        linux-steam-integration is available under the terms of the `LGPL-2.1`

`data/lsi-steam.desktop`:

        This file borrows translations from the official `steam.desktop` launcher.
        These are copyright of Valve*.

See `LICENSE.LGPL2.1 <LICENSE.LGPL2.1>`_ for more details


* Some names may be claimed as the property of others.
