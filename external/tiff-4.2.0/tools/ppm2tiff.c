/*
 * Copyright (c) 1991-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

#include "tif_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_IO_H
# include <io.h>
#endif

#ifdef NEED_LIBPORT
# include "libport.h"
#endif

#include "tiffio.h"

#ifndef HAVE_GETOPT
extern int getopt(int argc, char * const argv[], const char *optstring);
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#define	streq(a,b)	(strcmp(a,b) == 0)
#define	strneq(a,b,n)	(strncmp(a,b,n) == 0)

static	uint16 compression = COMPRESSION_PACKBITS;
static	uint16 predictor = 0;
static	int quality = 75;	/* JPEG quality */
static	int jpegcolormode = JPEGCOLORMODE_RGB;
static  uint32 g3opts;

static	void usage(int code);
static	int processCompressOptions(char*);

static void
pack_none (unsigned char *buf, unsigned int smpls, uint16 bps)
{
	(void)buf;
	(void)smpls;
	(void)bps;
	return;
}

static void
pack_swab (unsigned char *buf, unsigned int smpls, uint16 bps)
{
	unsigned int s;
	unsigned char h;
	unsigned char l;
	(void)bps;

	for (s = 0; smpls > s; s++) {

		h = buf [s * 2 + 0];
		l = buf [s * 2 + 1];

		buf [s * 2 + 0] = l;
		buf [s * 2 + 1] = h;
	}
	return;
}

static void
pack_bytes (unsigned char *buf, unsigned int smpls, uint16 bps)
{
	unsigned int s;
	unsigned int in;
	unsigned int out;
	int bits;
	uint16 t;

	in   = 0;
	out  = 0;
	bits = 0;
	t    = 0;

	for (s = 0; smpls > s; s++) {

		t <<= bps;
		t |= (uint16) buf [in++];

		bits += bps;

		if (8 <= bits) {
			bits -= 8;
			buf [out++] = (t >> bits) & 0xFF;
		}
	}
	if (0 != bits)
		buf [out] = (t << (8 - bits)) & 0xFF;
}

static void
pack_words (unsigned char *buf, unsigned int smpls, uint16 bps)
{
	unsigned int s;
	unsigned int in;
	unsigned int out;
	int bits;
	uint32 t;

	in   = 0;
	out  = 0;
	bits = 0;
	t    = 0;

	for (s = 0; smpls > s; s++) {

		t <<= bps;
		t |= (uint32) buf [in++] << 8;
		t |= (uint32) buf [in++] << 0;

		bits += bps;

		if (16 <= bits) {

			bits -= 16;
			buf [out++] = (t >> (bits + 8));
			buf [out++] = (t >> (bits + 0));
		}
	}
	if (0 != bits) {
		t <<= 16 - bits;

		buf [out++] = (t >> (16 + 8));
		buf [out++] = (t >> (16 + 0));
	}
}

static void
BadPPM(char* file)
{
	fprintf(stderr, "%s: Not a PPM file.\n", file);
	exit(EXIT_FAILURE);
}


#define TIFF_SIZE_T_MAX ((size_t) ~ ((size_t)0))
#define TIFF_TMSIZE_T_MAX (tmsize_t)(TIFF_SIZE_T_MAX >> 1)

static tmsize_t
multiply_ms(tmsize_t m1, tmsize_t m2)
{
        if( m1 == 0 || m2 > TIFF_TMSIZE_T_MAX / m1 )
            return 0;
        return m1 * m2;
}

int
main(int argc, char* argv[])
{
	uint16 photometric = 0;
	uint32 rowsperstrip = (uint32) -1;
	double resolution = -1;
	unsigned char *buf = NULL;
	tmsize_t linebytes = 0;
	int pbm;
	uint16 spp = 1;
	uint16 bpp = 8;
	void (*pack_func) (unsigned char *buf, unsigned int smpls, uint16 bps);
	TIFF *out;
	FILE *in;
	unsigned int w, h, prec, row;
	char *infile;
	int c;
#if !HAVE_DECL_OPTARG
	extern int optind;
	extern char* optarg;
#endif
	tmsize_t scanline_size;

	if (argc < 2) {
	    fprintf(stderr, "%s: Too few arguments\n", argv[0]);
	    usage(EXIT_FAILURE);
	}
	while ((c = getopt(argc, argv, "c:r:R:h")) != -1)
		switch (c) {
		case 'c':		/* compression scheme */
			if (!processCompressOptions(optarg))
				usage(EXIT_FAILURE);
			break;
		case 'r':		/* rows/strip */
			rowsperstrip = atoi(optarg);
			break;
		case 'R':		/* resolution */
			resolution = atof(optarg);
			break;
		case 'h':
			usage(EXIT_SUCCESS);
		case '?':
			usage(EXIT_FAILURE);
			/*NOTREACHED*/
		}

	if (optind + 2 < argc) {
	    fprintf(stderr, "%s: Too many arguments\n", argv[0]);
	    usage(EXIT_FAILURE);
	}

	/*
	 * If only one file is specified, read input from
	 * stdin; otherwise usage is: ppm2tiff input output.
	 */
	if (argc - optind > 1) {
		infile = argv[optind++];
		in = fopen(infile, "rb");
		if (in == NULL) {
			fprintf(stderr, "%s: Can not open.\n", infile);
			return (EXIT_FAILURE);
		}
	} else {
		infile = "<stdin>";
		in = stdin;
#if defined(HAVE_SETMODE) && defined(O_BINARY)
		setmode(fileno(stdin), O_BINARY);
#endif
	}

	if (fgetc(in) != 'P')
		BadPPM(infile);
	switch (fgetc(in)) {
		case '4':			/* it's a PBM file */
			pbm = !0;
			spp = 1;
			photometric = PHOTOMETRIC_MINISWHITE;
			break;
		case '5':			/* it's a PGM file */
			pbm = 0;
			spp = 1;
			photometric = PHOTOMETRIC_MINISBLACK;
			break;
		case '6':			/* it's a PPM file */
			pbm = 0;
			spp = 3;
			photometric = PHOTOMETRIC_RGB;
			if (compression == COMPRESSION_JPEG &&
			    jpegcolormode == JPEGCOLORMODE_RGB)
				photometric = PHOTOMETRIC_YCBCR;
			break;
		default:
			BadPPM(infile);
	}

	/* Parse header */
	while(1) {
		if (feof(in))
			BadPPM(infile);
		c = fgetc(in);
		/* Skip whitespaces (blanks, TABs, CRs, LFs) */
		if (strchr(" \t\r\n", c))
			continue;

		/* Check for comment line */
		if (c == '#') {
			do {
			    c = fgetc(in);
			} while(!(strchr("\r\n", c) || feof(in)));
			continue;
		}

		ungetc(c, in);
		break;
	}
	if (pbm) {
		if (fscanf(in, " %u %u", &w, &h) != 2)
			BadPPM(infile);
		if (fgetc(in) != '\n')
			BadPPM(infile);
		bpp = 1;
		pack_func = pack_none;
	} else {
		if (fscanf(in, " %u %u %u", &w, &h, &prec) != 3)
			BadPPM(infile);
		if (fgetc(in) != '\n' || 0 == prec || 65535 < prec)
			BadPPM(infile);

		if (0 != (prec & (prec + 1))) {
			fprintf(stderr, "%s: unsupported maxval %u.\n",
				infile, prec);
			exit(EXIT_FAILURE);
		}
		bpp = 0;
		if ((prec + 1) & 0xAAAAAAAA) bpp |=  1;
		if ((prec + 1) & 0xCCCCCCCC) bpp |=  2;
		if ((prec + 1) & 0xF0F0F0F0) bpp |=  4;
		if ((prec + 1) & 0xFF00FF00) bpp |=  8;
		if ((prec + 1) & 0xFFFF0000) bpp |= 16;

		switch (bpp) {
		case 8:
			pack_func = pack_none;
			break;
		case 16:
			{
				const unsigned short i = 0x0100;

				if (0 == *(unsigned char*) &i)
					pack_func = pack_swab;
				else
					pack_func = pack_none;
			}
			break;
		default:
			if (8 >= bpp)
				pack_func = pack_bytes;
			else
				pack_func = pack_words;
			break;
		}
	}
	out = TIFFOpen(argv[optind], "w");
	if (out == NULL)
		return (EXIT_FAILURE);
	TIFFSetField(out, TIFFTAG_IMAGEWIDTH, (uint32) w);
	TIFFSetField(out, TIFFTAG_IMAGELENGTH, (uint32) h);
	TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, spp);
	TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bpp);
	TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photometric);
	TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
	switch (compression) {
	case COMPRESSION_JPEG:
		TIFFSetField(out, TIFFTAG_JPEGQUALITY, quality);
		TIFFSetField(out, TIFFTAG_JPEGCOLORMODE, jpegcolormode);
		break;
	case COMPRESSION_LZW:
	case COMPRESSION_DEFLATE:
		if (predictor != 0)
			TIFFSetField(out, TIFFTAG_PREDICTOR, predictor);
		break;
        case COMPRESSION_CCITTFAX3:
		TIFFSetField(out, TIFFTAG_GROUP3OPTIONS, g3opts);
		break;
	}
	if (pbm) {
		/* if round-up overflows, result will be zero, OK */
		linebytes = (multiply_ms(spp, w) + (8 - 1)) / 8;
	} else if (bpp <= 8) {
		linebytes = multiply_ms(spp, w);
	} else {
		linebytes = multiply_ms(2 * spp, w);
	}
	if (rowsperstrip == (uint32) -1) {
		TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, h);
	} else {
		TIFFSetField(out, TIFFTAG_ROWSPERSTRIP,
		    TIFFDefaultStripSize(out, rowsperstrip));
	}
	if (linebytes == 0) {
		fprintf(stderr, "%s: scanline size overflow\n", infile);
		(void) TIFFClose(out);
		exit(EXIT_FAILURE);
	}
	scanline_size = TIFFScanlineSize(out);
	if (scanline_size == 0) {
		/* overflow - TIFFScanlineSize already printed a message */
		(void) TIFFClose(out);
		exit(EXIT_FAILURE);
	}
	if (scanline_size < linebytes)
		buf = (unsigned char *)_TIFFmalloc(linebytes);
	else
		buf = (unsigned char *)_TIFFmalloc(scanline_size);
	if (buf == NULL) {
		fprintf(stderr, "%s: Not enough memory\n", infile);
		(void) TIFFClose(out);
		exit(EXIT_FAILURE);
	}
	if (resolution > 0) {
		TIFFSetField(out, TIFFTAG_XRESOLUTION, resolution);
		TIFFSetField(out, TIFFTAG_YRESOLUTION, resolution);
		TIFFSetField(out, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
	}
	for (row = 0; row < h; row++) {
		if (fread(buf, linebytes, 1, in) != 1) {
			fprintf(stderr, "%s: scanline %lu: Read error.\n",
			    infile, (unsigned long) row);
			break;
		}
		pack_func (buf, w * spp, bpp);
		if (TIFFWriteScanline(out, buf, row, 0) < 0)
			break;
	}
	if (in != stdin)
		fclose(in);
	(void) TIFFClose(out);
	if (buf)
		_TIFFfree(buf);
	return (EXIT_SUCCESS);
}

static void
processG3Options(char* cp)
{
	g3opts = 0;
        if( (cp = strchr(cp, ':')) ) {
                do {
                        cp++;
                        if (strneq(cp, "1d", 2))
                                g3opts &= ~GROUP3OPT_2DENCODING;
                        else if (strneq(cp, "2d", 2))
                                g3opts |= GROUP3OPT_2DENCODING;
                        else if (strneq(cp, "fill", 4))
                                g3opts |= GROUP3OPT_FILLBITS;
                        else
                                usage(EXIT_FAILURE);
                } while( (cp = strchr(cp, ':')) );
        }
}

static int
processCompressOptions(char* opt)
{
	if (streq(opt, "none"))
		compression = COMPRESSION_NONE;
	else if (streq(opt, "packbits"))
		compression = COMPRESSION_PACKBITS;
	else if (strneq(opt, "jpeg", 4)) {
		char* cp = strchr(opt, ':');

                compression = COMPRESSION_JPEG;
                while (cp)
                {
                    if (isdigit((int)cp[1]))
			quality = atoi(cp+1);
                    else if (cp[1] == 'r' )
			jpegcolormode = JPEGCOLORMODE_RAW;
                    else
                        usage(EXIT_FAILURE);

                    cp = strchr(cp+1,':');
                }
	} else if (strneq(opt, "g3", 2)) {
		processG3Options(opt);
		compression = COMPRESSION_CCITTFAX3;
	} else if (streq(opt, "g4")) {
		compression = COMPRESSION_CCITTFAX4;
	} else if (strneq(opt, "lzw", 3)) {
		char* cp = strchr(opt, ':');
		if (cp)
			predictor = atoi(cp+1);
		compression = COMPRESSION_LZW;
	} else if (strneq(opt, "zip", 3)) {
		char* cp = strchr(opt, ':');
		if (cp)
			predictor = atoi(cp+1);
		compression = COMPRESSION_DEFLATE;
	} else
		return (0);
	return (1);
}

const char* stuff[] = {
"usage: ppm2tiff [options] input.ppm output.tif",
"where options are:",
" -r #		make each strip have no more than # rows",
" -R #		set x&y resolution (dpi)",
"",
" -c jpeg[:opts]  compress output with JPEG encoding",
" -c lzw[:opts]	compress output with Lempel-Ziv & Welch encoding",
" -c zip[:opts]	compress output with deflate encoding",
" -c packbits	compress output with packbits encoding (the default)",
" -c g3[:opts]  compress output with CCITT Group 3 encoding",
" -c g4         compress output with CCITT Group 4 encoding",
" -c none	use no compression algorithm on output",
"",
"JPEG options:",
" #		set compression quality level (0-100, default 75)",
" r		output color image as RGB rather than YCbCr",
"LZW and deflate options:",
" #		set predictor value",
"For example, -c lzw:2 to get LZW-encoded data with horizontal differencing",
NULL
};

static void
usage(int code)
{
	int i;
	FILE * out = (code == EXIT_SUCCESS) ? stdout : stderr;

        fprintf(out, "%s\n\n", TIFFGetVersion());
	for (i = 0; stuff[i] != NULL; i++)
		fprintf(out, "%s\n", stuff[i]);
	exit(code);
}

/* vim: set ts=8 sts=8 sw=8 noet: */
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
