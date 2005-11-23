#=============================================================================
#          This is a Watcom makefile to build application for OS/2
#
#
#=============================================================================

object_files= showimage.obj

# Extra stuffs to pass to C compiler:
ExtraCFlags=

#
#==============================================================================
#
!include Watcom-EXE.mif

.before
    @set include=$(%os2tk)\h;$(%include);$(%sdlhome)\include;.\;

all : showimage.exe

showimage.exe: $(object_files)
    wlink @showimage.lnk

clean : .SYMBOLIC
    @if exist showimage.exe del showimage.exe
    @if exist showimage.obj del showimage.obj
    @if exist *.map del *.map
    @if exist *.res del *.res
    @if exist *.lst del *.lst
