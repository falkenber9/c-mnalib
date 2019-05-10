#include <stdio.h>
#include <stdlib.h>

#include "cmnalib/network_interface.h"

int main(int argc, char** argv) {

    set_if_down("enp0s25", 0);

    return EXIT_SUCCESS;
}
