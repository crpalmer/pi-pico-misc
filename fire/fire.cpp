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

static color_t orange = { 223,  56,  25 };
static color_t purple = { 131,  56, 154 };
static color_t red    = { 255,  15,  15 };
static color_t skulls = { 255, 255,   0 };

#define SLEEP_LOW 10
#define SLEEP_HIGH 100

#define FIRE_PIN 0
#define FIRE_N_LEDS 26

#define SKULLS_PIN 1
#define SKULLS_N_LEDS 2

static critical_section cs;
static LightSensor *light_sensor;
static NeoPixelPico *fire_neo, *skulls_neo;
static bool is_paused = false;
static struct { int fire_high, skulls_high; } flicker = { 55, 55 };
static int purple_pct = 3;
static int red_pct = 12;

#define LIGHT_STATE_THRESHOLD	50
#define FIRE_OFF_LUX		9
#define FIRE_ON_LUX		8

static int light_counter = -LIGHT_STATE_THRESHOLD;	/* start ON */
static bool fire_is_on = true;

static void set_is_paused(bool new_is_paused)
{
    critical_section_enter_blocking(&cs);
    is_paused = new_is_paused;
    critical_section_exit(&cs);
}

static void pause_lights()
{
    set_is_paused(true);
}

static void resume_lights()
{
    set_is_paused(false);
}

static void flicker_led(NeoPixelPico *neo, int led, color_t c, int flicker_high)
{
    int r = c.r - random_number_in_range(0, flicker_high);
    int g = c.g - random_number_in_range(0, flicker_high);
    int b = c.b - random_number_in_range(0, flicker_high);

    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;

    neo->set_led(led, r, g, b);
}

static void flicker_fire(NeoPixelPico *neo)
{
    for (int i = 0; i < FIRE_N_LEDS; i++) {
	int pct = random_number_in_range(0, 99);
	color_t c;

	if (pct < purple_pct) c = purple;
	else if (pct < red_pct + purple_pct) c = red;
	else c = orange;

	flicker_led(neo, i, c, flicker.fire_high);
    }
}

static void flicker_skulls(NeoPixelPico *neo)
{
    for (int i = 0; i < SKULLS_N_LEDS; i++) {
	flicker_led(neo, i, skulls, flicker.skulls_high);
    }
}

static void fire_main()
{
    while (1) {
	critical_section_enter_blocking(&cs);
	if (! is_paused) {
	    double lux = light_sensor->get_lux();

	    if (lux >= FIRE_OFF_LUX && light_counter < LIGHT_STATE_THRESHOLD) {
		light_counter++;
		if (light_counter == LIGHT_STATE_THRESHOLD && fire_is_on) {
		    fire_neo->set_all(0, 0, 0);
		    fire_neo->show();
		    skulls_neo->set_all(0, 0, 0);
		    skulls_neo->show();
		    fire_is_on = false;
		}
	    } else if (lux <= FIRE_ON_LUX && light_counter > -LIGHT_STATE_THRESHOLD) {
		light_counter--;
		if (light_counter == -LIGHT_STATE_THRESHOLD) fire_is_on = true;
	    }

	    if (fire_is_on) {
	        flicker_fire(fire_neo);
	        fire_neo->show();
	        flicker_skulls(skulls_neo);
	        skulls_neo->show();
	    }
	}
	critical_section_exit(&cs);

        ms_sleep(random_number_in_range(SLEEP_LOW, SLEEP_HIGH));
    }
}

int
main(int argc, char **argv)
{
    pi_init_no_reboot();

    light_sensor = new LightSensor(2, light_sensor_temt6000);

    fire_neo = new NeoPixelPico(FIRE_PIN);
    fire_neo->set_n_leds(FIRE_N_LEDS);

    skulls_neo = new NeoPixelPico(SKULLS_PIN);
    skulls_neo->set_n_leds(SKULLS_N_LEDS);

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
	    printf("flicker       %3d %3d\n", flicker.fire_high, flicker.skulls_high);
	    printf("orange        %3d %3d %3d\n", orange.r, orange.g, orange.b);
	    printf("purple        %3d %3d %3d\n", purple.r, purple.g, purple.b);
	    printf("red           %3d %3d %3d\n", red.r, red.g, red.b);
	    printf("skulls        %3d %3d %3d\n", skulls.r, skulls.g, skulls.b);
	    printf("percentages   %3d %3d %3d\n", 100-purple_pct-red_pct, purple_pct, red_pct);
	    printf("fire_is_on    %s\n", fire_is_on ? "true" : "false");
	    printf("light_counter %3d %7.2f\n", light_counter, light_sensor->get_lux());
	} else if (strncmp(line, "flicker ", space+1) == 0) {
	    pause_lights();
	    sscanf(&line[space], "%d %d\n", &flicker.fire_high, &flicker.skulls_high);
	    resume_lights();
	    printf("flicker %3d %3d\n", flicker.fire_high, flicker.skulls_high);
	} else if (strcmp(line, "light_sensor") == 0 || strcmp(line, "ls") == 0) {
	    for (int i = 0; i < 20; i++) {
		printf("%6.2f\n", light_sensor->get_lux());
		ms_sleep(500);
	    }
	} else if (strncmp(line, "orange ", space+1) == 0) {
	    pause_lights();
	    sscanf(&line[space], "%d %d %d\n", &orange.r, &orange.g, &orange.b);
	    resume_lights();
	    printf("orange color %3d %3d %3d\n", orange.r, orange.g, orange.b);
	} else if (strncmp(line, "purple ", space+1) == 0) {
	    pause_lights();
	    sscanf(&line[space], "%d %d %d\n", &purple.r, &purple.g, &purple.b);
	    resume_lights();
	    printf("purple color %3d %3d %3d\n", purple.r, purple.g, purple.b);
	} else if (strncmp(line, "purple_pct ", space+1) == 0) {
	    pause_lights();
	    purple_pct = atoi(&line[space]);
	    resume_lights();
	    printf("purple_pct %d\n", purple_pct);
	} else if (strncmp(line, "red ", space+1) == 0) {
	    pause_lights();
	    sscanf(&line[space], "%d %d %d\n", &red.r, &red.g, &red.b);
	    resume_lights();
	    printf("red color %3d %3d %3d\n", red.r, red.g, red.b);
	} else if (strncmp(line, "red_pct ", space+1) == 0) {
	    pause_lights();
	    red_pct = atoi(&line[space]);
	    resume_lights();
	    printf("red_pct %d\n", red_pct);
	} else if (strncmp(line, "skulls ", space+1) == 0) {
	    color_t c;
	    pause_lights();
	    sscanf(&line[space], "%d %d %d\n", &skulls.r, &skulls.g, &skulls.b);
	    printf("skulls color %3d %3d %3d\n", skulls.r, skulls.g, skulls.b);
	    resume_lights();
	} else {
	    printf("bootsel - reboot for flashing\n");
	    printf("flicker low high - set range of flicker\n");
	    printf("light_sensor [or ls] - dump light sensor data\n");
	    printf("orange r g b - set orange color\n");
	    printf("purple r g b - set purple color\n");
	    printf("purple_pct pct - set amount of purple flickers\n");
	    printf("red r g b - set red color\n");
	    printf("red_pct pct - set amount of red flickers\n");
	    printf("skulls r g b - set color of skulls\n");
	}
    }
}
