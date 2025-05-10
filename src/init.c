#include "../hd/shared_header.h"
#include "../hd/lvgl.h"

void cleanup_f(struct _FILE_STATE* fs);

int init_file_state(struct _FILE_STATE* fs, const char* fname)
{
	fs->filename = (char*)malloc(sizeof (char) * 100);
	if (!fs->filename)
	{
		ERR("Error allocate mem for filename\n");
		return EXIT_FAILURE;
	}

	fs->full_path = (char*)malloc(sizeof (char) * 200);
	if (!fs->full_path)
	{
		ERR("Error allocate mem for filename\n");
		free(fs->filename);
		return EXIT_FAILURE;
	}

	strncpy(fs->full_path, fname, strlen(fname));
	fs->full_path[strlen(fname)] 		= '\0';

 	strncpy(fs->full_path, fname, 199);
	fs->full_path[199] 			= '\0';


 	char* last_slash = strrchr(fs->full_path, '/');
 	if (last_slash)
 	{

 		strncpy(fs->filename, last_slash + 1, 99);
 		fs->filename[99] 		= '\0';
 	}
 	else
 	{
 		strncpy(fs->filename, fs->full_path, 99);
 		fs->filename[99] 		= '\0';
 	}

 	fs->data_file = fopen(fs->full_path, "rb"); // Open in binary read mode
 	if (!fs->data_file)
 	{
 		ERR("Failed to open file: %s (%s)", fs->full_path, strerror(errno));
 		free(fs->filename);
 		free(fs->full_path);
 		return EXIT_FAILURE;
 	}

 	if (fseek(fs->data_file, 0, SEEK_END) != 0)
 	{
 		ERR("fseek error (SEEK_END): %s", strerror(errno));
 		fclose(fs->data_file);
 		free(fs->filename);
 		free(fs->full_path);
 		return EXIT_FAILURE;
 	}

 	long file_size_long = ftell(fs->data_file);
 	if (file_size_long < 0)
 	{
  		ERR("ftell error: %s", strerror(errno));
  		fclose(fs->data_file);
 		free(fs->filename);
 		free(fs->full_path);
  		return EXIT_FAILURE;
 	}

 	fs->bytes_per_page 	= LINES_PER_PAGE * BYTES_PER_LINE;
	fs->page_buffer_size 	= (LINES_PER_PAGE * MAX_CHARS_PER_LINE) + 1; 
	
	LOG("fs->page_buffer_size: %llu\n", fs->page_buffer_size);
	LOG("fs->bytes_per_page: %llu\n", fs->bytes_per_page);


	fs->file_bytes = (unsigned char*)malloc(20 * LINES_PER_PAGE * MAX_CHARS_PER_LINE); // 20 pages
 	if (!fs->file_bytes)
 	{
		ERR("error alloc file_bytes (%zu bytes)", 20 * LINES_PER_PAGE * MAX_CHARS_PER_LINE);
		fclose(fs->data_file);
		free(fs->filename);
 		free(fs->full_path);
		return EXIT_FAILURE;
 	}

 	fs->page_buffer = (unsigned char*)malloc(fs->page_buffer_size);
	if (!fs->page_buffer)
	{
		ERR("Error alloc page buffer\n");
		fclose(fs->data_file);
		free(fs->filename);
 		free(fs->full_path);
 		free(fs->file_bytes);
		return EXIT_FAILURE;
	}
	
	fs->result_data_string = (char*)malloc(512);
	if (!fs->result_data_string)
	{
		ERR("Failed alloc result_data_string\n");
		fclose(fs->data_file);
		free(fs->filename);
 		free(fs->full_path);
 		free(fs->file_bytes);
 		free(fs->page_buffer);
		return EXIT_FAILURE;
	}

 	fs->size_file 				= (size_t)file_size_long;
	fs->cleanup_file 			= NULL; 
	fs->current_page 			= 0;
	fs->offset_bytes_of_file 		= 0;
	fs->offset_bytes_of_arr 		= 0;
	fs->total_pages 			= 0;
	

	return EXIT_SUCCESS;
}




void cleanup_f(struct _FILE_STATE* fs)
{
	if (fs)
	{
		if (fs->file_bytes)
		{
			free(fs->file_bytes);
		}
		
		if (fs->filename)
		{
			free(fs->filename);
		}
		if (fs->full_path)
		{
			free(fs->filename);
		}
		
		if (fs->full_path)
		{
			free(fs->full_path);
		}
		if (fs->page_buffer)
		{
			free(fs->page_buffer);
		}
		if (fs->result_data_string)
		{
			free(fs->result_data_string);
		}
		
	}

}



















