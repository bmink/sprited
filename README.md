# sprited

Create, edit, animate & manipulate sprites.

![Demo of sprited](demo/sprited_demo.gif)


## Creating and editing

`sprited` operates on JSON files that describe one or more sprites. The file
has the following structure:

```JSON
[
  {
    "name": "domino_cat",
    "width": 32,
    "height": 32,
    "pixels": [
      "................................",
      "................................",
      "......X.........................",
      ".....XXX........XXX.............",
      ".....XXXX......XXXX.............",
      ".....XXXXX....XXXXX.............",
      ".....XXXXXXXXXXXXXX.............",
      ".....XXXXXXXXXXXXXX.............",
      ".....XX...XXX...XXX.............",
      "....XXX.X.XXX.X.XXX.............",
      "...XXXX...XXX...XXXX............",
      "....XXXXXXX.XXXXXXX.............",
      ".....XXXXXXXXXXXXX......XXX.....",
      "......XXXX..XXXXX......XXXXXX...",
      "........XXXXXXXX......XXXXXXXX..",
      ".......X........X....XXXXXXXXXX.",
      "......XXXX...XXXX....XXXXXXXXX..",
      "......XXXXXXXXXXXX...XXXXXXXX...",
      "......XXXXXXXXXXXXX...XXXXXXXX..",
      "......XXXXXXXXXXXXXX...XXXXXXXX.",
      "......XXXXXXXXXXXXXXX...XXXXXXX.",
      ".......XXXXXXXXXXXXXXX..XXXXXXX.",
      "........XXXXXXXX.XXXXXX.XXXXXXX.",
      "........X.XXXXXX.XXXXXX.XXXXXXX.",
      "........XX..XXXX.XXXXXX.XXXXXXX.",
      "........XXX..XX.XXXXXXX.XXXXXXX.",
      "........XXX.XXX.XXXXXXX.XXXXXX..",
      "........XXX.XXX.XXXXXXX.XXXXX...",
      "........XXX.XXX.XXXXXXXXXXXX....",
      ".......XX.X.XXX.XXXXX.XXXXX.....",
      ".......X.X.X.XX.XXXXX.XXXX......",
      "................................"
    ]
  }
]
```

(The file is always an array of objects, the above example shows an array with
one object element.)


## Displaying sprites

The contents of a sprite file can be displayed on the screen with `sprited
<spritefile.json> display`. Some sprites look better / are meant to be
inverted, so an `invdisplay` command is also provided.


## Animations

A file containing more than one sprite can be played back as an animation using
`sprited <spritefile.json> animate <milliseconds> <loopcnt>` where
`milliseconds` is the delay between frames and the animation will be
looped `loopcnt` times. As with displaying sprites, animations can be inverted
by using the `invanimate` command.


## Rotating a sprite

`sprited <spritefile.json> rotate <degree>` can be used to rotate a sprite a
specific angle. The output will be a new sprite file with the rotated sprite.

To create a full set of the sprite rotated 360 degrees in specific increments,
use `sprited <spritefile.json> rotate360 <increment>`. This will output a
a sprite set with all degree increments. Eg. if `increment == 15`, then the
output array will contain 24 sprites: 0deg, 15deg, 30deg, 45deg, all the
way to 345deg.

The below is the output of `sprited domino_cat.json rotate 90`:

```JSON
[
  {
    "name": "domino_cat_90_deg",
    "width": 32,
    "height": 32,
    "pixels": [
      "................................",
      "...............X...XXXXXXX......",
      "..............XXX.XXXXXXXXX.....",
      ".............XXXXXXXXXXXXXXX....",
      ".............XXXXXXXXXXXXXXXX...",
      "............XXXXXXXXXXXXXXXXXX..",
      "............XXXXXXXXXXXXXXXXXXX.",
      "............XXXXXXXXXXXXXXXXXXX.",
      ".............XXXXXXX........XXX.",
      "..............XXXXX...XXXXXXXXX.",
      "...............XXX...XXXXXXXX...",
      "....................XXXXXXXXXXX.",
      "..........X........XXXXXXXXXXXX.",
      "...XXXXXXXXX......XXXXXXXXXXXXX.",
      "...XXXXXXXXXX....XXXXXXXXXXXXXX.",
      "...XXXXXXXXXXX.XXXXXXX...XXXXXX.",
      "....XXXX...XXXX.XXXXXXXXX.......",
      ".....XXX.X.XXXX.XXXXXXXXXXXXXXX.",
      "......XX...XXXX.XXXXXXXXXXXXXXX.",
      "......XXXXXXXXX..XXXXXXXX.XXXX..",
      "......XXXXX.X.X..XXXXXXX......X.",
      "......XXXXXXX.X..XXXXXXX.XXXXX..",
      ".....XXX...XXXX.XXXXXXX.XXXXX.X.",
      "....XXXX.X.XXXX.XXXXXXXXXXXXXX..",
      "...XXXXX...XXX.XXXXXXX.......XX.",
      "..XXXXXXXXXXXX..XXXXX...........",
      "...XXXXXXXXXX...................",
      ".........XXX....................",
      "..........X.....................",
      "................................",
      "................................",
      "................................"
    ]
  }
]
```

The rotation is done with an inverse mapping + bilinear interpolation algorithm
with thresholds that work OK... but don't expect wonders. Algorithmically
rotated pixel art (especially small bitmaps such as most sprites) will almost
always require post-editing by hand. The expected workflow is to create the
main sprite, then use `sprited` to create the rotations, then hand-edit the
resulting file, all the while using `display` or `animate` to check the
results.

The only exception to this are 90deg, 180deg and 270deg rotations, as for those
the pixels can be mapped 1:1.

Note: for the rotation commands, the input file must contain exactly one sprite
of square dimensions (ie. `width` the same as `height`).


## Mirroring

Mirroring sprites is supported via `sprited <spritefile.json> fliph` and
`sprited <spritefile.json> flipv`. Mirroring can be very helpful in rotating
sprites that are symmetrical along one or both axes, to cut down on the number
of sprites needing hand editing. 


## Conversion of sprites to C byte arrays

Use `sprited <spritefile.json> tobytesh <bufname>` and `sprited
<spritefile.json> tobytesv <bufname>` to output the sprite(s) in the intput
file as byte arrays you can compile into your program. If the input contains
one sprite then the output will be a simple byte array. If the input contains
more than one sprites then the output will be an array of byte arrays.

Output of `sprited domino_cat.json tobytesh domino`:

```C
uint8_t domino_buf[] =
        { /* "domino_cat" (32x32): horizontal mapping */
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x02, 0x00, 0x00, 0x00, 0x07, 0x00, 0xe0, 0x00,
          0x07, 0x81, 0xe0, 0x00, 0x07, 0xc3, 0xe0, 0x00,
          0x07, 0xff, 0xe0, 0x00, 0x07, 0xff, 0xe0, 0x00,
          0x06, 0x38, 0xe0, 0x00, 0x0e, 0xba, 0xe0, 0x00,
          0x1e, 0x38, 0xf0, 0x00, 0x0f, 0xef, 0xe0, 0x00,
          0x07, 0xff, 0xc0, 0xe0, 0x03, 0xcf, 0x81, 0xf8,
          0x00, 0xff, 0x03, 0xfc, 0x01, 0x00, 0x87, 0xfe,
          0x03, 0xc7, 0x87, 0xfc, 0x03, 0xff, 0xc7, 0xf8,
          0x03, 0xff, 0xe3, 0xfc, 0x03, 0xff, 0xf1, 0xfe,
          0x03, 0xff, 0xf8, 0xfe, 0x01, 0xff, 0xfc, 0xfe,
          0x00, 0xff, 0x7e, 0xfe, 0x00, 0xbf, 0x7e, 0xfe,
          0x00, 0xcf, 0x7e, 0xfe, 0x00, 0xe6, 0xfe, 0xfe,
          0x00, 0xee, 0xfe, 0xfc, 0x00, 0xee, 0xfe, 0xf8,
          0x00, 0xee, 0xff, 0xf0, 0x01, 0xae, 0xfb, 0xe0,
          0x01, 0x56, 0xfb, 0xc0, 0x00, 0x00, 0x00, 0x00 };
```

Some displays map pixels vertically in their internal display buffer (ie. one
byte represents an 8 pixel section of a column on the display; for such cases
`tobytesv` command is provided.

## Fonts

In the context of drawing pixels on the screen (or in a framebuffer), text
and fonts are just another set of sprites! Check the "fonts" folder for some
of my favorite fonts that I use in my projects.

Use `sprited <spritefile.json> tofonth <fontname>` and `sprited
<spritefile.json> tofontv <fontname>` to output the sprites in the intput
file as several arrays and structs that you can use to write text into
display buffers. See [simple_displays/(https://github.com/bmink/simple_displays)
for more information on this.


Happy spriting!

