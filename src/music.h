#include <mikmod.h>

void music_init();
void music_thread(void *ptr);
void music_play();
void music_stop();
int music_isplaying();
