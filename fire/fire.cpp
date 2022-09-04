#include <stdio.h>
#include <stdlib.h>
#include <pico/bootrom.h>
#include <pico/multicore.h>
#include <tusb.h>
#include "gp-input.h"
#include "gp-output.h"
#include "neopixel-pico.h"
#include "pi.h"
#include "util.h"

typedef struct {
    int r, g, b;
} color_t;

static color_t orange = { 226, 121,  35 };
static color_t purple = { 131,  56, 154 };
static color_t red    = { 203,  89,  67 };

#define SLEEP_LOW 10
#define SLEEP_HIGH 100

#define FIRE_PIN 0
#define FIRE_N_LEDS 26

static critical_section cs;
static NeoPixelPico *fire_neo;
static bool is_paused = false;
static struct { int low, high; } flicker = { 0, 55 };
static int purple_pct = 3;
static int red_pct = 12;

static void set_is_paused(bool new_is_paused)
{
    critical_section_enter_blocking(&cs);
    is_paused = new_is_paused;
    critical_section_exit(&cs);
}

static void pause_fire()
{
    set_is_paused(true);
}

static void resume_fire()
{
    set_is_paused(false);
}

static void flicker_led(NeoPixelPico *neo, int led, color_t c)
{
    int r = c.r - random_number_in_range(flicker.low, flicker.high);
    int g = c.g - random_number_in_range(flicker.low, flicker.high);
    int b = c.b - random_number_in_range(flicker.low, flicker.high);

    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;

    neo->set_led(led, r, g, b);
}

static void flicker_all(NeoPixelPico *neo)
{
    for (int i = 0; i < N_LEDS; i++) {
	int pct = random_number_in_range(0, 99);
	color_t c;

	if (pct < purple_pct) c = purple;
	else if (pct < red_pct + purple_pct) c = red;
	else c = orange;

	flicker_led(neo, i, c);
    }
}

static void fire_main()
{
    while (1) {
	critical_section_enter_blocking(&cs);
	if (! is_paused) {
	    flicker_all(fire_neo);
	    fire_neo->show();
	}
	critical_section_exit(&cs);

        ms_sleep(random_number_in_range(SLEEP_LOW, SLEEP_HIGH));
    }
}

int
main(int argc, char **argv)
{
    pi_init_no_reboot();

    fire_neo = new NeoPixelPico(FIRE_PIN);
    fire_neo->set_n_leds(N_LEDS);

    critical_section_init(&cs);

    multicore_launch_core1(fire_main);

    for (;;) {
	static char line[1024];

	while (! tud_cdc_connected()) ms_sleep(100);

	pico_readline_echo(line, sizeof(line), true);
	int space = 0;
	while (line[space] && line[space] != ' ') space++;

	if (strcmp(line, "bootsel") == 0) {
            printf("Rebooting into bootloader mode...\n");
            reset_usb_boot(1<<PICO_DEFAULT_LED_PIN, 0);
	} else if (strcmp(line, "dump") == 0) {
	    printf("flicker       %3d %3d\n", flicker.low, flicker.high);
	    printf("orange        %3d %3d %3d\n", orange.r, orange.g, orange.b);
	    printf("purple        %3d %3d %3d\n", purple.r, purple.g, purple.b);
	    printf("red           %3d %3d %3d\n", red.r, red.g, red.b);
	    printf("percentages   %3d %3d %3d\n", 100-purple_pct-red_pct, purple_pct, red_pct);
	} else if (strncmp(line, "flicker ", space+1) == 0) {
	    pause_fire();
	    sscanf(&line[space], "%d %d\n", &flicker.low, &flicker.high);
	    resume_fire();
	    printf("flicker %3d %3d\n", flicker.low, flicker.high);
	} else if (strncmp(line, "orange ", space+1) == 0) {
	    pause_fire();
	    sscanf(&line[space], "%d %d %d\n", &orange.r, &orange.g, &orange.b);
	    resume_fire();
	    printf("orange color %3d %3d %3d\n", orange.r, orange.g, orange.b);
	} else if (strncmp(line, "purple ", space+1) == 0) {
	    pause_fire();
	    sscanf(&line[space], "%d %d %d\n", &purple.r, &purple.g, &purple.b);
	    resume_fire();
	    printf("purple color %3d %3d %3d\n", purple.r, purple.g, purple.b);
	} else if (strncmp(line, "purple_pct ", space+1) == 0) {
	    pause_fire();
	    purple_pct = atoi(&line[space]);
	    resume_fire();
	    printf("purple_pct %d\n", purple_pct);
	} else if (strncmp(line, "red ", space+1) == 0) {
	    pause_fire();
	    sscanf(&line[space], "%d %d %d\n", &red.r, &red.g, &red.b);
	    resume_fire();
	    printf("red color %3d %3d %3d\n", red.r, red.g, red.b);
	} else if (strncmp(line, "red_pct ", space+1) == 0) {
	    pause_fire();
	    red_pct = atoi(&line[space]);
	    resume_fire();
	    printf("red_pct %d\n", red_pct);
	} else {
	    printf("bootsel - reboot for flashing\n");
	    printf("flicker low high - set range of flicker\n");
	    printf("orange r g b - set orange color\n");
	    printf("purple r g b - set purple color\n");
	    printf("purple_pct pct - set amount of purple flickers\n");
	    printf("red r g b - set red color\n");
	    printf("red_pct pct - set amount of red flickers\n");
	}
    }
}
