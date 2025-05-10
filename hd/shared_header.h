#ifndef __SHAR_HD__
	#define __SHAR_HD__
	
	
#ifdef DBG
	#include <stdio.h>
	#define LOGGING(...) fprintf(stdout, "" __VA_ARGS__)
    	#define ERR(fmt, ...) fprintf(stderr, "ERROR: " fmt, ##__VA_ARGS__)

    	#define LOG(_MESSAGE, ...)        		\
    	do 						\
    	{                        			\
        	LOGGING(_MESSAGE, ##__VA_ARGS__);       \
    	} while (0)


	#define CALL_INIT_FILE_STATE(_FILESTATE, _FILENAME)  					\
		do										\
		{										\
			int result = init_file_state(_FILESTATE, _FILENAME);  			\
	    		if (result != 0) 							\
	    		{                                   					\
		                         							\
		    			ERR("Failed init file_state");    			\
	       				                                     			\
				return EXIT_FAILURE;                         			\
	    		}									\
	    	}										\
	    	while(0)									\

#endif
	
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <stdbool.h>



#define MAX_CHARS_PER_LINE 	80
#define true			1

#define LINES_PER_PAGE 		20
#define BYTES_PER_LINE 		16


enum _FORMAT
{
			BIN,	
			OCT,
			DEC,
			HEX
};

	
struct _FILE_STATE
{
	char*		filename;
	char*		full_path;
	
	unsigned char*	file_bytes;
	size_t		size_file;
	
	
	unsigned char*  page_buffer;         
    	size_t      	page_buffer_size;    
    	size_t      	bytes_per_page;      
   	size_t      	current_page;        
    	size_t      	total_pages;
    	
    	size_t 		offset_bytes_of_file;
    	size_t 		offset_bytes_of_arr;
    	
    	
	char*		result_data_string;
	
	FILE*		data_file;
	enum _FORMAT 	format;
	
	void		(*cleanup_file)(struct _FILE_STATE*);
	
};


extern int open_file(struct _FILE_STATE* , const char* );
extern int init_file_state(struct _FILE_STATE* , const char* );
extern int getting_bytes_from_a_file(struct _FILE_STATE*, size_t);
	
#endif // __SHAR_HD__















