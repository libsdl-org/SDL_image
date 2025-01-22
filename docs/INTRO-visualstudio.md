
# Introduction to SDL_image with Visual Studio

The easiest way to use SDL_image is to include it along with SDL as subprojects in your project.

We'll start by creating a simple project to build and run [hello.c](hello.c)

- Create a new project in Visual Studio, using the C++ Empty Project template
- Add hello.c to the Source Files
- Right click the solution, select add an existing project, navigate to the SDL VisualC/SDL directory and add SDL.vcxproj
- Right click the solution, select add an existing project, navigate to the SDL_image VisualC directory and add SDL_image.vcxproj
- Select your main project and go to Project -> Add Reference and select SDL3 and SDL3_image
- Select your main project and go to Project -> Properties, set the filter at the top to "All Configurations" and "All Platforms", select VC++ Directories and add the SDL and SDL_image include directories to "Include Directories"
- Build and run!

Support for AVIF, TIFF, and WebP are dynamically loaded at runtime. You can choose to include those DLLs and license files with your application if you want support for those formats, or omit them if they are not needed.

Support for JPEG-XL is not included by default because of the size of the decode library, but you can get it by running external/Get-GitModules.ps1, and then building libjxl and adding the LOAD_JXL preprocessor define to the SDL_image Visual Studio project.
