/*******************************
    Zoom Window
********************************/

#include <pebble.h>
#include "Rebble.h"
#include "ThreadWindow.h"
#include "SubredditWindow.h"
#include "LoadingWindow.h"
#include "ThreadMenuWindow.h"
#include "CommentWindow.h"

Window *zoom_window;

GSize zoom_layer_size;

ScrollLayer *image_pan_layer;

BitmapLayer *zoom_bitmap_layer;

GBitmap *image;

static void zoom_click_config(void *context);
static void zoom_offset_changed_handler(ScrollLayer *scroll_layer, void *context);
static void zoom_button_up(ClickRecognizerRef recognizer, void *context);
static void zoom_button_select(ClickRecognizerRef recognizer, void *context);
static void zoom_button_down(ClickRecognizerRef recognizer, void *context);
//static void zoom_load();
static void zoom_window_init();


void zoom_load()
{
	loading_init();

		loading_set_text("Loading Zoomed Image");

		init_netimage(GetSelectedThreadID());


}

void zoom_load_finished()
{
	if(loading_visible())
	{
		loading_uninit();
		zoom_window_init();
	}
}

void zoom_window_init()
{
	window_stack_push(zoom_window, true);
}

void zoom_window_load(Window *window)
{
	//struct ThreadData *thread = GetSelectedThread();

	image_pan_layer = scroll_layer_create(window_frame);

	scroll_layer_set_shadow_hidden(image_pan_layer, true);
	scroll_layer_set_click_config_onto_window(image_pan_layer, window);
	scroll_layer_set_content_size(image_pan_layer, GSize(window_frame.size.w, 0));
	scroll_layer_set_content_offset(image_pan_layer, GPoint(0, 0), false);

	ScrollLayerCallbacks scrollOverride =
	{
		.click_config_provider = &zoom_click_config,
		.content_offset_changed_handler = &zoom_offset_changed_handler
	};
	scroll_layer_set_callbacks(image_pan_layer, scrollOverride);









	layer_add_child(window_get_root_layer(window), scroll_layer_get_layer(image_pan_layer));




		// we are an image

		zoom_bitmap_layer = bitmap_layer_create(GRect(0, 22, window_frame.size.w, window_frame.size.h));
		scroll_layer_add_child(image_pan_layer, bitmap_layer_get_layer(zoom_bitmap_layer));

		scroll_layer_set_content_size(image_pan_layer, GSize(window_frame.size.w, 22 + window_frame.size.h + 10));



}

void zoom_window_appear(Window *window)
{

}

void zoom_window_disappear(Window *window)
{
	cancel_timer();
}

void zoom_window_unload(Window *window)
{
	DEBUG_MSG("zoom_window_unload");

	free_netimage();

	if (image != NULL)
	{
		gbitmap_destroy(image);
		image = NULL;
	}


	if(zoom_bitmap_layer != NULL)
	{
		bitmap_layer_destroy(zoom_bitmap_layer);
		zoom_bitmap_layer = NULL;
	}

	scroll_layer_destroy(image_pan_layer);
}


void zoom_display_image(GBitmap *inputimage)
{
	if(inputimage == NULL)
	{
		loading_disable_dots();
		loading_set_text("Unable to load image");
		return;
	}

	zoom_load_finished();

	if (image)
	{
		gbitmap_destroy(image);
		DEBUG_MSG("gbitmap_destroy 1");
	}

	image = inputimage;

	if(zoom_bitmap_layer == NULL)
	{
		return;
	}

	DEBUG_MSG("zoom_display_image!");

	bitmap_layer_set_bitmap(zoom_bitmap_layer, image);
}

static void zoom_offset_changed_handler(ScrollLayer *scroll_layer, void *context)
{

}

static void zoom_click_config(void *context)
{
	window_long_click_subscribe(BUTTON_ID_UP, 0, zoom_button_up, NULL);
	window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) zoom_button_select);
	window_long_click_subscribe(BUTTON_ID_DOWN, 0, zoom_button_down, NULL);
}

static void zoom_button_up(ClickRecognizerRef recognizer, void *context)
{

}

static void zoom_button_select(ClickRecognizerRef recognizer, void *context)
{

}

static void zoom_button_down(ClickRecognizerRef recognizer, void *context)
{

}



/***************************************************
zoomimage functions
***************************************************/

void zoomimage_receive(DictionaryIterator *iter)
{

	DEBUG_MSG( "received");

	NetImageContext *zoomctx = get_zoomimage_context();

	Tuple *tuple = dict_read_first(iter);

	DEBUG_MSG( "read");

	if (!tuple)
	{
		DEBUG_MSG( "Error");
		DEBUG_MSG("Got a message with no first key! Size of message: %li", (uint32_t)iter->end - (uint32_t)iter->dictionary);
		return;
	}

	switch (tuple->key)
	{
		case ZOOMIMAGE_DATA:
			DEBUG_MSG( "Middle of zoomimage");

			if (zoomctx->index + tuple->length <= zoomctx->length)
			{
				memcpy(zoomctx->data + zoomctx->index, tuple->value->data, tuple->length);
				zoomctx->index += tuple->length;
			}
			else
			{
				DEBUG_MSG("Not overriding rx buffer. Bufsize=%li BufIndex=%li DataLen=%i",
						zoomctx->length, zoomctx->index, tuple->length);
			}
			break;
		case ZOOMIMAGE_BEGIN:
			DEBUG_MSG( "Beginning of zoomimage");

			DEBUG_MSG("Start transmission. Size=%lu", tuple->value->uint32);
			if (zoomctx->data != NULL)
			{
				nt_Free(zoomctx->data);
			}
			if(tuple->value->uint32 == 0)
			{
				zoomctx->data = NULL;
				break;
			}
			zoomctx->data = nt_Malloc(tuple->value->uint32);
			if (zoomctx->data != NULL)
			{
				zoomctx->length = tuple->value->uint32;
				zoomctx->index = 0;
			}
			else
			{
				DEBUG_MSG("Unable to allocate memory to receive image.");
				zoomctx->length = 0;
				zoomctx->index = 0;
			}
			break;
		case ZOOMIMAGE_END:
			DEBUG_MSG( "End of zoomimage");

			if (zoomctx->data && zoomctx->length > 0 && zoomctx->index > 0)
			{

				DEBUG_MSG( "Running GBitmap Create");

				#ifdef PBL_BW
					GBitmap *bitmap = gbitmap_create_with_data(zoomctx->data);
				#else
					GBitmap *bitmap = gbitmap_create_from_png_data(zoomctx->data, zoomctx->length);
				#endif
				nt_Free(zoomctx->data);
				if (bitmap)
				{

					if(bitmap==NULL){
						DEBUG_MSG( "Created bitmap is null");

					}

					DEBUG_MSG( "Running callback");
					zoomctx->callback(bitmap);
				}
				else
				{
					zoomctx->callback(NULL);
					DEBUG_MSG( "Unable to create bitmap");
					DEBUG_MSG("Unable to create GBitmap. Is this a valid PBI?");
				}
				zoomctx->data = NULL;
				zoomctx->index = zoomctx->length = 0;
			}
			else
			{
				zoomctx->callback(NULL);
				DEBUG_MSG( "Got End message but we have no image...");
			}
			break;
		default:
			DEBUG_MSG("Unknown key in dict: %lu", tuple->key);
			break;
	}
}
