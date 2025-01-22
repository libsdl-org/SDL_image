
# Introduction to SDL_image with Visual Studio

The easiest way to use SDL_image is to include it as a subproject in your project.

We'll start by creating a simple project to build and run [hello.c](hello.c)

- Create a new project in Visual Studio, using the C++ Empty Project template
- Add hello.c to the Source Files
- Right click the solution, select add an existing project, navigate to SDL VisualC/SDL and add SDL.vcxproj
- Right click the solution, select add an existing project, navigate to SDL_image VisualC and add SDL_image.vcxproj
- Select your main project and go to Project -> Project Dependencies and select SDL3
- Select your main project and go to Project -> Properties, set the filter at the top to "All Configurations" and "All Platforms", select VC++ Directories and add the SDL include directory to "Include Directories"
- Select your main project and go to Project -> Add Reference and select SDL3 and SDL3_image
- Build and run!

Support for AVIF, JPEG-XL, TIFF, and WebP are not included by default because of the size of the decode libraries, but you can get them by running external/download.sh, and then building the libraries and adding the appropriate LOAD_* preprocessor define to the SDL_image Visual Studio project.
