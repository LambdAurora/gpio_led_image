# GPIO_led_image

A little fun software in C++ to "display" an image to an RGB led.

## How it works?

Run as root the software with as an argument a path to an image file (PNG, JPG, etc...).
For the first run, it will create a configuration file `/etc/gpio_led_image.txt`, the 3 uncommented lines are the R, G and B GPIO pins of the led, defaults are: R=0, G=2 and B=3.

When you launch the configured software with an image, it will read the image and display each pixel one after the other for 150ms, it's just that, just for fun.

## Libraries used

 - wiringPi -> For GPIO manipulation.
 - stb_image -> For image reading.

## Build

Just build it with cmake and make!
