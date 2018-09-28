#ifndef ZOOM_WINDOW_H
#define ZOOM_WINDOW_H

#include <pebble.h>

void zoom_load();
void zoom_window_init();
void zoomimage_receive(DictionaryIterator *iter);
void zoom_display_image(GBitmap *inputimage);


#endif
