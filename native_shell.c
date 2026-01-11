#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

uint32_t rand_u16() {
  return (rand() & 0xF) | ((rand() & 0xF) << 8);
}

void render(uint32_t x, uint32_t y, uint32_t color) {
  printf("\e[?25l\e[%u;%uH%s", x + 1, 2 * y + 1,
    color == 0x00FFFFFF ? "  " : color == 0x0 ? "@@" :
    color == 0x00FF0000 ? "**" : color == 0x0000FF00 ? "00" : ".."
  );
}

uint32_t get_msec() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return (uint32_t) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void sleep_msec(uint32_t msec) {
  usleep(msec * 1000);
}
