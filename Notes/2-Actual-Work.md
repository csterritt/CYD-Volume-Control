Color cycling.

During the hours between 6 AM and 9 PM, the color should cycle through the rainbow.
It should start with red, and proceed through the rainbow, ending with red again.
The color should change every 5 minutes. So at a particular time, the color should be
determined by the current time of day.

The color should be applied just to the text displayed, not the background, which
should remain black.

I would like to use the OKLCH color space for this, as it is more perceptually uniform than RGB.
There should be a constant luminance value of 0.7, and a constant chroma value of 0.2.

The hue should cycle through the rainbow, from 0° to 360°, in 5-minute intervals. Once a color
is chosen, it should be converted to RGB for the text display.
