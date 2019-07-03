# Test code for MiniGUI APIs.

Since MiniGUI 4.0.0, we organize the unit test code of MiniGUI APIs
in this repository.

## Prerequisites

* Always run the test programs on Ubuntu Linux 16.04 LTS or later.
* Configure MiniGUI as MiniGUI-Threads runmode.
* Install the electric fence package:

    $ sudo apt install electric-fence

## Building

    $ ./autogen.sh
    $ ./configure
    $ make

## Copying

Copyright (C) 2018 ~ 2019, Beijing FMSoft Technologies Co., Ltd.

Most programs of mg-tests are licensed under the Apache License,
Version 2.0. Some programs of mg-tests are licensed under GPL,
because the programs uses some code from other GPL'd programs.

For more information, please see the `README.md` file in individual
directories.

In particular, the above open source license(s) does not apply to any
entity in the Exception List published by
Beijing FMSoft Technologies Co., Ltd.

If you are or the entity you represent is listed in the Exception List,
the above open source license does not apply to you or the entity
you represent. Regardless of the purpose, you should not use the
software in any way whatsoever, including but not limited to downloading,
viewing, copying, distributing, compiling, and running. If you have
already downloaded it, you MUST destroy all of its copies.

The Exception List is published by FMSoft
in the following webpage and may be updated from time to time:

https://www.fmsoft.cn/exception-list

