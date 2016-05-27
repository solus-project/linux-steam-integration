linux-steam-integration
-----------------------

A helper shim to enable better Steam* integration on Linux systems.
This is part of an effort by Solus to enhance Steam for everyone.


Please check back soon when we'll have a git tag and some documentation for
integration and usage prepared. In the mean time you can check out the
[Linux Steam Integration Announcement](https://plus.google.com/u/0/+Solus-Project/posts/FxYebbR8cxk)


        This project, and by extension Solus, is not officially endorsed by, or
        affiliated with, Steam*, or it's parent company, Valve*.


License
-------

`src/nica`:
        Partial import of libnica, Copyright © Intel Corporation.
        
        
        Used within the shim component to ensure it is leak free
        and as lightweight as possible, due to needing to exec the
        main Steam* launcher.
        libnica is available under the terms of the `LGPL-2.1` license.

`src/shim`:
        Copyright © 2016 Ikey Doherty

        
        linux-steam-integration is available under the terms of the `LGPL-2.1`


See [LICENSE.LGPL2.1](LICENSE.LGPL2.1) for more details


*Some names may be claimed as the property of others.
