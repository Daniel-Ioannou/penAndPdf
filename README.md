## ABOUT

Pen&PDF is a PDF viewer and annotation app for Android built on top of MuPDF.

Pen&PDF intents to makes editing PDFs as easy as writing on a piece of paper.

## LICENSE

```
Pen&PDF is Copyright 2015-2016 Christian Gogolin

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU Affero General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.
```

## COMPILING

If you are compiling from source you will need several third party libraries:
freetype2, jbig2dec, libjpeg, openjpeg, and zlib. These libraries are contained
in the source archive. If you are using git, they are included as git
submodules.

You will also need the X11 headers and libraries if you're building on Linux.
These can typically be found in the xorg-dev package. Alternatively, if you
only want the command line tools, you can build with HAVE_X11=no.

The new OpenGL-based viewer also needs OpenGL headers and libraries. If you're
building on Linux, install the mesa-common-dev and libgl1-mesa-dev packages.
You'll also need several X11 development packages: xorg-dev, libxcursor-dev,
libxrandr-dev, and libxinerama-dev. To skip building the OpenGL viewer, build
with HAVE_GLFW=no.

## DOWNLOAD

The latest development source is available directly from the git repository:

	git clone https://github.com/cgogolin/penandpdf.git

In the penandpdf directory, update the third party libraries:

	git submodule update --init

REPORTING BUGS AND PROBLEMS

Pen&PDF uses the GitHub issue tracker on:

    https://github.com/cgogolin/penandpdf/issues

In case your problem is directly related to MuPDF, please consider the following:

The MuPDF developers hang out on IRC in the #ghostscript channel on
irc.freenode.net.

Report bugs on the ghostscript bugzilla, with MuPDF as the selected component.

	http://bugs.ghostscript.com/

If you are reporting a problem with PDF parsing, please include the problematic
file as an attachment.
