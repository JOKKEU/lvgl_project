#ifndef __LVGL_HD__
	#define __LVGL_HD__
	
#include "../hd/shared_header.h"
#include "lvgl/lvgl.h"
#include <SDL2/SDL.h>
#include "../hd/roboto_mono.h"

#define STANDART_HEIGHT 	800
#define STANDART_WIDTH  	1200
#define CHUNK_SIZE		4096


struct window_param
{
	lv_coord_t height;
	lv_coord_t width;
};


struct media_driver
{
	struct window_param			win_param;	
	
	lv_display_t*				disp;
	lv_fs_drv_t*				disp_drv;
	
	struct buffers*				buffs;
	struct _mouse*				mouse;
	struct _keyboard*			keyboard;
	
	SDL_Window*				window;
	SDL_Event				event;
	SDL_Renderer* 				renderer;
	SDL_Texture*				lvgl_texture;
	
	lv_group_t*				hex_viewer_group;
			
};

struct _mouse
{
	lv_indev_t* 		mouse_indev;
	lv_indev_t*		encoder_indev;
	int 			encoder_diff;
	bool			encoder_event_pending;

};

struct _keyboard
{
	lv_indev_t*		keyboard_indev;
	bool 			key_event_processed;
	lv_group_t*		group;
	SDL_Keycode      	last_sdl_keycode;
	lv_indev_state_t 	last_key_state;
};

struct buffers
{
	void* 			buf1;
	void* 			buf2;
};


struct buttons
{
	lv_obj_t*		button_open_file;
	lv_obj_t*		label_for_choice_file;
	
	lv_obj_t*		button_close;
	lv_obj_t*		lable_for_close;
	
	lv_obj_t*		hex_content_label;
	
	lv_obj_t*		button_change_page;
	lv_obj_t*		label_for_change_page;
	
	lv_obj_t*		label_file_info;
	
};



struct _DISPLAY
{
	
	struct media_driver*	media_drv;
	struct buttons		butts;
	char*			disp_name;
	void			(*cleanup_display)(struct _DISPLAY* );
	
	struct _FILE_STATE*	fs;
	
	
			
};

struct dialog_window_for_data_file
{
	lv_obj_t*		text;
	lv_obj_t*		keyboard;
	lv_obj_t*		cont;
	struct _DISPLAY*	display;
};



extern int display_init(struct _DISPLAY* , struct _FILE_STATE* );
extern void hex_label_event_handler(lv_event_t* event);
extern void sdl_encoder_read_cb(lv_indev_t* indev, lv_indev_data_t* data);
extern void sdl_mouse_read_cb(lv_indev_t * indev, lv_indev_data_t * data);
extern void sdl_keyboard_read_cb(lv_indev_t* indev, lv_indev_data_t* data);
extern void button_open_file_handler(lv_event_t* );
extern void button_for_close(lv_event_t* );
extern void button_change_page_handler(lv_event_t* );

extern void my_flush_cb(lv_display_t* , const lv_area_t* , unsigned char* );

extern void get_log_file_data(struct _FILE_STATE*);
	

#endif // __LVGL_HD__








