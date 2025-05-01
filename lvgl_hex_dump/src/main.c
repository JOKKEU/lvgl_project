#include "../hd/shared_header.h"
#include "../hd/lvgl.h"



int main(int argc, char** argv)
{
	int result = 0;
	
	struct _FILE_STATE* file_state = (struct _FILE_STATE*)malloc(sizeof (struct _FILE_STATE));
	if (!file_state)
	{
		ERR("Error allocate file_state in main()\n");
		return EXIT_FAILURE;
	}
	
	
	struct _DISPLAY* display = (struct _DISPLAY*)malloc(sizeof (struct _DISPLAY));
	if (!display)
	{
		ERR("Error alloc display in main()\n");
		return EXIT_FAILURE;
	}
	
	result = display_init(display, file_state);
	if (result != 0)
	{
		ERR("Error display init\n");
		return EXIT_FAILURE;
	}
	
	int running = true;
	uint32_t last_tick = SDL_GetTicks();
	while (running) 
	{
	
		bool key_event_this_iteration = false;
		
        	while (SDL_PollEvent(&display->media_drv->event)) 
        	{
        		
           	 	if (display->media_drv->event.type == SDL_QUIT)
		    	{
				running = false;
		    	}
		    
		    	else if (display->media_drv->event.type == SDL_KEYDOWN)
		    	{
			
				display->media_drv->keyboard->last_sdl_keycode = display->media_drv->event.key.keysym.sym;
				display->media_drv->keyboard->last_key_state = LV_INDEV_STATE_PRESSED;
				display->media_drv->keyboard->key_event_processed = false; 
				LOG("SDL_KEYDOWN: sym=0x%x (%s), state=PRESSED, processed=false\n",
			    	display->media_drv->event.key.keysym.sym, SDL_GetKeyName(display->media_drv->event.key.keysym.sym));
		    	}
		    	else if (display->media_drv->event.type == SDL_KEYUP)
		    	{
		
				display->media_drv->keyboard->last_sdl_keycode = display->media_drv->event.key.keysym.sym;
				display->media_drv->keyboard->last_key_state = LV_INDEV_STATE_RELEASED;
				display->media_drv->keyboard->key_event_processed = false; 
				LOG("SDL_KEYUP: sym=0x%x (%s), state=RELEASED, processed=false\n",
			    	display->media_drv->event.key.keysym.sym, SDL_GetKeyName(display->media_drv->event.key.keysym.sym));
		    	}
		    	
		    	else if (display->media_drv->event.type == SDL_MOUSEWHEEL)
		    	{
		    		if (display->media_drv->event.wheel.y > 0)
		    		{
		    			display->media_drv->mouse->encoder_diff--;
		    			display->media_drv->mouse->encoder_event_pending = true;
		    			LOG("mouse wheel up\n");
		    			
		    		}
		    		
		    		else if (display->media_drv->event.wheel.y < 0)
		    		{
		    			display->media_drv->mouse->encoder_diff++;
		    			display->media_drv->mouse->encoder_event_pending = true;
		    			LOG("mouse wheel down\n");
		    		}
		    	}
        	}

        
		uint32_t time_elapsed = SDL_GetTicks() - last_tick; 
		last_tick = SDL_GetTicks();
		lv_tick_inc(time_elapsed); 
		lv_timer_handler();      

		SDL_SetRenderDrawColor(display->media_drv->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE); 
		SDL_RenderClear(display->media_drv->renderer);
		
		
		if (display->media_drv->lvgl_texture) 
		{
			SDL_RenderCopy(display->media_drv->renderer, display->media_drv->lvgl_texture, NULL, NULL);
		}

		SDL_RenderPresent(display->media_drv->renderer);

		SDL_Delay(5); 
    	}
	
	
	/*
	result = open_file(file_state, "/home/jokkeu/Desktop/TLauncher.jar");
	if (result != 0)
	{
		return EXIT_FAILURE;
	}
	*/
	//get_arr_to_hex_format(file_state);
	
	file_state->cleanup_file(file_state);
	display->cleanup_display(display);
	
	LOG("SUCCESS\n");
	return EXIT_SUCCESS;
}












