
# Introduction to SDL_image with Xcode

The easiest way to use SDL_image is to include it along with SDL as subprojects in your project.

We'll start by creating a simple project to build and run [hello.c](hello.c)

- Create a new project in Xcode, using the App template and selecting Objective C as the language
- Remove the .h and .m files that were automatically added to the project
- Remove the main storyboard that was automatically added to the project
- On iOS projects, select the project, select the main target, select the Info tab, look for "Custom iOS Target Properties", and remove "Main storyboard base file name" and "Application Scene Manifest"
- Right click the project and select "Add Files to [project]", navigate to the SDL_image docs directory and add the file hello.c
- Right click the project and select "Add Files to [project]", navigate to the SDL Xcode/SDL directory and add SDL.xcodeproj
- Right click the project and select "Add Files to [project]", navigate to the SDL_image Xcode directory and add SDL_image.xcodeproj
- Select the project, select the main target, select the General tab, look for "Frameworks, Libaries, and Embedded Content", and add SDL3.framework and SDL3_image.framework
- Build and run!

Support for AVIF, JPEG-XL, and WebP are not included by default because of the size of the decode libraries, but you can get them by running external/download.sh and editing the config at the top of the Xcode project to enable them. You will need to include the appropriate framework in your application.

