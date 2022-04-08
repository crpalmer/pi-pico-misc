#include <stdio.h>
#include <stdlib.h>
#include "gp-input.h"
#include "gp-output.h"
#include "util.h"

#define QUIET 1
#define BUZZ 0

#define TRIGGER 1

#include <pico/stdlib.h>

int
main(int argc, char **argv)
{
    gpioInitialise();

    stdio_init_all();

    input_t *i = new GPInput(0);
    output_t *o = new GPOutput(1);
    //output_t *o = new GPOutput(PICO_DEFAULT_LED_PIN);

    o->set(QUIET);
    i->set_pullup_up();

    while (1) {
	int buzzes = 0;

printf("%d\n", i->get_with_debounce());
	while (i->get_with_debounce() == TRIGGER) {
printf("buzz %d\n", buzzes);
	    o->set(BUZZ);
	    ms_sleep(75);
printf("quiet\n");
	    o->set(QUIET);
	    ms_sleep(100);
	    buzzes++;
	    if (buzzes > 10) ms_sleep(1000);
	}
    }
}
