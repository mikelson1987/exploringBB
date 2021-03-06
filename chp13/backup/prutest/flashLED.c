#include <stdio.h>
#include <stdlib.h>
#include <prussdrv.h>
#include <pruss_intc_mapping.h>
#include <unistd.h>


#define PRU_NUM 0

/*** pru_setup() -- initialize PRU and interrupt handler

Initializes the PRU specified by PRU_NUM and sets up PRU_EVTOUT_0 handler.

The argument is a pointer to a nul-terminated string containing the path
to the file containing the PRU program binary.

Returns 0 on success, non-0 on error.
***/
static int pru_setup(const char * const path) {
   int rtn;
   tpruss_intc_initdata intc = PRUSS_INTC_INITDATA;

   if(!path) {
      fprintf(stderr, "pru_setup(): path is NULL\n");
      return -1;
   }

   /* initialize PRU */
   if((rtn = prussdrv_init()) != 0) {
      fprintf(stderr, "prussdrv_init() failed\n");
      return rtn;
   }

   /* open the interrupt */
   if((rtn = prussdrv_open(PRU_EVTOUT_0)) != 0) {
      fprintf(stderr, "prussdrv_open() failed\n");
      return rtn;
   }

   /* initialize interrupt */
   if((rtn = prussdrv_pruintc_init(&intc)) != 0) {
      fprintf(stderr, "prussdrv_pruintc_init() failed\n");
      return rtn;
   }

   /* load and run the PRU program */
   if((rtn = prussdrv_exec_program(PRU_NUM, path)) < 0) {
      fprintf(stderr, "prussdrv_exec_program() failed\n");
      return rtn;
   }

   return rtn;
}

/*** pru_cleanup() -- halt PRU and release driver

Performs all necessary de-initialization tasks for the prussdrv library.

Returns 0 on success, non-0 on error.
***/

static int pru_cleanup(void) {
   int rtn = 0;

   /* clear the event (if asserted) */
   if(prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT)) {
      fprintf(stderr, "prussdrv_pru_clear_event() failed\n");
      rtn = -1;
   }

   /* halt and disable the PRU (if running) */
   if((rtn = prussdrv_pru_disable(PRU_NUM)) != 0) {
      fprintf(stderr, "prussdrv_pru_disable() failed\n");
      rtn = -1;
   }

   /* release the PRU clocks and disable prussdrv module */
   if((rtn = prussdrv_exit()) != 0) {
      fprintf(stderr, "prussdrv_exit() failed\n");
      rtn = -1;
   }

   return rtn;
}

int main(int argc, char **argv) {
   int rtn;

   printf("Starting the EBB PRU Program:\n");

   if(geteuid()) {
      fprintf(stderr, "You must be run as root to use the prussdrv\n", argv[0]);
      return -1;
   }

   if(pru_setup("./flashLED.bin")) {
      pru_cleanup();
      return -1;
   }

   /* wait for PRU to assert the interrupt to indicate completion */
   printf("waiting for interrupt from PRU0...\n");

   /* The prussdrv_pru_wait_event() function returns the number of times
      the event has taken place, as an unsigned int. There is no out-of-
      band value to indicate error (and it can wrap around to 0 if you
      run the program just a whole lot of times). */
   rtn = prussdrv_pru_wait_event(PRU_EVTOUT_0);

   printf("PRU program completed, event number %d\n", rtn);

   /* clear the event, disable the PRU and let the library clean up */
   return pru_cleanup();
}
