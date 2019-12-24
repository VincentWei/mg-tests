# Test code for DRM engine

## Introduction

This test program covers the `drm` NEWGAL engine of MiniGUI 4.0.0.
It implements a sub driver of `drm` engine for `i915` GPU, and
gives an example to exploit the GPU's accelerated rendering
capabilities:

* Allocating and freeing memory from GPU for a hardware accelerated
  MiniGUI surface.
* Filling a rectangle in the hardware surface with a solid pixel value.
* Blitting pixels on a source surface to a destination surface with
  alpha or color-key enabled.

Please make sure to install `libdrm-dev` package to your Linux system:

    $ sudo apt install libdrm-dev


## Copying

Copyright (C) 2019, Beijing FMSoft Technologies Co., Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

In particular, the above open source license does not apply to any
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

<https://www.fmsoft.cn/exception-list>

