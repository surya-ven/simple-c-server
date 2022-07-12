/*
Author : Surya Venkatesh
Purpose: This file contains the driver code for the server.
*/
// Reference: Beej's networking guide
// Reference: Jacob Sorber's series on multithreaded servers

#define _POSIX_C_SOURCE 200112L
#include "serverops.h"
#include <stdlib.h>

#define IMPLEMENTS_IPV6
#define MULTITHREADED

/*
 * Main entrypoint.
 */
int main(int argc, char** argv) {
	init_server(argc, argv);
	return 0;
}