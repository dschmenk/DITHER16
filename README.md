# DITHER16
Dither RGB color to 4 bit planar brush using the standard EGA/VGA colors.

The EGA/VGA palette is constructed to map:

    Plane 0: Blue
    Plane 1: Green
    Plane 2: Red
    Plane 3: Brightness

The colors are mapped so they can be additive: the same bit set in plane 0 as in plane 1 will combine to make Cyan (Blue + Green = Cyan). If the same bit is set in plane 2, then White will result (Blue + Green + Red = White). Plane 3 is mapped to create the high intensity versions of the 8 color combinations.  This makes a simple dither algorithm a little more difficult but manageable when the normal intensity values are seperated from the high intensity values. Thinking of the mapping as a [HSV cone](https://en.wikipedia.org/wiki/HSL_and_HSV) clarifies the process. If V is less than 50% (128), building a simple dithered brush by scaling the RGB values from 0-127 to 0-15 is sufficient. The Brightness value is set to 0. For values of V greater than 50%, it gets a little more interesting. The idea is to scale the RGB values to percentages of V. Note that V = MAX(R,G,B) so by dividing each by V will fill the brush with the percentage of the RGB color. Now, the Brightness can be set to V. However, a little trick here is to reverse the order of the Brightness dither pattern so that the most significant color is brightened first. If the same dither pattern was used as the RGB color values, the least significant colors would be brightened first.

I can see two issues with this algorithm:

+ The colors are symmetrical around 128 leading to a double mapping of brushes. 0x70-0x7F look suspiciously like 0x80-0x8F. This is actually the solid colors mapped to the palette so I don't think it's a horrible problem, but you can see it in certain smooth gradients.

+ Palette entry 0x08 doesn't get used in building the brushes. This is the equivalent of Bright Black. The Windows driver doesn't use it either for the dithered brush. However, it is used for the "best match" when a solid color value is needed. So I added a "best match" to this brush building routine and return that as a value, too.

[Update]
The above issues have been addressed in the latest update as well as the plattes have been reprogrammed to fix the dim yellow looking more like brown.

Finally, the sample program has added a gamma function to adjust the image display. It greatly improves the image on cheap, washed-out LCDs or bright CTRs.

Type: `brush -g 1.5 bars.pnm` (inside DOSBox) to apply a power function to darken the displayed image. A value > 1.0 will lighten the image.

This algorithm was written with clarity in mind, not extreme speed. It is much clearer (and I'm pretty sure a bit faster) than the mess you would find in the Windows driver.

Lastly, the Portable PixMap read routine is an abomination. It doesn't allow for comments in the header, which The GIMP will add. To use a PBM formatted image (extension .pnm in The GIMP), you must remove the comments.

Sorry.

====================================================================================

## Implementation

```
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
        brush[BRI] = idithmask[(v >> 3) & 0x0F]; //  Bright dither is opposit color dither
        brush[RED] = bdithmask[(red << 4) / (v + 8)];
        brush[GRN] = bdithmask[(grn << 4) / (v + 8)];
        brush[BLU] = bdithmask[(blu << 4) / (v + 8)];
        clr        = 0x08
                   | ((red & 0x80) >> 5)
                   | ((grn & 0x80) >> 6)
                   | ((blu & 0x80) >> 7);
    }
    else // 0%-50% brightness
    {
        /*
         * Fill brush based on RGB values.
         */
        if ((v - red) + (v - grn) + (v - blu) < 8)
        {
            /*
             * Fill brush with dark grey dither if RGB mostly grey.
             */
            if (v > 63)
            {
                brush[BRI] = ~ddithmask[((v - 64) >> 2)];
                brush[RED] =
                brush[GRN] =
                brush[BLU] =  ddithmask[((v - 64) >> 2)];
                clr        =  0x08;
            }
            else
            {
                brush[BRI] = ddithmask[(v >> 2)];
                brush[RED] = 0;
                brush[GRN] = 0;
                brush[BLU] = 0;
                clr        = 0x00;
            }
        }
        else
        {
            brush[BRI] = 0;
            brush[RED] = ddithmask[red >> 3];
            brush[GRN] = ddithmask[grn >> 3];
            brush[BLU] = ddithmask[blu >> 3];
            clr        = ((red & 0x40) >> 4)
                       | ((grn & 0x40) >> 5)
                       | ((blu & 0x40) >> 6);
        }
    }
    return clr;
}
```

====================================================================================

## Here are the results:

Color Bars #1

![Original Color Bars](https://github.com/dschmenk/DITHER16/blob/master/images/bars1.jpg)
![Dithered Color Bars](https://github.com/dschmenk/DITHER16/blob/master/images/bars1.png)
Update (gamma 1.25):
![Dithered Color Bars](https://github.com/dschmenk/DITHER16/blob/master/images/bars1.a.png)


Color Bars #2

![Original Color Bars](https://github.com/dschmenk/DITHER16/blob/master/images/bars2.jpg)
![Dithered Color Bars](https://github.com/dschmenk/DITHER16/blob/master/images/bars2.png)
Update (gamma 1.25):
![Dithered Color Bars](https://github.com/dschmenk/DITHER16/blob/master/images/bars2.a.png)

Compaq Computers

![Original Color Bars](https://github.com/dschmenk/DITHER16/blob/master/images/compaqs.jpg)
![Dithered Color Bars](https://github.com/dschmenk/DITHER16/blob/master/images/compaqs.png)
Update (gamma 1.75):
![Dithered Color Bars](https://github.com/dschmenk/DITHER16/blob/master/images/compaqs.a.png)

Race Car

![Original Color Bars](https://github.com/dschmenk/DITHER16/blob/master/images/racecar.jpg)
![Dithered Color Bars](https://github.com/dschmenk/DITHER16/blob/master/images/racecar.png)
Update (gamma 1.75):
![Dithered Color Bars](https://github.com/dschmenk/DITHER16/blob/master/images/racecar.a.png)
