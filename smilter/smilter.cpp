#include <stdio.h>
#include <stdlib.h>
#include <pico/bootrom.h>
#include <pico/multicore.h>
#include <tusb.h>
#include "gp-input.h"
#include "gp-output.h"
#include "light-sensor.h"
#include "neopixel-pico.h"
#include "pi.h"
#include "util.h"

typedef struct {
    int r, g, b;
} color_t;

static color_t white = { 223, 223, 223 };
static color_t red   = { 200,  60,   0 };
static color_t green = {   0, 255,   0 };
static color_t blue  = {   0,  60, 200 };

static color_t *colors[] = { &white, &red, &green, &blue };
#define N_COLORS (sizeof(colors) / sizeof(colors[0]))

#define PIN 0
#define N_LEDS 50

static NeoPixelPico *neo;
static struct { int fire_high, skulls_high; } flicker = { 55, 55 };
static int purple_pct = 3;
static int red_pct = 12;

int main(int argc, char **argv)
{
    int color = 0;

    pi_init();

    neo = new NeoPixelPico(PIN, true);
    neo->set_n_leds(N_LEDS);
    neo->set_mode(neopixel_mode_RGB);
    neo->set_brightness(0.25);

    while (1) {
	for (int i = 0; i < N_LEDS; i++) {
	    color_t *c = colors[(color + i) % N_COLORS];
	    neo->set_led(i, c->r, c->g, c->b);
	}

	neo->show();

	color = (color + 1) % N_COLORS;
        ms_sleep(1000);
    }
}
