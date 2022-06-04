SDL_image is an example portable image loading library for use with SDL.

The source code is available from: http://www.libsdl.org/projects/SDL_image

This library is distributed under the terms of the zlib license: http://www.zlib.net/zlib_license.html

This packages contains the SDL2_image.framework for OS X. Conforming with Apple guidelines, this framework contains both the SDL runtime component and development header files.

Requirements:
You must have the SDL2.framework installed.

To Install:
Copy the SDL2_image.framework to /Library/Frameworks

You may alternatively install it in <your home directory>/Library/Frameworks if your access privileges are not high enough. (Be aware that the Xcode templates we provide in the SDL Developer Extras package may require some adjustment for your system if you do this.)


Use in CMake projects:
SDL2_image.framework can be used in CMake projects using the following pattern:
```
find_package(SDL2_image REQUIRED)
add_executable(my_game ${MY_SOURCES})
target_link_libraries(my_game PRIVATE SDL2_image::SDL2_image)
```
If SDL2_image.framework is installed in a non-standard location,
please refer to the following link for ways to configure CMake:
https://cmake.org/cmake/help/latest/command/find_package.html#config-mode-search-procedure


(Partial) History of PB/Xcode projects:
2009-09-21 - Updated for 64-bit (Snow Leopard) Universal Binaries.
	Switched to 10.4 minimum requirement.
	Switched to ImageIO backend for distribution.
	Static libraries of libpng and libjpeg are no longer maintained and may eventually be removed.

2006-01-31 - First entry in history. Updated for Universal Binaries. Static libraries of libpng and libjpeg have been brought up-to-date and built as Universal.
