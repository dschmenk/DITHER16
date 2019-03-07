/****************************************************************************\
*                                                                            *
* Dither RGB color to 4 bit planar brush using the standard EGA/VGA colors.  *
*                                                                            *
* The EGA/VGA palette is constructed to map:                                 *
*                                                                            *
*   Plane 0: Blue                                                            *
*   Plane 1: Green                                                           *
*   Plane 2: Red                                                             *
*   Plane 3: Brightness                                                      *
*                                                                            *
* The colors are mapped so they can be additive: the same bit set in plane 0 *
* as in plane 1 will combine to make Cyan (Blue + Green = Cyan). If the same *
* bit is set in plane 2, then White will result (Blue + Green + Red = White).*
* Plane 3 is mapped to create the high intensity versions of the 8 color     *
* combinations.  This makes a simple dither algorithm a little more difficult*
* but manageable when the normal intensity values are seperated from the high*
* intensity values. Thinking of the mapping as a HSV cone clarifies the      *
* process. If V is less than 50% (128), building a simple dithered brush by  *
* scaling the RGB values from 0-127 to 0-15 is sufficient. The Brightness    *
* value is set to 0. For values of V greater than 50%, it gets a little more *
* interesting. The idea is to scale the RGB values to percentages of V. Note *
* that V = MAX(R,G,B) so by dividing each by V will fill the brush with the  *
* percentage of the RGB color. Now, the Brightness can be set to V. However, *
* a little trick here is to reverse the order of the dither pattern so that  *
* the most significant color is brightened first. If the same dither pattern *
* was used as the RGB color values, the lesser colors would be brighted as   *
* well.                                                                      *
*                                                                            *
* I can see two issues with this algorithm:                                  *
*                                                                            *
*   + The colors are symmetrical around 128 leading to a double mapping of   *
*     brushes. 0x70-0x7F look suspiciously like 0x80-0x8F. This is actually  *
*     the solid colors mapped to the palette so I don't think it's a         *
*     horrible problem, but you can see it in certain smooth gradients.      *
*                                                                            *
*   + Palette entry 0x08 doesn't get used in building the brushes. This is   *
*     the equivalent of Bright Black. The Windows driver doesn't use it      *
*     either for the dithered brush. However, it is used for the "best match"*
*     when a solid color value is needed. So I added a "best match" to this  *
*     brush building routine and return that as a value, too.                *
*                                                                            *
* This algorithm was written with clarity in mind, not extreme speed. It is  *
* much clearer (and I'm pretty sure a bit faster) than the mess you would    *
* find in the Windows driver.                                                *
*                                                                            *
* Lastly, the Portable PixMap read routine is an abomination. It doesn't     *
* allow for comments in the header, which The GIMP will add. To use a P6     *
* formatted image (extension .pnm in The GIMP), you must remove the comments.*
* Sorry.                                                                     *
*                                                                            *
\****************************************************************************/

#include <stdio.h>
#include <bios.h>

#define BRI     3
#define RED     2
#define GRN     1
#define BLU     0

static unsigned char far *vidmem = (unsigned char far *)0xA0000000L;
static int orgmode;
/*
 * 8x4 dither matrix (4x4 replicated twice horizontally to fill byte).
 */
static unsigned long dithmask[16] =
{
    0x00000000L,
    0x88000000L,
    0x88002200L,
    0x8800AA00L,
    0xAA00AA00L,
    0xAA44AA00L,
    0xAA44AA11L,
    0xAA44AA55L,
    0xAA55AA55L,
    0xAADDAA55L,
    0xAADDAA77L,
    0xAADDAAFFL,
    0xAAFFAAFFL,
    0xEEFFAAFFL,
    0xEEFFFFFFL,
    0xFFFFFFFFL
};
static unsigned int pixmask[] =
{
    0x8008, 0x4008, 0x2008, 0x1008, 0x0808, 0x0408, 0x0208, 0x0108
};
void setmode()
{
    union REGS regs;

    /*
     * Get current mode and set mode 0x12 (640x480x4).
     */
    regs.x.ax = 0x0F00;
    int86(0x10, &regs, &regs);
    orgmode   = regs.h.al;
    regs.x.ax = 0x0012;
    int86(0x10, &regs, &regs);
    /*
     * Set write mode 2.
     */
    outpw(0x3CE, 0x0205);
    outpw(0x3C4, 0x0F02);
}
void restoremode(void)
{
    union REGS regs;

    regs.x.ax = orgmode;
    int86(0x10, &regs, &regs);
}
/*
 * Build a dithered brush and return the closest solid color match to the RGB.
 */
unsigned char buildbrush(unsigned char red, unsigned char grn, unsigned blu, unsigned long *brush)
{
    unsigned char clr, v;

    /*
     * Find MAX(R,G,B)
     */
    if (red >= grn && red >= blu)
        v = red;
    else if (grn >= red && grn >= blu)
        v = grn;
    else // if (blue >= grn && blu >= red)
        v = blu;
    if (v > 127) // 50%-100% brightness
    {
        /*
         * Fill brush based on scaled RGB values (brightest -> 100% -> 0x0F).
         */
        brush[BRI] = ~dithmask[(~v >> 3) & 0x0F]; // Reverse dither for brightness
        brush[RED] =  dithmask[(red << 4) / (v + 8)];
        brush[GRN] =  dithmask[(grn << 4) / (v + 8)];
        brush[BLU] =  dithmask[(blu << 4) / (v + 8)];
        clr = 0x08 | ((red & 0x80) >> 5) | ((grn & 0x80) >> 6) | ((blu & 0x80) >> 7);
    }
    else // 0%-50% brightness
    {
        /*
         * Fill brush based on RGB values.
         */
        brush[BRI] = 0;
        brush[RED] = dithmask[red >> 3];
        brush[GRN] = dithmask[grn >> 3];
        brush[BLU] = dithmask[blu >> 3];
        clr = ((red & 0x40) >> 4) | ((grn & 0x40) >> 5) | ((blu & 0x40) >> 6);
        if (clr == 0x00 && red > 31 && grn > 31 && blu > 31)
            clr = 0x08;
    }
    return clr;
}
/*
 * This is a horrible way to set a pixel. It builds a 4x4 brush then extracts
 * the 4 bit pixel value from the 4 color planes. Treat the brush as 4 rows of
 * individual IRGB bytes instead of the 4 rows of a combined IRGB long in the
 * build brush routine.
 */
void setpixel(int x, int y, unsigned char red, unsigned char grn, unsigned char blu)
{
	int ofs;
	volatile unsigned char dummy;
    unsigned char pixbrush[4][4], pix, idx;

    idx = buildbrush(red, grn, blu, (unsigned long *)pixbrush);
    /*
     * Extract pixel value from IRGB planes.
     */
    pix  = ((pixbrush[BRI][y & 3] >> (x & 3)) & 0x01) << BRI;
    pix |= ((pixbrush[RED][y & 3] >> (x & 3)) & 0x01) << RED;
    pix |= ((pixbrush[GRN][y & 3] >> (x & 3)) & 0x01) << GRN;
    pix |= ((pixbrush[BLU][y & 3] >> (x & 3)) & 0x01) << BLU;
    /*
     * Write mode 2 to set the color value.
     */
    outpw(0x3CE, pixmask[x & 0x07]);
    ofs         = y * 80 + (x >> 3);
    dummy       = vidmem[ofs];
	vidmem[ofs] = pix; // Use idx here to see "best match" color
}
/*
 * World's dumbest routine to read PNM file.
 */
int main(int argc, char **argv)
{
	FILE *pbmfile;
	int pbmwidth, pbmheight, pbmdepth;
	int xorg, yorg, x, y;
	unsigned char r, g, b;

	if (argc > 1)
	{
		if ((pbmfile = fopen(argv[1], "rb")) == NULL)
		{
			fprintf(stderr, "Can't open %s\n", argv[1]);
			return -1;
		}
	}
	else
		pbmfile = stdin;
	if (fscanf(pbmfile, "P6\n%d\n%d\n%d\n", &pbmwidth, &pbmheight, &pbmdepth) != 3)
	{
		fprintf(stderr, "Not a valid PBM file.\n");
		return -1;
	}
	if (pbmwidth > 640 || pbmheight > 480)
	{
		fprintf(stderr, "PBM too large to  display.n");
		return -1;
	}
	xorg = 320 - (pbmwidth / 2);
	yorg = 240 - (pbmheight / 2);
	setmode();
	for (y = 0; y < pbmheight; y++)
		for (x = 0; x < pbmwidth; x++)
        {
            r = getc(pbmfile);
            g = getc(pbmfile);
            b = getc(pbmfile);
			setpixel(x + xorg, y + yorg, r, g, b);
        }
	getch();
	restoremode();
	return 0;
}


