linux-steam-integration
-----------------------

A helper shim to enable better Steam* integration on Linux systems.
This is part of an effort by Solus to enhance Steam for everyone.

This project, and by extension Solus, is not officially endorsed by, or
affiliated with, Steam*, or it's parent company, Valve*.


Integrating LSI
===============

To correctly integrate LSI, your Steam package will require modification.
LSI must provide the /usr/bin/steam binary, so your Steam package must
move the main launcher to a shadow location.

**Configuring LSI build**

There are a number of configure options you should be aware of when integrating
LSI correctly. The prominent ones are explained here.



``--with-real-steam-binary=$NAME``

        Set the absolute path for the shadowed Steam binary.
        LSI will execv Steam after it has initialised the environment.
        Note that execv, not execvp, is used, to mask programs that may be
        in the ``$PATH``, hence requiring an absolute path.

        The default value for this option is::

                /usr/lib/steam/steam

``--with-preload-libs=$LIBS``

        A colon separated list of libraries that are required to launch Steam
        when using its own runtime. LSI enables users to switch back to the Steam
        runtime, and in this instance we manage the LD_PRELOAD environmental variable.
        Ensure this is correct, and escape $LIB for correct Linux usage.

        The default value for this option is::

                /usr/\$LIB/libX11.so.6:/usr/\$LIB/libstdc++.so.6

``--enable-frontend``

        A small UI application is shipped to enable configuration of LSI, which presents
        a simple GTK3 Dialog to the user. It is not enabled by default, it is up
        to the integrator to decide if they wish to employ lsi-settings, or implement
        an alternative.

        The lsi-settings application will only ever write new configurations to the
        **user** settings file, and requires no special permissions.

        The default value for this option is::

                disabled


**How LSI Works**

LSI provides a /usr/bin/steam binary to be used in place of the existing Steam script,
which will then correctly set up the environment before swapping the process for the
Steam process.

Configuration options can be placed in an INI-style configuration file in a series
of locations, which are ordered by priority in a cascade::

        ~/.config/linux-steam-integration.conf
        /etc/linux-steam-integration.conf
        /usr/share/defaults/linux-steam-integration/linux-steam-integration.conf

The user configuration takes immediate priority. Secondly we have the system-wide
configuration for affecting all accounts, and lastly the vendor configuration,
which may be provided by the integrator.

Currently this INI file supports two options. The root section in this INI file
must be ``[Steam]``.

``use-native-runtime = $boolean``

        If set to a boolean value, (yes/true/on), the host OS's native runtime
        will be used instead of the Steam provided runtime. If this is set to
        a false boolean value (no/false/off), then the startup will be modified
        to export the relevant ``LD_PRELOAD`` and ``STEAM_RUNTIME`` settings.

        The default value of this variable is ``true``.

``force-32bit = $boolean``

        If set to a boolean value (yes/true/on), the shadowed Steam binary will
        be run via the ``linux32`` command. This will force the ``steam`` process
        and all children to believe they are running on a 32-bit system. This
        may be useful for 64-bit games that are buggy only on 64-bit.

        If this is set to a false boolean value (no/false/off), then the
        shadowed Steam binary will be directly executed. Note that on 32-bit
        OSs this setting is ignored.

        The default value of this variable is ``false``.



License
-------

``src/nica``::

        Partial import of libnica, Copyright © Intel Corporation.
        
        
        Used within the shim component to ensure it is leak free
        and as lightweight as possible, due to needing to exec the
        main Steam* launcher.
        libnica is available under the terms of the `LGPL-2.1` license.

``src/shim``::

        Copyright © 2016 Ikey Doherty

        
        linux-steam-integration is available under the terms of the `LGPL-2.1`


See `LICENSE.LGPL2.1 <LICENSE.LGPL2.1>`_ for more details


* Some names may be claimed as the property of others.
