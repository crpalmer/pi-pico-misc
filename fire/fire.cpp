#include <stdio.h>
#include <stdlib.h>
#include "gp-input.h"
#include "gp-output.h"
#include "neopixel-pico.h"
#include "pi.h"
#include "util.h"

#define BASE_R 226		/* use 158,8,148 for purple */
#define BASE_G 121		/* use 74,150,12 for green */
#define BASE_B 35

#define FLICKER_LOW 0
#define FLICKER_HIGH 55

#define SLEEP_LOW 10
#define SLEEP_HIGH 100

#define PIN 0
#define N_LEDS 10

static void flicker(NeoPixelPico *neo, int led)
{
    int flicker = random_number_in_range(0, 55);

    int r = BASE_R - random_number_in_range(FLICKER_LOW, FLICKER_HIGH);
    int g = BASE_G - random_number_in_range(FLICKER_LOW, FLICKER_HIGH);
    int b = BASE_B - random_number_in_range(FLICKER_LOW, FLICKER_HIGH);

    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;

    neo->set_led(led, r, g, b);
}

static void flicker_all(NeoPixelPico *neo)
{
    for (int i = 0; i < N_LEDS; i++) {
	flicker(neo, i);
    }
}

int
main(int argc, char **argv)
{
    pi_init_no_reboot();

    gpioInitialise();

    NeoPixelPico *neo = new NeoPixelPico(PIN);
    neo->set_n_leds(N_LEDS);

    while (1) {
	flicker_all(neo);
        neo->show();
        ms_sleep(random_number_in_range(SLEEP_LOW, SLEEP_HIGH));
    }
}
