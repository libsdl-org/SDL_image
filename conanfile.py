from conans import ConanFile


class Sdl2Ttf(ConanFile):
    def build_requirements(self):
        self.build_requires("autoconf/2.71")
        self.build_requires("libtool/2.4.6")

    def build(self):
        self.run("./autogen.sh")
