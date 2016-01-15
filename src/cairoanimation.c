#include <tizen.h>
#include "cairoanimation.h"
#include <cairo.h>
#include <math.h>
#include <Evas_GL.h>
#include <cairo-evas-gl.h>

#define DW 360
#define DH 360

typedef struct appdata {
	Evas_Coord width;
	Evas_Coord height;

	Evas_Object *win;
	Evas_Object *img;

	cairo_surface_t *surface;
	cairo_t *cairo;

	cairo_device_t *cairo_device;
	Evas_GL *evas_gl;
	Evas_GL_Config *evas_gl_config;
	Evas_GL_Surface *evas_gl_surface;
	Evas_GL_Context *evas_gl_context;
	int angle;
} appdata_s;

void cairo_drawing(void *data, int dir);

Eina_Bool _rotary_handler_cb(void *data, Eext_Rotary_Event_Info *ev)
{
   if (ev->direction == EEXT_ROTARY_DIRECTION_CLOCKWISE)
   {
      //dlog_print(DLOG_DEBUG, LOG_TAG, "DBG FW Rotary device rotated in clockwise");
      cairo_drawing(data,0);
   }
   else
   {
      //dlog_print(DLOG_DEBUG, LOG_TAG, "DBG FF Rotary device rotated in counter clockwise");
      cairo_drawing(data,1);
   }

   return EINA_FALSE;
}


void cairo_drawing_rt(void){
	//dummy draw
}

void cairo_drawing(void *data, int dir)
{
	appdata_s *ad = data;
	static int i;

	/* clear background as white */
	cairo_set_source_rgba(ad->cairo, 0, 0, 0, 1);
	cairo_paint(ad->cairo);

	cairo_set_operator(ad->cairo, CAIRO_OPERATOR_OVER);

	if(dir == 0) i++; else i--;

	cairo_set_line_width(ad->cairo, 1);
	cairo_set_source_rgb(ad->cairo, 1, 1, 1);
	cairo_rectangle(ad->cairo, 10, 10, DW - 20, DW - 20);
	cairo_stroke(ad->cairo);

	cairo_rectangle(ad->cairo, 10+DW, 10, DW * 2 - 20, DW - 20);
	cairo_stroke(ad->cairo);

	//cairo_translate(cairo, DW/2, DW/2);
	cairo_arc(ad->cairo, DW/2, DW/2, 30, 0,  2 * M_PI);
	//cairo_stroke_preserve(cairo);
	cairo_stroke(ad->cairo);

	cairo_set_source_rgba(ad->cairo, 0,1,0,0.2);

	cairo_arc(ad->cairo, DW/2, DW/2, DW/2, i * (M_PI/180) , (i+30) * (M_PI/180));
	cairo_fill (ad->cairo);
	cairo_stroke(ad->cairo);


	cairo_move_to(ad->cairo, DW/2, DW/2);
	cairo_line_to(ad->cairo, DW/2 + DW/2 *cos(i*(M_PI/180)),DW/2 + DW/2 * sin(i*(M_PI/180)));
	cairo_line_to(ad->cairo, DW/2 + DW/2 *cos(((i+30)*(M_PI/180))),DW/2 + DW/2 * sin((i+30)*(M_PI/180)));
	cairo_line_to(ad->cairo, DW/2, DW/2);
	cairo_fill(ad->cairo);
	cairo_stroke(ad->cairo);

	cairo_surface_flush(ad->surface);
}

static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;

	cairo_surface_destroy(ad->surface);
	cairo_destroy(ad->cairo);
	cairo_device_destroy(ad->cairo_device);

	evas_gl_surface_destroy(ad->evas_gl, ad->evas_gl_surface);
	evas_gl_context_destroy(ad->evas_gl, ad->evas_gl_context);
	evas_gl_config_free(ad->evas_gl_config);
	evas_gl_free(ad->evas_gl);

	ui_app_exit();
}

static void
win_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	/* Let window go to hide state. */
	elm_win_lower(ad->win);
}

static void
win_resize_cb(void *data, Evas *e , Evas_Object *obj , void *event_info)
{
	appdata_s *ad = data;

	if(ad->evas_gl_surface)
	{
		cairo_surface_destroy(ad->surface);
		cairo_destroy(ad->cairo);
		cairo_device_destroy(ad->cairo_device);
		evas_gl_surface_destroy(ad->evas_gl, ad->evas_gl_surface);
		ad->evas_gl_surface = NULL;
	}

	evas_object_geometry_get(obj, NULL, NULL, &ad->width, &ad->height);
	evas_object_image_size_set(ad->img, ad->width, ad->height);
	evas_object_resize(ad->img, ad->width, ad->height);
	evas_object_show(ad->img);

	if(!ad->evas_gl_surface)
	{
		Evas_Native_Surface ns;
		ad->evas_gl_surface = evas_gl_surface_create(ad->evas_gl, ad->evas_gl_config, ad->width, ad->height);
		evas_gl_native_surface_get(ad->evas_gl, ad->evas_gl_surface, &ns);
		evas_object_image_native_surface_set(ad->img, &ns);
		evas_object_image_pixels_dirty_set (ad->img, EINA_TRUE);

		ad->cairo_device = (cairo_device_t *)cairo_evas_gl_device_create (ad->evas_gl, ad->evas_gl_context);
		cairo_gl_device_set_thread_aware(ad->cairo_device, 0);
		ad->surface = (cairo_surface_t *)cairo_gl_surface_create_for_evas_gl(ad->cairo_device, ad->evas_gl_surface, ad->evas_gl_config, ad->width, ad->height);
		ad->cairo = cairo_create (ad->surface);
	}
}

static void
cairo_evasgl_drawing(appdata_s *ad)
{
	/* Window */
	elm_config_accel_preference_set("opengl");
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_autodel_set(ad->win, EINA_TRUE);
	if (elm_win_wm_rotation_supported_get(ad->win))
	{
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(ad->win, (const int *)(&rots), 4);
	}
	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);
	eext_object_event_callback_add(ad->win, EEXT_CALLBACK_BACK, win_back_cb, ad);
	evas_object_event_callback_add(ad->win, EVAS_CALLBACK_RESIZE, win_resize_cb, ad);
	evas_object_show(ad->win);

	/* Image */
	ad->img = evas_object_image_filled_add(evas_object_evas_get(ad->win));
	evas_object_show(ad->img);

	evas_object_geometry_get(ad->win, NULL, NULL, &ad->width, &ad->height);

	/* Init EVASGL */
	Evas_Native_Surface ns;
	ad->evas_gl = evas_gl_new(evas_object_evas_get(ad->img));
	ad->evas_gl_config = evas_gl_config_new();
	ad->evas_gl_config->color_format = EVAS_GL_RGBA_8888;
	ad->evas_gl_surface = evas_gl_surface_create(ad->evas_gl, ad->evas_gl_config, ad->width, ad->height);
	ad->evas_gl_context = evas_gl_context_create(ad->evas_gl, NULL);
	evas_gl_native_surface_get(ad->evas_gl, ad->evas_gl_surface, &ns);
	evas_object_image_native_surface_set(ad->img, &ns);
	evas_object_image_pixels_get_callback_set(ad->img, cairo_drawing_rt, ad);

	/* cairo & cairo device create with evasgl */
	setenv("CAIRO_GL_COMPOSITOR", "msaa", 1);
	ad->cairo_device = (cairo_device_t *)cairo_evas_gl_device_create (ad->evas_gl, ad->evas_gl_context);
	cairo_gl_device_set_thread_aware(ad->cairo_device, 0);
	ad->surface = (cairo_surface_t *)cairo_gl_surface_create_for_evas_gl(ad->cairo_device, ad->evas_gl_surface, ad->evas_gl_config, ad->width, ad->height);
	ad->cairo = cairo_create (ad->surface);
}

static Eina_Bool
_animate_cb(void *data)
{
	Evas_Object *obj = (Evas_Object *)data;
	evas_object_image_pixels_dirty_set(obj, EINA_TRUE);
	return EINA_TRUE;
}

static bool
app_create(void *data)
{
	appdata_s *ad = data;
	cairo_evasgl_drawing(ad);

	ecore_animator_frametime_set(0.016);
	ecore_animator_add(_animate_cb, (void *)ad->img);

	eext_rotary_event_handler_add(_rotary_handler_cb, ad);
	return true;
}

static void
app_control(app_control_h app_control, void *data)
{
	/* Handle the launch request. */
}

static void
app_pause(void *data)
{
	/* Take necessary actions when application becomes invisible. */
}

static void
app_resume(void *data)
{
	/* Take necessary actions when application becomes visible. */
}

static void
app_terminate(void *data)
{
	/* Release all resources. */
	eext_rotary_event_handler_del(_rotary_handler_cb);
}

static void
ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	char *locale = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);
	elm_language_set(locale);
	free(locale);
	return;
}

static void
ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
	return;
}

static void
ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void
ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void
ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

int
main(int argc, char *argv[])
{
	appdata_s ad = {0,};
	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);
	ui_app_remove_event_handler(handlers[APP_EVENT_LOW_MEMORY]);

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_main() is failed. err = %d", ret);
	}

	return ret;
}
