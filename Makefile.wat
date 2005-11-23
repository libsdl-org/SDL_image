#=============================================================================
#          This is a Watcom makefile to build SDLIMAGE.DLL for OS/2
#
#
#=============================================================================

dllname=SDLIMAGE

zlibfiles=zlib121\adler32.obj zlib121\compress.obj zlib121\crc32.obj zlib121\deflate.obj zlib121\gzio.obj zlib121\infback.obj zlib121\inffast.obj zlib121\inflate.obj zlib121\inftrees.obj zlib121\trees.obj zlib121\uncompr.obj zlib121\zutil.obj
libpngfiles=lpng125\png.obj lpng125\pngerror.obj lpng125\pnggccrd.obj lpng125\pngget.obj lpng125\pngmem.obj lpng125\pngpread.obj lpng125\pngread.obj lpng125\pngrio.obj lpng125\pngrtran.obj lpng125\pngrutil.obj lpng125\pngset.obj lpng125\pngtest.obj lpng125\pngtrans.obj lpng125\pngvcrd.obj lpng125\pngwio.obj lpng125\pngwrite.obj lpng125\pngwtran.obj lpng125\pngwutil.obj
libjpgfiles=jpeg-6b\jcapimin.obj jpeg-6b\jcapistd.obj jpeg-6b\jccoefct.obj jpeg-6b\jccolor.obj jpeg-6b\jcdctmgr.obj jpeg-6b\jchuff.obj &
        jpeg-6b\jcinit.obj jpeg-6b\jcmainct.obj jpeg-6b\jcmarker.obj jpeg-6b\jcmaster.obj jpeg-6b\jcomapi.obj jpeg-6b\jcparam.obj &
        jpeg-6b\jcphuff.obj jpeg-6b\jcprepct.obj jpeg-6b\jcsample.obj jpeg-6b\jctrans.obj jpeg-6b\jdapimin.obj jpeg-6b\jdapistd.obj &
        jpeg-6b\jdatadst.obj jpeg-6b\jdatasrc.obj jpeg-6b\jdcoefct.obj jpeg-6b\jdcolor.obj jpeg-6b\jddctmgr.obj jpeg-6b\jdhuff.obj &
        jpeg-6b\jdinput.obj jpeg-6b\jdmainct.obj jpeg-6b\jdmarker.obj jpeg-6b\jdmaster.obj jpeg-6b\jdmerge.obj jpeg-6b\jdphuff.obj &
        jpeg-6b\jdpostct.obj jpeg-6b\jdsample.obj jpeg-6b\jdtrans.obj jpeg-6b\jerror.obj jpeg-6b\jfdctflt.obj jpeg-6b\jfdctfst.obj &
        jpeg-6b\jfdctint.obj jpeg-6b\jidctflt.obj jpeg-6b\jidctfst.obj jpeg-6b\jidctint.obj jpeg-6b\jidctred.obj jpeg-6b\jquant1.obj &
        jpeg-6b\jquant2.obj jpeg-6b\jutils.obj jpeg-6b\jmemmgr.obj jpeg-6b\jmemnobs.obj
tifffiles=tiff-v3.6.1\libtiff\fax3sm_winnt.obj &
        tiff-v3.6.1\libtiff\tif_aux.obj &
        tiff-v3.6.1\libtiff\tif_close.obj &
        tiff-v3.6.1\libtiff\tif_codec.obj &
        tiff-v3.6.1\libtiff\tif_color.obj &
        tiff-v3.6.1\libtiff\tif_compress.obj &
        tiff-v3.6.1\libtiff\tif_dir.obj &
        tiff-v3.6.1\libtiff\tif_dirinfo.obj &
        tiff-v3.6.1\libtiff\tif_dirread.obj &
        tiff-v3.6.1\libtiff\tif_dirwrite.obj &
        tiff-v3.6.1\libtiff\tif_dumpmode.obj &
        tiff-v3.6.1\libtiff\tif_error.obj &
        tiff-v3.6.1\libtiff\tif_extension.obj &
        tiff-v3.6.1\libtiff\tif_fax3.obj &
        tiff-v3.6.1\libtiff\tif_flush.obj &
        tiff-v3.6.1\libtiff\tif_getimage.obj &
        tiff-v3.6.1\libtiff\tif_jpeg.obj &
        tiff-v3.6.1\libtiff\tif_luv.obj &
        tiff-v3.6.1\libtiff\tif_lzw.obj &
        tiff-v3.6.1\libtiff\tif_msdos.obj &
        tiff-v3.6.1\libtiff\tif_next.obj &
        tiff-v3.6.1\libtiff\tif_ojpeg.obj &
        tiff-v3.6.1\libtiff\tif_open.obj &
        tiff-v3.6.1\libtiff\tif_packbits.obj &
        tiff-v3.6.1\libtiff\tif_pixarlog.obj &
        tiff-v3.6.1\libtiff\tif_predict.obj &
        tiff-v3.6.1\libtiff\tif_print.obj &
        tiff-v3.6.1\libtiff\tif_read.obj &
        tiff-v3.6.1\libtiff\tif_strip.obj &
        tiff-v3.6.1\libtiff\tif_swab.obj &
        tiff-v3.6.1\libtiff\tif_thunder.obj &
        tiff-v3.6.1\libtiff\tif_tile.obj &
        tiff-v3.6.1\libtiff\tif_version.obj &
        tiff-v3.6.1\libtiff\tif_warning.obj &
        tiff-v3.6.1\libtiff\tif_write.obj &
        tiff-v3.6.1\libtiff\tif_zip.obj

object_files= IMG.obj IMG_bmp.obj IMG_gif.obj IMG_jpg.obj IMG_lbm.obj IMG_pcx.obj IMG_png.obj IMG_pnm.obj IMG_tga.obj IMG_tif.obj IMG_xcf.obj IMG_xpm.obj IMG_xxx.obj $(zlibfiles) $(libpngfiles) $(tifffiles)

# Extra stuffs to pass to C compiler:
ExtraCFlags=-DLOAD_BMP -DLOAD_GIF -DLOAD_JPG -DLOAD_LBM -DLOAD_PCX -DLOAD_PNG -DLOAD_PNM -DLOAD_XCF -DLOAD_XPM -DLOAD_XXX -DLOAD_TGA -DLOAD_TIF

#
#==============================================================================
#
!include Watcom.mif

.before
    @set include=$(%os2tk)\h;$(%include);$(%sdlhome)\include;.\;.\jpeg-6b;.\lpng125;.\zlib121;.\tiff-v3.6.1\libtiff;

all : $(dllname).dll $(dllname).lib

$(dllname).dll : $(object_files)
    wlink @$(dllname)

$(dllname).lib : $(dllname).dll
    implib $(dllname).lib $(dllname).dll

clean : .SYMBOLIC
    @if exist $(dllname).dll del $(dllname).dll
    @if exist *.lib del *.lib
    @if exist *.obj del *.obj
    @if exist *.map del *.map
    @if exist *.res del *.res
    @if exist *.lst del *.lst
