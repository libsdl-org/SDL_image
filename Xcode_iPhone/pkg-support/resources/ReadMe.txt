SDL_image is an example portable image loading library for use with SDL.

The source code is available from: http://www.libsdl.org/projects/SDL_image

This library is distributed under the terms of the GNU LGPL license: http://www.gnu.org/copyleft/lesser.html
Non-LGPL licenses are available.


Requirements:
This currently looks for headers from the (Mac) SDL.framework in */Library/Frameworks
(Yes, a little hack-ish for now.)


Usage:
Apple forbids dynamic linking on iPhone OS so you must build SDL_image directly into your app or 
link against a static library (setup in our Xcode project).

Please understand the license ramifications of the LGPL when doing this.




(Partial) History of PB/Xcode projects:
2009-10-07 - Switched to UIImage backend.

2006-01-31 - First entry in history. Updated for Universal Binaries. Static libraries of libpng and libjpeg have been brought up-to-date and built as Universal.
