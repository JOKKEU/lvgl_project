#include "../hd/lvgl.h"

static void cleanup_disp(struct _DISPLAY* display);
static uint8_t get_average_color(lv_color_t color);
static void cleanup_dialog_resources(struct dialog_window_for_data_file* dlg_data);
static void proccess_page_number_input(struct dialog_window_for_data_file* dialog_data);
static int format_and_display_page(struct _FILE_STATE* fs, lv_obj_t* label, lv_obj_t* label_file_info, size_t page_num);


struct dialog_window_for_data_file* 	dialog_window;

int display_init(struct _DISPLAY* display, struct _FILE_STATE* fs)
{
	if (!display) 
	{
		ERR("display pointer is NULL\n");
		return EXIT_FAILURE;
	}

	display->media_drv 		= NULL;
	display->disp_name		= NULL;
	display->fs 			= fs;
	display->cleanup_display 	= cleanup_disp; 
	
	
	
	display->media_drv = (struct media_driver*)malloc(sizeof(struct media_driver));
	if (!display->media_drv) 
	{
		ERR("error alloc media driver\n");
		goto cleanup; 
	}
	
	display->media_drv->disp 		= NULL;
	display->media_drv->buffs 		= NULL;
	display->media_drv->window 		= NULL;
	display->media_drv->renderer 		= NULL;
	display->media_drv->keyboard 		= NULL; 
        display->media_drv->mouse 		= NULL;    
        display->media_drv->lvgl_texture 	= NULL; 
	
	
	
	display->media_drv->keyboard = (struct _keyboard*)malloc(sizeof (struct _keyboard));
	if (!display->media_drv->keyboard)
	{
		ERR("error alloc keyboard\n");
		goto cleanup;
	}
	
	display->media_drv->mouse = (struct _mouse*)malloc(sizeof (struct _mouse));
	if (!display->media_drv->mouse)
	{
		ERR("error alloc mouse\n");
		goto cleanup;
	}
	
	display->media_drv->keyboard->group 			= NULL;
	display->media_drv->keyboard->key_event_processed 	= true;
	display->media_drv->keyboard->last_sdl_keycode 		= SDLK_UNKNOWN;
	display->media_drv->keyboard->last_key_state 		= LV_INDEV_STATE_RELEASED;
	display->media_drv->keyboard->keyboard_indev 		= NULL;

	
	display->media_drv->buffs = (struct buffers*)malloc(sizeof(struct buffers));
	if (!display->media_drv->buffs) 
	{
		ERR("error alloc buffer structure\n");
		goto cleanup;
	}
	display->media_drv->buffs->buf1 	= NULL;
	display->media_drv->buffs->buf2 	= NULL;

	display->media_drv->win_param.height 	= STANDART_HEIGHT;
	display->media_drv->win_param.width 	= STANDART_WIDTH;
	display->disp_name = "dump"; 

	
	if (SDL_Init(SDL_INIT_VIDEO) != 0) 
	{
		ERR("SDL Initialization failed: %s\n", SDL_GetError());
		goto cleanup;
	}

	
	display->media_drv->window = SDL_CreateWindow(
		display->disp_name,
		SDL_WINDOWPOS_CENTERED, 
		SDL_WINDOWPOS_CENTERED,
		display->media_drv->win_param.width,
		display->media_drv->win_param.height,
		SDL_WINDOW_SHOWN
	);
	
	if (!display->media_drv->window) 
	{
		ERR("Window could not be created: %s\n", SDL_GetError());
		goto cleanup;
	}

	
	display->media_drv->renderer = SDL_CreateRenderer(display->media_drv->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!display->media_drv->renderer) 
	{
		ERR("error create renderer: %s\n", SDL_GetError());
		goto cleanup;
	}
	Uint32 sdl_pixel_format;
    
    	if (LV_COLOR_DEPTH == 32) 
    	{
        	sdl_pixel_format = SDL_PIXELFORMAT_ARGB8888;
    	} 
    	else if (LV_COLOR_DEPTH == 16) 
    	{
        	sdl_pixel_format = SDL_PIXELFORMAT_RGB565;
    	}
    	else if (LV_COLOR_DEPTH == 24)
    	{
    		sdl_pixel_format = SDL_PIXELFORMAT_RGB24;
    	}
    	else 
    	{
        	ERR("Unsupported LV_COLOR_DEPTH: %d\n", LV_COLOR_DEPTH);
        	goto cleanup; 
    	}

    	display->media_drv->lvgl_texture = SDL_CreateTexture(
		display->media_drv->renderer,
		sdl_pixel_format,
		SDL_TEXTUREACCESS_STREAMING, 
		STANDART_WIDTH,
		STANDART_HEIGHT
   	);
   	 
   	 
    	if (!display->media_drv->lvgl_texture) 
    	{
        	ERR("Failed to create LVGL texture: %s\n", SDL_GetError());
        	goto cleanup;
    	}
	
	lv_init();
	
	display->media_drv->disp = lv_display_create(STANDART_WIDTH, STANDART_HEIGHT);
    	if (!display->media_drv->disp) 
    	{
        	ERR("Failed to create LVGL display\n");
       		goto cleanup;
    	}
    	
    	
    	LOG("Initializing LVGL input device...\n");
	
	display->media_drv->mouse->mouse_indev = lv_indev_create();
	if (!display->media_drv->mouse->mouse_indev)
	{
		ERR("Failed to create input device\n");
		goto cleanup;
	}
	
	
	lv_indev_set_type(display->media_drv->mouse->mouse_indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(display->media_drv->mouse->mouse_indev, sdl_mouse_read_cb);
	lv_indev_set_display(display->media_drv->mouse->mouse_indev, display->media_drv->disp);
	LOG("Mouse input device initialized.\n");
	
	display->media_drv->keyboard->keyboard_indev = lv_indev_create();
	if (!display->media_drv->keyboard->keyboard_indev)
	{
		ERR("Failed to create input device\n");
		goto cleanup;
	}
	
	lv_indev_set_type(display->media_drv->keyboard->keyboard_indev, LV_INDEV_TYPE_KEYPAD);
	
	lv_indev_set_read_cb(display->media_drv->keyboard->keyboard_indev, sdl_keyboard_read_cb);
	lv_indev_set_display(display->media_drv->keyboard->keyboard_indev, display->media_drv->disp);
	
	
	lv_indev_set_user_data(display->media_drv->keyboard->keyboard_indev, (void*)display);
	
	display->media_drv->keyboard->group = lv_group_create();
	if (!display->media_drv->keyboard->group)
	{
		ERR("failed to create input group\n");
		goto cleanup;
	}
	
	lv_group_set_default(display->media_drv->keyboard->group);
	lv_indev_set_group(display->media_drv->keyboard->keyboard_indev, display->media_drv->keyboard->group);
	LOG("Keyboard input device initialized.\n");
	
	
	
	
	display->media_drv->mouse->encoder_indev = lv_indev_create();
	if (!display->media_drv->mouse->encoder_indev)
	{
		ERR("Failed to create input device\n");
		goto cleanup;
	}
	
	
	display->butts.hex_content_label = lv_label_create(display->butts.hex_content_label);
	if (display->butts.hex_content_label == NULL) 
	{
		 ERR("Critical error: 'display->butts.hex_content_label' equal NULL before lv_obj_add_event_cb!\n");
		 abort(); 
    	}
    	
    	
    	
	lv_indev_set_type(display->media_drv->mouse->encoder_indev, LV_INDEV_TYPE_ENCODER);
	lv_indev_set_read_cb(display->media_drv->mouse->encoder_indev, sdl_encoder_read_cb);
	lv_indev_set_display(display->media_drv->mouse->encoder_indev, display->media_drv->disp);
	lv_indev_set_user_data(display->media_drv->mouse->encoder_indev, (void*)display);
	lv_obj_add_event_cb(display->butts.hex_content_label, hex_label_event_handler, LV_EVENT_ALL, (void*)display);
	display->media_drv->mouse->encoder_diff 		= 0;
	display->media_drv->mouse->encoder_event_pending 	= false;

	
	LOG("Input device initialized and linked to display.\n");


	
	size_t buf_size = STANDART_WIDTH * STANDART_HEIGHT * sizeof(lv_color_t);
	display->media_drv->buffs->buf1 = malloc(buf_size);
	if (!display->media_drv->buffs->buf1) 
	{
		ERR("failed alloc buf1 (%zu bytes)\n", buf_size);
		goto cleanup;
	}
	
	display->media_drv->buffs->buf2 = malloc(buf_size);
	
	if (!display->media_drv->buffs->buf2) 
	{
		ERR("failed alloc buf2 (%zu bytes) - continuing with single buffer\n", buf_size);	
		//goto cleanup;
	}

	
	lv_display_set_buffers(display->media_drv->disp,
	                         display->media_drv->buffs->buf1,
	                         display->media_drv->buffs->buf2,
	                         buf_size,
	                         LV_DISPLAY_RENDER_MODE_PARTIAL); 

	lv_display_set_flush_cb(display->media_drv->disp, my_flush_cb);
	lv_display_set_user_data(display->media_drv->disp, (void*)display);

	lv_obj_t* screen = lv_screen_active(); 


	display->butts.button_open_file = lv_button_create(screen);
	lv_obj_set_size(display->butts.button_open_file, 80, 30);
	lv_obj_align(display->butts.button_open_file, LV_ALIGN_TOP_LEFT, 10, 10); 
	lv_obj_add_event_cb(
		display->butts.button_open_file,
		button_open_file_handler, 
		LV_EVENT_CLICKED,
		(void*)display
	);
	display->butts.label_for_choice_file = lv_label_create(display->butts.button_open_file);
	lv_label_set_text(display->butts.label_for_choice_file, "File"); 
	lv_obj_center(display->butts.label_for_choice_file);

	
	display->butts.button_close = lv_button_create(screen);
	lv_obj_set_size(display->butts.button_close, 80, 30); 
	
	lv_obj_align(display->butts.button_close, LV_ALIGN_TOP_RIGHT, -10, 10);
	lv_obj_add_event_cb(
		display->butts.button_close,
		button_for_close, 
		LV_EVENT_CLICKED,
		(void*)display 
	);
	display->butts.lable_for_close = lv_label_create(display->butts.button_close);
	lv_label_set_text(display->butts.lable_for_close, "Close"); 
	lv_obj_center(display->butts.lable_for_close);
	
	
	display->butts.button_change_page = lv_button_create(screen);
	lv_obj_set_size(display->butts.button_change_page, 80, 30);
	lv_obj_align(display->butts.button_change_page, LV_ALIGN_LEFT_MID, 10, 150);
	lv_obj_add_flag(display->butts.button_change_page, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_event_cb(
		display->butts.button_change_page,
		button_change_page_handler,
		LV_EVENT_CLICKED,
		(void*)display
	);
	display->butts.label_for_change_page = lv_label_create(display->butts.button_change_page);
	lv_obj_set_style_text_font(display->butts.label_for_change_page, &roboto_mono, 0);
	lv_label_set_text(display->butts.label_for_change_page, "Page");
	lv_obj_center(display->butts.label_for_change_page);
	
	
	
	display->butts.label_file_info = lv_label_create(screen);
	if (!display->butts.label_file_info)
	{
		ERR("Failed to create file info label");
        	goto cleanup;
	}
	
	lv_obj_set_size(display->butts.label_file_info, 400, LV_SIZE_CONTENT);
	lv_obj_align_to(display->butts.label_file_info, display->butts.hex_content_label, LV_ALIGN_OUT_RIGHT_MID, -400, 150);
	lv_obj_add_flag(display->butts.button_change_page, LV_OBJ_FLAG_HIDDEN);
	lv_label_set_text(display->butts.label_file_info, "");
    	lv_label_set_long_mode(display->butts.label_file_info, LV_LABEL_LONG_WRAP);
    	lv_obj_set_style_text_font(display->butts.label_file_info, &roboto_mono, 0);
    	lv_obj_set_style_text_color(display->butts.label_file_info, lv_color_black(), LV_PART_MAIN);
    	
	
	
	
	
	
	
	display->media_drv->hex_viewer_group = lv_group_create();
    	if (!display->media_drv->hex_viewer_group) 
    	{
        	ERR("Failed to create hex viewer group");
        	goto cleanup;
    	}


   	display->butts.hex_content_label = lv_label_create(screen);
    	if (!display->butts.hex_content_label)
    	{
        	ERR("Failed to create hex content label");
        	goto cleanup;
    	}

   	lv_obj_set_width(display->butts.hex_content_label, LV_PCT(95));
    	lv_obj_set_height(display->butts.hex_content_label, LV_SIZE_CONTENT);

    	lv_obj_align_to(display->butts.hex_content_label, display->butts.button_open_file, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    	lv_label_set_text(display->butts.hex_content_label, "No file loaded.");
    	lv_label_set_long_mode(display->butts.hex_content_label, LV_LABEL_LONG_WRAP);
    	lv_obj_set_style_text_font(display->butts.hex_content_label, &roboto_mono, 0);
    	lv_obj_set_style_text_color(display->butts.hex_content_label, lv_color_black(), LV_PART_MAIN);

    	lv_group_add_obj(display->media_drv->hex_viewer_group, display->butts.hex_content_label);
    	lv_obj_add_flag(display->butts.hex_content_label, LV_OBJ_FLAG_CLICKABLE); 

    	lv_obj_add_event_cb(display->butts.hex_content_label, hex_label_event_handler, LV_EVENT_ALL, (void*)display);


    	lv_group_add_obj(display->media_drv->keyboard->group, display->butts.button_open_file);
   	lv_group_add_obj(display->media_drv->keyboard->group, display->butts.button_close);

   	lv_indev_set_group(display->media_drv->mouse->encoder_indev, display->media_drv->hex_viewer_group);
   	//lv_indev_set_group(display->media_drv->keyboard->keyboard_indev, display->media_drv->hex_viewer_group);
 
   	
   	
	LOG("Display initialized successfully\n");
	return EXIT_SUCCESS;
   	

cleanup:

	ERR("Display initialization failed, cleaning up...\n");
	if (display) 
	{ 
		cleanup_disp(display);
	}
	return EXIT_FAILURE;
}



static void cleanup_disp(struct _DISPLAY* display)
{
	if (display == NULL) 
	{
        	return; 
    	}

    	if (display->media_drv && display->media_drv->buffs)
    	{
        	if (display->media_drv->buffs->buf1)
        	{
            		free(display->media_drv->buffs->buf1);
           		display->media_drv->buffs->buf1 = NULL; 
        	}
        	
        	if (display->media_drv->buffs->buf2)
        	{
            		free(display->media_drv->buffs->buf2);
            		display->media_drv->buffs->buf2 = NULL;
       	 	}

        	free(display->media_drv->buffs);
        	display->media_drv->buffs = NULL;
    	}

    	if (display->media_drv)
    	{
    	
        	if (display->media_drv->renderer)
        	{
            		SDL_DestroyRenderer(display->media_drv->renderer);
            		SDL_DestroyTexture(display->media_drv->lvgl_texture);
            		display->media_drv->renderer = NULL;
        	}
        	

        	if (display->media_drv->window)
        	{
            		SDL_DestroyWindow(display->media_drv->window);
            		display->media_drv->window = NULL;
        	}
        	
        	if (display && display->media_drv && display->media_drv->mouse->mouse_indev) 
        	{
         		lv_indev_delete(display->media_drv->mouse->mouse_indev);
        		display->media_drv->mouse->mouse_indev = NULL;
		}
		
		if (display && display->media_drv && display->media_drv->keyboard->keyboard_indev) 
        	{
         		lv_indev_delete(display->media_drv->keyboard->keyboard_indev);
        		display->media_drv->keyboard->keyboard_indev = NULL;
		}
		

  		
        	free(display->media_drv);
        	display->media_drv = NULL;
    	}

	SDL_Quit();

}

void sdl_mouse_read_cb(lv_indev_t* indev, lv_indev_data_t* data)
{
	(void)indev;
	int mouse_x, mouse_y;
	uint32_t buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
	
	data->point.x = mouse_x;
	data->point.y = mouse_y;
	
	if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		data->state = LV_INDEV_STATE_PRESSED;
	}
	else
	{
		data->state = LV_INDEV_STATE_RELEASED;

	}
}





static void page_input_dialog_ok_cb(lv_event_t* event) 
{
	lv_event_code_t code = lv_event_get_code(event);
    	struct dialog_window_for_data_file* dialog_data = (struct dialog_window_for_data_file*)lv_event_get_user_data(event);

   	 if (code == LV_EVENT_CLICKED)
   	 {
		LOG("Page input dialog: OK clicked.\n");
		if (dialog_data)
		{
			cleanup_dialog_resources(dialog_data);
		}
		
    	}
}


static void page_input_dialog_cancel_cb(lv_event_t* event) 
{
	lv_event_code_t code = lv_event_get_code(event);
    	struct dialog_window_for_data_file* dialog_data = (struct dialog_window_for_data_file*)lv_event_get_user_data(event);

    	if (code == LV_EVENT_CLICKED) 
    	{
		LOG("Page input dialog: Cancel clicked.\n");
		if (dialog_data)
		{
			cleanup_dialog_resources(dialog_data);
		}
    	}
}



static void page_input_dialog_textarea_cb(lv_event_t* event)
{
	lv_event_code_t code 	= lv_event_get_code(event);
	lv_obj_t* ta 	     	= lv_event_get_target(event);
	struct dialog_window_for_data_file* dialog_data = (struct dialog_window_for_data_file*)lv_event_get_user_data(event);
	
	if (!dialog_data)
	{
		ERR("Page input textarea event (code %d): Received NULL user_data!\n", code);
		return;
	}
	
	if (code == LV_EVENT_READY)
	{
		LOG("Page input textarea: LV_EVENT_READY received.\n");
		proccess_page_number_input(dialog_data);
	}
	else if (code == LV_EVENT_CANCEL)
	{
		LOG("Page input textarea: LV_EVENT_CANCEL received.\n");
        	if (dialog_data)
		{
			cleanup_dialog_resources(dialog_data);
		}
	}
	else if (code == LV_EVENT_DELETE)
	{
		LOG("Page input textarea LV_EVENT_DELETE received for textarea %p, user_data %p.\n", (void*)ta, (void*)dialog_data);
		if (dialog_window == dialog_data)
		{
			LOG("LV_EVENT_DELETE: Global current_page_input_dialog matches user_data. Freeing memory.\n");
			lv_free(dialog_data);
			dialog_window = NULL;
		}
		else
		{
			LOG("LV_EVENT_DELETE: Global dialog_window (%p) does NOT match user_data (%p). Freeing user_data associated with this textarea.\n", (void*)dialog_window, (void*)dialog_data);
			
		}
	}
	
}


static void proccess_page_number_input(struct dialog_window_for_data_file* dialog_data)
{
	if (!dialog_data || !dialog_data->text || !dialog_data->display || !dialog_data->display->fs || !dialog_data->display->butts.hex_content_label) 
        {
		ERR("process_page_number_input: Critical data missing.\n");
		if (dialog_data) cleanup_dialog_resources(dialog_data); 
		return;
    	} 
    	
    	const char* page_str = lv_textarea_get_text(dialog_data->text);
    	if (!page_str || strlen(page_str) == 0)
    	{
    		LOG("Page number input is empty.\n");
    		return;
    	}
    	
    	char* endptr;
    	size_t page_num = (size_t)strtol(page_str, &endptr, 10);
    	
    	if (*endptr != '\0' || page_num >= dialog_data->display->fs->total_pages)
    	{
    		ERR("Invalid page number format: '%s'. Must be < total_pages.\n", page_str);
    		lv_textarea_set_text(dialog_data->text, "");
    		lv_textarea_set_placeholder_text(dialog_data->text, "Invalid!");
    		if (dialog_data->keyboard && lv_obj_is_valid(dialog_data->keyboard)) 
    		{
             		lv_keyboard_set_textarea(dialog_data->keyboard, dialog_data->text); 
        	}
        	if (lv_obj_is_valid(dialog_data->text)) 
        	{
            		lv_group_focus_obj(dialog_data->text);
       		}
       		
       		return;
    	}
    	
    	dialog_data->display->fs->current_page = page_num;
    	if (format_and_display_page(dialog_data->display->fs, dialog_data->display->butts.hex_content_label, dialog_data->display->butts.label_file_info, dialog_data->display->fs->current_page))
    	{
    		ERR("error in format_and_display_page\n");
    		lv_label_set_text(dialog_data->display->butts.hex_content_label, "Error displaying page.");
    	}
    	
    	LOG("Switching to page %llu\n", page_num);
    	if (dialog_data) cleanup_dialog_resources(dialog_data); 
}



void button_change_page_handler(lv_event_t* event)
{
	lv_event_code_t code = lv_event_get_code(event);
	if (dialog_window != NULL) 
	{
		LOG("Dialog is already open or being processed. Ignoring click.\n");
		
		if (dialog_window->text && lv_obj_is_valid(dialog_window->text)) 
		{
			lv_group_t* group = lv_obj_get_group(dialog_window->text);
		     	if (group) lv_group_focus_obj(dialog_window->text);
		}
		return; 
    	}
	
	struct _DISPLAY* display = (struct _DISPLAY*)lv_event_get_user_data(event);
	
	lv_obj_t* old_btn = lv_event_get_target(event);
	lv_obj_t* screen = lv_screen_active();
	
	lv_obj_t* modal_bg = lv_obj_create(screen);
    	lv_obj_set_size(modal_bg, LV_PCT(100), LV_PCT(100));
    	lv_obj_set_style_bg_color(modal_bg, lv_color_black(), 0);
    	lv_obj_set_style_bg_opa(modal_bg, LV_OPA_70, 0); 
    	lv_obj_add_flag(modal_bg, LV_OBJ_FLAG_CLICKABLE); 
    	
    	lv_obj_t* dialog_cont = lv_obj_create(modal_bg);
    	lv_obj_set_size(dialog_cont, LV_PCT(80), LV_PCT(40));
    	lv_obj_center(dialog_cont);
    	lv_obj_set_flex_flow(dialog_cont, LV_FLEX_FLOW_COLUMN); 
    	lv_obj_set_flex_align(dialog_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    	
    	lv_obj_t * ta = lv_textarea_create(dialog_cont);
    	lv_textarea_set_one_line(ta, true);
    	lv_obj_set_width(ta, LV_PCT(90));
    	lv_textarea_set_placeholder_text(ta, "Enter page...");
    	
    	lv_obj_t * btn_cont = lv_obj_create(dialog_cont);
    	lv_obj_remove_style_all(btn_cont); 
    	lv_obj_set_width(btn_cont, LV_PCT(100));
    	lv_obj_set_height(btn_cont, LV_SIZE_CONTENT);
    	lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW); 
    	lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); 
    	
    	lv_obj_t * ok_btn = lv_button_create(btn_cont);
    	lv_obj_t * ok_label = lv_label_create(ok_btn);
    	lv_label_set_text(ok_label, "OK");
    	
    	lv_obj_t * cancel_btn = lv_button_create(btn_cont);
    	lv_obj_t * cancel_label = lv_label_create(cancel_btn);
    	lv_label_set_text(cancel_label, "Cancel");
    	
    	lv_obj_t * kb = lv_keyboard_create(screen);
    	lv_keyboard_set_textarea(kb, ta);
	
	
	dialog_window = lv_malloc(sizeof (struct dialog_window_for_data_file));
	if (!dialog_window)	
	{
		ERR("Failed to allocate user data for dialog");
		lv_obj_delete(modal_bg);
       		lv_obj_delete(kb);
       		exit(EXIT_FAILURE);
	}
	
	dialog_window->text = ta;
	dialog_window->keyboard = kb;
	dialog_window->cont = modal_bg;
	dialog_window->display = display;
	
	
	lv_obj_add_event_cb(ok_btn, page_input_dialog_ok_cb, LV_EVENT_CLICKED, dialog_window);
   	lv_obj_add_event_cb(cancel_btn, page_input_dialog_cancel_cb, LV_EVENT_CLICKED, dialog_window);
   	lv_obj_add_event_cb(ta, page_input_dialog_textarea_cb, LV_EVENT_ALL, dialog_window);
   	
   	LOG("Adding textarea %p to group %p\n", (void*)ta, (void*)display->media_drv->keyboard->group);
    	lv_group_add_obj(display->media_drv->keyboard->group, ta); 
    	lv_group_add_obj(display->media_drv->keyboard->group, ok_btn);
    	lv_group_add_obj(display->media_drv->keyboard->group, cancel_btn);
    	lv_group_add_obj(display->media_drv->keyboard->group, kb);
    	
    	if (display->media_drv && display->media_drv->keyboard && display->media_drv->keyboard->keyboard_indev && display->media_drv->keyboard->group) 
    	{
        	lv_indev_set_group(display->media_drv->keyboard->keyboard_indev, display->media_drv->keyboard->group);
        	LOG("Keyboard indev group set to DIALOG group: %p\n", (void*)display->media_drv->keyboard->group);
    	} 
    	else 
    	{
        	ERR("Failed to set keyboard indev group for dialog - critical components missing!\n");
    	}
    	
    	lv_group_focus_obj(ta);
    	LOG("Focused object is now: %p\n", (void*)lv_group_get_focused(display->media_drv->keyboard->group));
	
	
	
}


static int format_and_display_page(struct _FILE_STATE* fs, lv_obj_t* label, lv_obj_t* label_file_info, size_t page_num) 
{
	LOG("call format_and_display_page for page_num: %zu\n", page_num);
    	if (!fs || !label || !label_file_info) 
    	{ 
		ERR("Invalid arguments to format_and_display_page (fs: %p, label: %p, label_file_info: %p)\n", (void*)fs, (void*)label, (void*)label_file_info);
		return EXIT_FAILURE;
    	}
    
    	if (fs->bytes_per_page == 0 && fs->size_file > 0) 
    	{
        	fs->bytes_per_page = LINES_PER_PAGE * BYTES_PER_LINE;
       	 	LOG("Warning: fs->bytes_per_page was 0 in format_and_display_page. Set to default: %zu\n", fs->bytes_per_page);
         	if (fs->bytes_per_page == 0) 
         	{
             		ERR("Cannot operate with fs->bytes_per_page = 0 for non-empty file (format).\n");
             		lv_label_set_text(label, "Error: Config");
             		return EXIT_FAILURE;
        	}
    	}
    
    	if (fs->size_file == 0) 
    	{
        	fs->total_pages = 1;
    	} 
    	else if (fs->bytes_per_page > 0) 
    	{
        	fs->total_pages = (fs->size_file + fs->bytes_per_page - 1) / fs->bytes_per_page;
        	if (fs->total_pages == 0) fs->total_pages = 1;
    	} 
    	else 
    	{
        	fs->total_pages = 0; 
    	}


    	if (fs->size_file == 0) 
    	{
        	LOG("File is empty, displaying (Empty) message.\n");
        	if (fs->page_buffer && fs->page_buffer_size > strlen("(Empty)")) 
        	{
            
            		strcpy(fs->page_buffer, "(Empty)");
            		lv_label_set_text(label, fs->page_buffer);
        	} 
        	else 
        	{
             		lv_label_set_text(label, "Error: No Buffer");
        	}
        	
        	fs->current_page = 0;
        	get_log_file_data(fs); 
       	 	lv_label_set_text(label_file_info, fs->result_data_string);
       		return EXIT_SUCCESS;
   	 }


    	if (fs->total_pages == 0) 
    	{ 
		ERR("fs->total_pages is 0 for a non-empty file. Cannot display page.\n");
		lv_label_set_text(label, "Error: No Pages");
		return EXIT_FAILURE;
    	}	
   	if (page_num >= fs->total_pages) 
    	{
        	LOG("page_num %zu is out of bounds (total_pages %zu). Clamping.\n", page_num, fs->total_pages);
        	page_num = fs->total_pages > 0 ? fs->total_pages - 1 : 0;
    	}
    	
    	
    	fs->current_page = page_num; 

    	if (getting_bytes_from_a_file(fs, fs->current_page) != EXIT_SUCCESS) 
    	{
		ERR("Failed to get bytes from file for page %zu\n", fs->current_page);
		lv_label_set_text(label, "Error: Read Fail");
		return EXIT_FAILURE;
   	}


    	if (!fs->file_bytes || !fs->page_buffer) 
    	{
		ERR("File data (fs->file_bytes %p) or page buffer (fs->page_buffer %p) is missing\n", (void*)fs->file_bytes, (void*)fs->page_buffer);
		lv_label_set_text(label, "Error: Data/Buf Nil");
		return EXIT_FAILURE;
    	}
    
    	size_t page_absolute_start_in_file = fs->current_page * fs->bytes_per_page;
    	size_t bytes_on_this_page;
    
    	if (page_absolute_start_in_file >= fs->size_file) 
    	{
		bytes_on_this_page = 0; 
		LOG("Start offset %zu for page %zu is beyond file size %zu. Displaying empty.\n",
            	page_absolute_start_in_file, fs->current_page, fs->size_file);
    	} 
    	else if (page_absolute_start_in_file + fs->bytes_per_page > fs->size_file) 
    	{
        	bytes_on_this_page = fs->size_file - page_absolute_start_in_file;
    	} 
    	else 
    	{
        	bytes_on_this_page = fs->bytes_per_page;
    	}

    	LOG("Formatting page %zu. Absolute start in file: %zu. Bytes on this page: %zu.\n",
        	fs->current_page, page_absolute_start_in_file, bytes_on_this_page);
    	LOG("Current chunk in fs->file_bytes starts at file offset %zu and has %zu bytes.\n",
        	fs->offset_bytes_of_file, fs->offset_bytes_of_arr);

    	char* p_buf = fs->page_buffer;
    	size_t remaining_buf_size = fs->page_buffer_size;
    	int written;

    	if (remaining_buf_size == 0) 
    	{
		ERR("fs->page_buffer_size is 0!\n");
		return EXIT_FAILURE;
    	}
    	p_buf[0] = '\0'; 

    	if (bytes_on_this_page == 0 && page_absolute_start_in_file >= fs->size_file) 
    	{
        	LOG("Page %zu is effectively empty.\n", fs->current_page);    
    	}

    	char ascii_part[BYTES_PER_LINE + 1];
    	int ascii_idx = 0;

    	for (size_t i = 0; i < bytes_on_this_page; ++i) 
    	{
        	size_t current_byte_absolute_offset_in_file = page_absolute_start_in_file + i;
        

		if (current_byte_absolute_offset_in_file < fs->offset_bytes_of_file) 
		{
			ERR("Logic error: current byte %zu is before current chunk start %zu.\n", current_byte_absolute_offset_in_file, fs->offset_bytes_of_file);
		     	goto buffer_error; 
		}
        	size_t current_byte_offset_in_buffer = current_byte_absolute_offset_in_file - fs->offset_bytes_of_file;

 
        	if (current_byte_offset_in_buffer >= fs->offset_bytes_of_arr) 
        	{
            		ERR("Accessing fs->file_bytes out of loaded bounds! index_in_buffer: %zu, chunk_data_size: %zu (page %zu, byte %zu of page)\n",
                		current_byte_offset_in_buffer, fs->offset_bytes_of_arr, fs->current_page, i);
   
            		strncat(p_buf, " [READ ERR]", remaining_buf_size - strlen(p_buf) -1);
            		goto buffer_error; 
        	}

		unsigned char current_byte_value = fs->file_bytes[current_byte_offset_in_buffer];
		size_t index_in_line = i % BYTES_PER_LINE;

		if (index_in_line == 0) 
		{ 
			if (i > 0) 
		    	{ 
				ascii_part[ascii_idx] = '\0';
				written = snprintf(p_buf, remaining_buf_size, " | %s\n", ascii_part);
				if (written < 0 || (size_t)written >= remaining_buf_size) goto buffer_error;
				p_buf += written;
				remaining_buf_size -= written;
		    	}

		    	written = snprintf(p_buf, remaining_buf_size, "%04zX: ", current_byte_absolute_offset_in_file); 
		    	if (written < 0 || (size_t)written >= remaining_buf_size) goto buffer_error;
		    	p_buf += written;
		    	remaining_buf_size -= written;
		    	ascii_idx = 0;
		}

 
		written = snprintf(p_buf, remaining_buf_size, "%02X ", current_byte_value);
		if (written < 0 || (size_t)written >= remaining_buf_size) goto buffer_error;
		p_buf += written;
		remaining_buf_size -= written;

		if (index_in_line == (BYTES_PER_LINE / 2) - 1 ) 
		{ 
			if (BYTES_PER_LINE > 1) 
		     	{ 
				written = snprintf(p_buf, remaining_buf_size, "| ");
				if (written < 0 || (size_t)written >= remaining_buf_size) goto buffer_error;
				p_buf += written;
				remaining_buf_size -= written;
		     	}
		}

        
        	if (current_byte_value >= 32 && current_byte_value <= 126) 
        	{
            		ascii_part[ascii_idx++] = (char)current_byte_value;
        	} 
        	else 
        	{
            		ascii_part[ascii_idx++] = '.';
        	}
    	}

    	if (bytes_on_this_page > 0) 
    	{
		ascii_part[ascii_idx] = '\0';
		size_t last_line_bytes = bytes_on_this_page % BYTES_PER_LINE;
		if (last_line_bytes == 0 && bytes_on_this_page > 0) last_line_bytes = BYTES_PER_LINE;

		if (last_line_bytes < BYTES_PER_LINE) 
		{
			size_t padding_chars_needed = (BYTES_PER_LINE - last_line_bytes) * 3; 
		    	if (BYTES_PER_LINE > 1 && last_line_bytes < (BYTES_PER_LINE / 2)) 
		    	{ 
		        	padding_chars_needed += 2; 
		    	}
		    	if (padding_chars_needed < remaining_buf_size) 
		    	{
				memset(p_buf, ' ', padding_chars_needed);
				p_buf += padding_chars_needed;
				remaining_buf_size -= padding_chars_needed;
		    	} 
		    	else 
		    	{
	    
		        	LOG("Not enough space for last line padding.\n");
		    	}
		}
		
		written = snprintf(p_buf, remaining_buf_size, " | %s", ascii_part); 
		if (written < 0 || (size_t)written >= remaining_buf_size) goto buffer_error;
		p_buf += written;
        
    	}
 
    
    	*p_buf = '\0'; 

    	get_log_file_data(fs); 
    	lv_label_set_text(label_file_info, fs->result_data_string);
    	lv_label_set_text(label, fs->page_buffer);

    	LOG("Page formatting successful.\n");
    	return EXIT_SUCCESS;

buffer_error:
    	ERR("Buffer overflow or snprintf error during page formatting (page %zu)\n", fs->current_page);
    	LOG("******************DUMP******************\n");
   	LOG("pointer to page_buffer: %p\n", (void*)fs->page_buffer);
    	LOG("page_buffer_size: %zu\n", fs->page_buffer_size);
    	LOG("bytes_per_page (logical): %zu\n", fs->bytes_per_page);
    	LOG("remaining_buf_size for page_buffer: %zu\n", remaining_buf_size);
    	LOG("current_page: %zu\n", fs->current_page);
    	LOG("total_pages: %zu\n", fs->total_pages);
    	LOG("fs->offset_bytes_of_file (chunk start): %zu\n", fs->offset_bytes_of_file);
    	LOG("fs->offset_bytes_of_arr (chunk size): %zu\n", fs->offset_bytes_of_arr);
    	LOG("\n\n");

    	if (fs->page_buffer && fs->page_buffer_size > 0) 
    	{
        	fs->page_buffer[fs->page_buffer_size - 1] = '\0';
	}
    	lv_label_set_text(label, fs->page_buffer); 

   	 return EXIT_FAILURE;
}



void get_log_file_data(struct _FILE_STATE* fs)
{
	snprintf(fs->result_data_string, 512, "*** FILE INFO ***\nFile name: %s\nSize: %llu\nPages:  %llu / %llu", 
		fs->filename,
		fs->size_file,
		fs->current_page,
		fs->total_pages - 1
		

	);
	
	LOG("get data success!\n");
	//LOG("%s\n", fs->result_data_string);
	
}	





void hex_label_event_handler(lv_event_t* event) 
{
	lv_event_code_t code = lv_event_get_code(event);
    	struct _DISPLAY* display = (struct _DISPLAY*)lv_event_get_user_data(event);
    	lv_obj_t* label = lv_event_get_target(event);
    	LOG("hex_label_event_handler: Received event code %d on label %p\n", code, (void*)label);

    	if (!display || !display->fs) return;

    	lv_group_t* hex_group = display->media_drv->hex_viewer_group; 
    	if (hex_group) 
    	{
         	LOG("  Current focused obj in hex_group: %p\n", (void*)lv_group_get_focused(hex_group));
    	}

    	if (code == LV_EVENT_KEY) 
    	{
        	uint32_t key = lv_event_get_key(event);
        	LOG("  --> LV_EVENT_KEY received! Key: %u (%s)\n", key,
             	(key == LV_KEY_NEXT) ? "LV_KEY_NEXT" :
             	(key == LV_KEY_PREV) ? "LV_KEY_PREV" :
             	(key == LV_KEY_UP) ? "LV_KEY_UP" :
             	(key == LV_KEY_HOME) ? "LV_KEY_HOME" :
             	(key == LV_KEY_END) ? "LV_KEY_END" :
             	(key == LV_KEY_DOWN) ? "LV_KEY_DOWN" : "Other");

        	size_t current_page = display->fs->current_page;
        	size_t total_pages = display->fs->total_pages;
        	bool page_changed = false;
        	size_t next_page = current_page;

        	if (key == LV_KEY_NEXT || key == LV_KEY_DOWN) 
        	{
        		if (current_page + 1 < total_pages) 
            		{
               			next_page = current_page + 1;
                		page_changed = true;
            		}  
        	}
        	else if (key == LV_KEY_PREV || key == LV_KEY_UP) 
        	{
        		if (current_page > 0) 
		     	{
		        	next_page = current_page - 1;
		        	page_changed = true;
		    	} 
        	}
        	else if (key == LV_KEY_HOME) 
        	{
        		if (current_page != 0) 
		     	{
		         	next_page = 0;
		         	page_changed = true;
		     	}
        	}
        
        	else if (key == LV_KEY_END) 
        	{
        		if (total_pages > 0 && current_page != total_pages - 1)
		      	{
				 next_page = total_pages - 1;
				 page_changed = true;
		     	}
        	}
        
       

        	if (page_changed) 
        	{
            		LOG("  --> Page change triggered! current=%zu, total=%zu, next=%zu\n", current_page, total_pages, next_page);
            		format_and_display_page(display->fs, label, display->butts.label_file_info, next_page);
        	} 
        	else 
        	{
             		LOG("  --> Key pressed, but page not changed (current=%zu, total=%zu)\n", current_page, total_pages);
        	}
    	}
    	else if (code == LV_EVENT_FOCUSED) 
    	{
		LOG("  --> LV_EVENT_FOCUSED\n");
		lv_obj_set_style_outline_width(label, 2, LV_PART_MAIN | LV_STATE_FOCUSED);
		lv_obj_set_style_outline_color(label, lv_theme_get_color_primary(label), LV_PART_MAIN | LV_STATE_FOCUSED);
		lv_obj_set_style_outline_pad(label, 2, LV_PART_MAIN | LV_STATE_FOCUSED);
    	}	 
    	else if (code == LV_EVENT_DEFOCUSED) 
    	{
		LOG("  --> LV_EVENT_DEFOCUSED\n");
		lv_obj_set_style_outline_width(label, 0, LV_PART_MAIN | LV_STATE_FOCUSED);
    	}
    
}





static lv_key_t sdl_keycode_to_lvgl(SDL_Keycode sdl_key, SDL_Keymod sdl_mod)
{
	 bool is_shift = (sdl_mod & KMOD_SHIFT);


    	switch (sdl_key) 
    	{
		case SDLK_RETURN:
		case SDLK_KP_ENTER:     return LV_KEY_ENTER;
		case SDLK_BACKSPACE:    return LV_KEY_BACKSPACE;
		case SDLK_ESCAPE:       return LV_KEY_ESC;
		case SDLK_DELETE:       return LV_KEY_DEL;
		case SDLK_UP:           return LV_KEY_UP;
		case SDLK_DOWN:         return LV_KEY_DOWN;
		case SDLK_LEFT:         return LV_KEY_LEFT;
		case SDLK_RIGHT:        return LV_KEY_RIGHT;
		case SDLK_HOME:         return LV_KEY_HOME;
		case SDLK_END:          return LV_KEY_END;
		case SDLK_PAGEDOWN:     return LV_KEY_NEXT; 
		case SDLK_PAGEUP:       return LV_KEY_PREV; 
		case SDLK_TAB:          return is_shift ? LV_KEY_PREV : LV_KEY_NEXT; 


		default: break; 
    	}


    	if (sdl_key >= SDLK_SPACE && sdl_key <= SDLK_z) 
    	{
        	if (sdl_key >= SDLK_a && sdl_key <= SDLK_z) 
        	{
            		if (is_shift || (sdl_mod & KMOD_CAPS)) 
            		{
                		return (lv_key_t)(sdl_key - SDLK_a + 'A'); 
            		} 
            		else 
            		{
                		return (lv_key_t)(sdl_key); 
            		}
        	}
        
        	else if (sdl_key >= SDLK_0 && sdl_key <= SDLK_9) 
        	{
            		if (is_shift) 
            		{
                		switch (sdl_key) 
                		{		
					case SDLK_0: return ')';
				    	case SDLK_1: return '!';
				    	case SDLK_2: return '@';
				    	case SDLK_3: return '#';
				    	case SDLK_4: return '$';
				    	case SDLK_5: return '%';
				    	case SDLK_6: return '^';
				    	case SDLK_7: return '&';
				    	case SDLK_8: return '*';
				    	case SDLK_9: return '(';
				    	default: break;
                		}
            		} 
            		else 
            		{
               			return (lv_key_t)(sdl_key); 
            		}
        	}

        	else if (is_shift) 
        	{
        	 
             		switch (sdl_key) 
             		{
				case SDLK_BACKQUOTE:    return '~';
				case SDLK_MINUS:        return '_';
				case SDLK_EQUALS:       return '+';
				case SDLK_LEFTBRACKET:  return '{';
				case SDLK_RIGHTBRACKET: return '}';
				case SDLK_BACKSLASH:    return '|';
				case SDLK_SEMICOLON:    return ':';
				case SDLK_QUOTE:        return '"';
				case SDLK_COMMA:        return '<';
				case SDLK_PERIOD:       return '>';
				case SDLK_SLASH:        return '?';
				default: break;
             
           		}
        	} 
        	else 
        	{
         
            		switch (sdl_key) 
            		{
				case SDLK_SPACE:        return ' ';
				case SDLK_BACKQUOTE:    return '`';
				case SDLK_MINUS:        return '-';
				case SDLK_EQUALS:       return '=';
				case SDLK_LEFTBRACKET:  return '[';
				case SDLK_RIGHTBRACKET: return ']';
				case SDLK_BACKSLASH:    return '\\';
				case SDLK_SEMICOLON:    return ';';
				case SDLK_QUOTE:        return '\'';
				case SDLK_COMMA:        return ',';
				case SDLK_PERIOD:       return '.';
				case SDLK_SLASH:        return '/';
				default: break;
            		}
        	}
    	}

    
	return 0;
}


static bool encoder_pressed_reported = false;

void sdl_encoder_read_cb(lv_indev_t* indev, lv_indev_data_t* data) 
{
    struct _DISPLAY* display = (struct _DISPLAY*)lv_indev_get_user_data(indev);
    if (!display || !display->media_drv || !display->media_drv->mouse) 
    {
         LOG("sdl_encoder_read_cb: ERROR - Invalid pointers!\n");
         data->enc_diff = 0;
         data->key = 0; 
         data->state = LV_INDEV_STATE_RELEASED;
         data->continue_reading = false;
         encoder_pressed_reported = false;

         return;
    }
    struct _mouse* mouse_drv = display->media_drv->mouse;


    if (mouse_drv->encoder_diff != 0) 
    {

        data->state = LV_INDEV_STATE_PRESSED;


        if (mouse_drv->encoder_diff > 0) 
        { 
             data->key = LV_KEY_NEXT; 
             LOG("sdl_encoder_read_cb: Diff=%d --> Reporting PRESSED with key=LV_KEY_NEXT (%u)\n", mouse_drv->encoder_diff, data->key);
        } 
        else 
        { 
             data->key = LV_KEY_PREV; 
             LOG("sdl_encoder_read_cb: Diff=%d --> Reporting PRESSED with key=LV_KEY_PREV (%u)\n", mouse_drv->encoder_diff, data->key);
        }

        data->enc_diff = 0;

        mouse_drv->encoder_diff = 0;
        encoder_pressed_reported = true; 
    } 
    else 
    {

        if (encoder_pressed_reported) 
        {

            data->state = LV_INDEV_STATE_RELEASED;
           
            data->key = 0; 
            data->enc_diff = 0;
            encoder_pressed_reported = false; 
            
            LOG("sdl_encoder_read_cb: No Diff --> Reporting RELEASE (after press)\n");
        } 
        else 
        {
            
            data->state = LV_INDEV_STATE_RELEASED;
            data->key = 0;
            data->enc_diff = 0;
        }
    }

    data->continue_reading = false;
}




void sdl_keyboard_read_cb(lv_indev_t* indev, lv_indev_data_t* data)
{

	(void)indev;
	
	struct _DISPLAY* display = (struct _DISPLAY*)lv_indev_get_user_data(indev);
	if (!display->media_drv->keyboard->key_event_processed)
    	{
		
		SDL_Keymod current_mods = SDL_GetModState();
		data->key = sdl_keycode_to_lvgl(display->media_drv->keyboard->last_sdl_keycode, current_mods);
		data->state = display->media_drv->keyboard->last_key_state;

		display->media_drv->keyboard->key_event_processed = true;

		LOG("sdl_keyboard_read_cb: SENT Event to LVGL -> key=0x%x ('%c'), state=%s\n",
		    (unsigned int)data->key,
		    (data->key >= 32 && data->key <= 126) ? (char)data->key : '?',
		    (data->state == LV_INDEV_STATE_PRESSED) ? "PRESSED" : "RELEASED");
    	}
	else
	{
		SDL_Keymod current_mods = SDL_GetModState();
		data->key = sdl_keycode_to_lvgl(display->media_drv->keyboard->last_sdl_keycode, current_mods);
		data->state = LV_INDEV_STATE_RELEASED;
		
		if (display->media_drv->keyboard->last_key_state == LV_INDEV_STATE_RELEASED) 
		{
			data->key = 0;
		}
	}
	
	data->continue_reading = false;
}





void my_flush_cb(lv_display_t * disp, const lv_area_t * area, unsigned char * color_p)
{
        struct _DISPLAY* display = (struct _DISPLAY*)lv_display_get_user_data(disp);
        if (!display || !display->media_drv || !display->media_drv->lvgl_texture) 
        {

            lv_display_flush_ready(disp);
            return;
        }

        SDL_Rect sdl_area;
        sdl_area.x = area->x1;
        sdl_area.y = area->y1;
        sdl_area.w = lv_area_get_width(area);
        sdl_area.h = lv_area_get_height(area);

        int lvgl_pitch = lv_area_get_width(area) * 2;

        int result = SDL_UpdateTexture(
            display->media_drv->lvgl_texture,
            &sdl_area,    
            color_p,      
            lvgl_pitch    
        );

        if (result != 0) 
        {
        	ERR("SDL_UpdateTexture failed: %s\n", SDL_GetError());
        }

  
        lv_display_flush_ready(disp);
}



static void cleanup_dialog_resources(struct dialog_window_for_data_file* dlg_data) 
{
	if (!dlg_data) 
    	{
		LOG("Cleanup called with NULL data.\n");
		return;
    	}

    	LOG("Cleanup requested for dialog data %p. Keyboard=%p, Cont=%p, Textarea=%p\n",
        		(void*)dlg_data, (void*)dlg_data->keyboard, (void*)dlg_data->cont, (void*)dlg_data->text);


    	lv_group_t * group = NULL;
    	if (dlg_data->text && lv_obj_is_valid(dlg_data->text)) 
    	{
         	group = lv_obj_get_group(dlg_data->text);
    	}

   
    	if (dlg_data->keyboard && lv_obj_is_valid(dlg_data->keyboard)) 
    	{
		LOG("Requesting async delete for keyboard %p\n", (void*)dlg_data->keyboard);
		lv_obj_delete_async(dlg_data->keyboard);
    	} 
    	else if (dlg_data->keyboard) 
    	{
         	LOG("Keyboard object %p was already invalid.\n", (void*)dlg_data->keyboard);
    	}	

    
    	if (dlg_data->cont && lv_obj_is_valid(dlg_data->cont)) 
    	{
		LOG("Requesting async delete for container %p and its children\n", (void*)dlg_data->cont);
		lv_obj_delete_async(dlg_data->cont);
    	} 
    	else if (dlg_data->cont) 
    	{
         	LOG("Container object %p was already invalid.\n", (void*)dlg_data->cont);
    	}
    	
    	
    	if (dlg_data && dlg_data->display && dlg_data->display->butts.hex_content_label &&
        	lv_obj_is_valid(dlg_data->display->butts.hex_content_label) &&
        	dlg_data->display->media_drv && dlg_data->display->media_drv->hex_viewer_group &&
        	dlg_data->display->media_drv->keyboard && dlg_data->display->media_drv->keyboard->keyboard_indev)
    	{
		lv_indev_set_group(dlg_data->display->media_drv->keyboard->keyboard_indev, dlg_data->display->media_drv->hex_viewer_group); 
		lv_indev_set_group(dlg_data->display->media_drv->mouse->encoder_indev, dlg_data->display->media_drv->hex_viewer_group);     

		
		lv_group_focus_obj(dlg_data->display->butts.hex_content_label);

		LOG("Focus explicitly set to hex label (%p) in hex group (%p). Keyboard and mouse indev groups also set to hex_viewer_group.\n",
		    (void*)dlg_data->display->butts.hex_content_label,
		    (void*)dlg_data->display->media_drv->hex_viewer_group);
    	} 
    	else 
    	{

        if (dlg_data && dlg_data->display && dlg_data->display->media_drv &&
            	dlg_data->display->media_drv->keyboard && dlg_data->display->media_drv->keyboard->group && 
            	dlg_data->display->media_drv->keyboard->keyboard_indev)
        {
        	lv_indev_set_group(dlg_data->display->media_drv->keyboard->keyboard_indev, dlg_data->display->media_drv->keyboard->group); 
            	if (dlg_data->display->butts.button_open_file && lv_obj_is_valid(dlg_data->display->butts.button_open_file))
            	{
                	lv_group_focus_obj(dlg_data->display->butts.button_open_file);
                	LOG("Focus set back to File button (fallback). Keyboard indev group set to default button group.\n");
            	} 
             	else 
             	{
                	LOG("Could not set focus (fallback).\n");
             	}
        }
    }

    
    	if (group) 
    	{
        	//lv_group_focus_freeze(group, false); 
        
        	if (dlg_data->display && dlg_data->display->butts.button_open_file && lv_obj_is_valid(dlg_data->display->butts.button_open_file)) 
        	{
            		lv_group_focus_obj(dlg_data->display->butts.button_open_file);
            		LOG("Focus explicitly set back to File button (%p).\n", (void*)dlg_data->display->butts.button_open_file);
        	} 
        	else 
        	{
            
            		lv_group_focus_obj(lv_screen_active());
            		LOG("Focus set back to screen (fallback).\n");
        	}
    	} 
    	else 
    	{
         	LOG("Could not get group from textarea, focus not managed.\n");
    	}

    	if (dialog_window == dlg_data) 
    	{
		LOG("Resetting global dialog_window pointer and freeing structure %p\n", (void*)dlg_data);
		lv_free(dlg_data); 
		dialog_window = NULL; 
    	} 
    	else 
    	{
        
        	LOG("Warning: Cleaning up dlg_data %p which is not the current global dialog_window %p. Freeing dlg_data %p anyway.\n",
            		(void*)dlg_data, (void*)dialog_window, (void*)dlg_data);
        	lv_free(dlg_data); 
        
    }
}



static void dialog_cancel_event_cb(lv_event_t *event) 
{
	lv_event_code_t code = lv_event_get_code(event);
    	struct dialog_window_for_data_file* user_data = (struct dialog_window_for_data_file*)lv_event_get_user_data(event);

    	if (code == LV_EVENT_CLICKED && user_data) 
    	{
        	LOG("File dialog cancelled. Closing dialog.\n");

        	
         	if (user_data->cont) 
         	{
         		LOG("Textarea LV_EVENT_CANCEL received. Cleaning up dialog...\n");
            		cleanup_dialog_resources(user_data);
        	}

    	}
    	


}


static void dialog_textarea_event_cb(lv_event_t* event)
{
	lv_event_code_t code = lv_event_get_code(event);
    	lv_obj_t* ta = lv_event_get_target(event);
    	struct dialog_window_for_data_file* user_data = (struct dialog_window_for_data_file*)lv_event_get_user_data(event);

    
    	if (!user_data) 
    	{
		ERR("Textarea event (code %d): Received NULL user_data!\n", code);
		return;
    	}

    	if (code == LV_EVENT_READY)
    	{
        	LOG("Textarea LV_EVENT_READY received (Keyboard Enter/Tick).\n");


        	if (!user_data->display || !user_data->display->fs || !user_data->display->butts.hex_content_label || !user_data->text) 
        	{
		     	ERR("Textarea event LV_EVENT_READY: Invalid user data fields!\n");

		     	cleanup_dialog_resources(user_data);
		     	return;
        	}

        	const char *filename = lv_textarea_get_text(user_data->text);
        	LOG("Filename entered: %s\n", filename ? filename : "<empty>");


       
        	if (filename && strlen(filename) > 0)
        	{
            		printf("Attempting to process file: %s\n", filename);
            		if (open_file(user_data->display->fs, filename) == 0)
            		{
 
		             			if (user_data->display->fs->file_bytes || user_data->display->fs->size_file == 0) 
						{
						    LOG("Formatting and displaying first page.\n");
						    
						    if (format_and_display_page(user_data->display->fs,
										user_data->display->butts.hex_content_label,
										user_data->display->butts.label_file_info,
										0) != 0) 
						    {
						    	 ERR("Failed to display first page.");
							 lv_label_set_text(user_data->display->butts.hex_content_label, "Error displaying data.");
						    }
						    
						    lv_obj_clear_flag(user_data->display->butts.button_change_page, LV_OBJ_FLAG_HIDDEN);
						    lv_obj_clear_flag(user_data->display->butts.label_file_info, LV_OBJ_FLAG_HIDDEN);
						    
						    
						}
						else
						{
						    ERR("File bytes are NULL after getting_bytes_from_a_file (and file not empty).\n");
						    lv_label_set_text(user_data->display->butts.hex_content_label, "Error: Failed to load data.");
						}

                				//user_data->display->fs->data_file = NULL;
                	}
                	
            		
            		else
            		{
                		ERR("Failed to open file: %s\n", filename);
                		char error_msg[256];
                		snprintf(error_msg, sizeof(error_msg), "Error: Could not open file\n'%s'", filename);
                		lv_label_set_text(user_data->display->butts.hex_content_label, error_msg);
                		lv_obj_add_flag(user_data->display->butts.button_change_page, LV_OBJ_FLAG_HIDDEN);
            		}
        	}
        	else
        	{
            		LOG("Filename is empty, doing nothing.\n");
            		lv_label_set_text(user_data->display->butts.hex_content_label, "No file loaded.");
        	}
       

        	LOG("Processing complete for LV_EVENT_READY. Cleaning up dialog...\n");
		if (user_data) 
		{
			cleanup_dialog_resources(user_data); 
		}
        	
        

	}
    	else if (code == LV_EVENT_CANCEL) 
    	{
         	LOG("Textarea LV_EVENT_CANCEL received. Cleaning up dialog...\n");
         	cleanup_dialog_resources(user_data);
    	}

    	else if (code == LV_EVENT_DELETE)
    	{
        
         	LOG("Textarea LV_EVENT_DELETE received for textarea %p.\n", (void*)ta);
         	if(dialog_window == user_data) 
         	{
		      LOG("LV_EVENT_DELETE: Global dialog_window points to the data of the deleted textarea. Resetting global pointer and freeing memory %p.\n",
		      (void*)user_data);
		      lv_free(user_data); 
		      dialog_window = NULL;
         	} 
         	else 
         	{
              		LOG("LV_EVENT_DELETE: Global dialog_window does NOT point to the user_data %p of the deleted textarea. Assuming memory was already freed or managed elsewhere.\n", (void*)user_data);
         	}
    	}

}


static void dialog_ok_event_cb(lv_event_t* event) 
{
	lv_event_code_t code = lv_event_get_code(event);
    	struct dialog_window_for_data_file* dialog_data = (struct dialog_window_for_data_file*)lv_event_get_user_data(event);

   	 if (code == LV_EVENT_CLICKED)
   	 {
		LOG("dialog event: OK clicked.\n");
		if (dialog_data)
		{
			cleanup_dialog_resources(dialog_data);
		}
		
    	}
}



void button_open_file_handler(lv_event_t* event)
{
	LOG("Cliecked on choice file!\n");
	if (dialog_window != NULL) 
	{
		LOG("Dialog is already open or being processed. Ignoring click.\n");
		
		if (dialog_window->text && lv_obj_is_valid(dialog_window->text)) 
		{
			lv_group_t* group = lv_obj_get_group(dialog_window->text);
		     	if (group) lv_group_focus_obj(dialog_window->text);
		}
		return; 
    	}
	struct _DISPLAY* display = (struct _DISPLAY*)lv_event_get_user_data(event);
	//if (display->fs->data_file) {fclose(display->fs->data_file);}
	
	lv_obj_t* old_btn = lv_event_get_target(event);
	lv_obj_t* screen = lv_screen_active();
	
	lv_obj_t* modal_bg = lv_obj_create(screen);
    	lv_obj_set_size(modal_bg, LV_PCT(100), LV_PCT(100));
    	lv_obj_set_style_bg_color(modal_bg, lv_color_black(), 0);
    	lv_obj_set_style_bg_opa(modal_bg, LV_OPA_70, 0); 
    	lv_obj_add_flag(modal_bg, LV_OBJ_FLAG_CLICKABLE); 

    	lv_obj_t* dialog_cont = lv_obj_create(modal_bg);
    	lv_obj_set_size(dialog_cont, LV_PCT(80), LV_PCT(40));
    	lv_obj_center(dialog_cont);
    	lv_obj_set_flex_flow(dialog_cont, LV_FLEX_FLOW_COLUMN); 
    	lv_obj_set_flex_align(dialog_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    	lv_obj_t * ta = lv_textarea_create(dialog_cont);
    	lv_textarea_set_one_line(ta, true);
    	lv_obj_set_width(ta, LV_PCT(90));
    	lv_textarea_set_placeholder_text(ta, "Enter path to file...");

    	lv_obj_t * btn_cont = lv_obj_create(dialog_cont);
    	lv_obj_remove_style_all(btn_cont); 
    	lv_obj_set_width(btn_cont, LV_PCT(100));
    	lv_obj_set_height(btn_cont, LV_SIZE_CONTENT);
    	lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW); 
    	lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); 

    	lv_obj_t * ok_btn = lv_button_create(btn_cont);
    	lv_obj_t * ok_label = lv_label_create(ok_btn);
    	lv_label_set_text(ok_label, "OK");
    	
    	lv_obj_t * cancel_btn = lv_button_create(btn_cont);
    	lv_obj_t * cancel_label = lv_label_create(cancel_btn);
    	lv_label_set_text(cancel_label, "Cancel");
    	
    	lv_obj_t * kb = lv_keyboard_create(screen);
    	lv_keyboard_set_textarea(kb, ta);

	
	dialog_window = lv_malloc(sizeof (struct dialog_window_for_data_file));
	if (!dialog_window)	
	{
		ERR("Failed to allocate user data for dialog");
		lv_obj_delete(modal_bg);
       		lv_obj_delete(kb);
       		exit(EXIT_FAILURE);
	}
	
	dialog_window->text = ta;
	dialog_window->keyboard = kb;
	dialog_window->cont = modal_bg;
	dialog_window->display = display;
	
	
	lv_obj_add_event_cb(ok_btn, dialog_ok_event_cb, LV_EVENT_CLICKED, dialog_window);
   	lv_obj_add_event_cb(cancel_btn, dialog_cancel_event_cb, LV_EVENT_CLICKED, dialog_window);
   	lv_obj_add_event_cb(ta, dialog_textarea_event_cb, LV_EVENT_ALL, dialog_window);
   	
   	LOG("Adding textarea %p to group %p\n", (void*)ta, (void*)display->media_drv->keyboard->group);
    	lv_group_add_obj(display->media_drv->keyboard->group, ta); 
    	lv_group_add_obj(display->media_drv->keyboard->group, cancel_btn);
    	lv_group_add_obj(display->media_drv->keyboard->group, kb);
    	
    	
    	if (display->media_drv && display->media_drv->keyboard && display->media_drv->keyboard->keyboard_indev && display->media_drv->keyboard->group) 
    	{
        	lv_indev_set_group(display->media_drv->keyboard->keyboard_indev, display->media_drv->keyboard->group);
        	LOG("Keyboard indev group set to DIALOG group: %p\n", (void*)display->media_drv->keyboard->group);
    	} 
    	else 
    	{
        	ERR("Failed to set keyboard indev group for dialog - critical components missing!\n");
    	}

    	lv_group_focus_obj(ta); 
    	LOG("Focused object in dialog group is now: %p\n", (void*)lv_group_get_focused(display->media_drv->keyboard->group));
    	
	
	
}

void button_for_close(lv_event_t* event)
{
	LOG("Cliecked on button close!\n");
	exit(EXIT_SUCCESS);
}







