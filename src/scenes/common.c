#include <stdlib.h>
#include "../include/text_renderer.h"

float frand() {
    return (float)rand()/(float)(RAND_MAX);
}

void panicEverything(const char* message) {
    // write something!!!
    drawText(0,0,0.5,0,5,0.5,message);
    
    // stop the world for 2 seconds
    svcSleepThread(2 * 1000000000);

    exit(1);
}
