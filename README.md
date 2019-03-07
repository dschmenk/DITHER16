# DITHER16
Dither RGB color to 4 bit planar brush using the standard EGA/VGA colors.

The EGA/VGA palette is constructed to map:

    Plane 0: Blue
    Plane 1: Green
    Plane 2: Red
    Plane 3: Brightness

The colors are mapped so they can be additive: the same bit set in plane 0 as in plane 1 will combine to make Cyan (Blue + Green = Cyan). If the same bit is set in plane 2, then White will result (Blue + Green + Red = White). Plane 3 is mapped to create the high intensity versions of the 8 color combinations.  This makes a simple dither algorithm a little more difficult but manageable when the normal intensity values are seperated from the high intensity values. Thinking of the mapping as a HSV cone clarifies the process. If V is less than 50% (128), building a simple dithered brush by scaling the RGB values from 0-127 to 0-15 is sufficient. The Brightness value is set to 0. For values of V greater than 50%, it gets a little more interesting. The idea is to scale the RGB values to percentages of V. Note that V = MAX(R,G,B) so by dividing each by V will fill the brush with the percentage of the RGB color. Now, the Brightness can be set to V. However, a little trick here is to reverse the order of the dither pattern so that the most significant color is brightened first. If the same dither pattern was used as the RGB color values, the lesser colors would be brighted as well.

I can see two issues with this algorithm:

+ The colors are symetrical around 128 leading to a double mapping of brushes. 0x70-0x7F look suspiciously like 0x80-0x8F. This is actually the solid colors mapped to the palette so I don't think it's a horrible problem, but you can see it in certain smooth gradients.

+ Palette entry 0x08 doesn't get used in building the brushes. This is the equivalent of Bright Black. The Windows driver doesn't use it either for the dithered brush. However, it is used for the "best match" when a solid color value is needed. So I added a "best match" to this brush building routine and return that as a value, too.

This algorithm was written with clarity in mind, not extreme speed. It is much clearer (and I'm pretty sure a bit faster) than the mess you would find in the Windows driver.
 
Lastly, the Portable PixMap read routine is an abomination. It doesn't allow for comments in the header, which The GIMP will add. To use a PBM formatted image (extension .pnm in The GIMP), you must remove the comments.

Sorry.
