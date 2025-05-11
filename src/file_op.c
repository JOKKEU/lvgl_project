#include "../hd/shared_header.h"



int getting_bytes_from_a_file(struct _FILE_STATE* fs, size_t page_index) 
{
	LOG("call getting_bytes_from_a_file for page_index: %zu\n", page_index);
    	if (!fs) 
    	{
		ERR("fs is NULL\n");
		return EXIT_FAILURE;
    	}

    	if (fs->bytes_per_page == 0 && fs->size_file > 0) 
    	{
		fs->bytes_per_page = LINES_PER_PAGE * BYTES_PER_LINE;
		LOG("Warning: fs->bytes_per_page was 0 for non-empty file. Set to default: %zu\n", fs->bytes_per_page);
		
		if (fs->bytes_per_page == 0) 
		{ 
			ERR("Cannot operate with fs->bytes_per_page = 0 for non-empty file.\n");	
		    	 return EXIT_FAILURE;
		}
    	}
    

    	if (fs->size_file == 0) 
    	{
        	fs->total_pages = 1; 
    	} 
    	else 
    	{
		if (fs->bytes_per_page > 0) 
		{
			fs->total_pages = (fs->size_file + fs->bytes_per_page - 1) / fs->bytes_per_page;
		    	if (fs->total_pages == 0) fs->total_pages = 1;
		} 
		else 
		{

			ERR("fs->bytes_per_page is 0. Cannot calculate total_pages for non-empty file.\n");
		    	fs->total_pages = 0;
		    	return EXIT_FAILURE;
		}
    	}
    	
    	
    	LOG("Calculated total_pages: %zu\n", fs->total_pages);


    	if (fs->size_file == 0) 
    	{
		LOG("File is empty\n");

		fs->offset_bytes_of_file = 0;
		fs->offset_bytes_of_arr = 0; 

		if (fs->page_buffer && fs->page_buffer_size > strlen("(Empty)")) 
		{
			strcpy(fs->page_buffer, "(Empty)");
		} 
		else 
		{
			ERR("page_buffer is NULL or too small for \"(Empty)\"\n");
		}
		
        	return EXIT_SUCCESS;
    	}
    

    	if (!fs->file_bytes) 
    	{
		ERR("fs->file_bytes is NULL. Buffer not allocated.\n");
		return EXIT_FAILURE;
    	}
    	
    	


    
    	const size_t file_bytes_capacity = 20 * LINES_PER_PAGE * BYTES_PER_LINE; 
    	if (file_bytes_capacity == 0) 
    	{
		ERR("file_bytes_capacity is 0. Check LINES_PER_PAGE/BYTES_PER_LINE defines.\n");
		return EXIT_FAILURE;
    	}

    
    	size_t requested_page_start_offset = page_index * fs->bytes_per_page;
    	size_t requested_page_end_offset = requested_page_start_offset + fs->bytes_per_page;
    
    	if (requested_page_end_offset > fs->size_file) 
    	{
        	requested_page_end_offset = fs->size_file;
    	}


    
    	if (fs->offset_bytes_of_arr > 0 && 
		requested_page_start_offset >= fs->offset_bytes_of_file &&
		requested_page_end_offset <= fs->offset_bytes_of_file + fs->offset_bytes_of_arr) 
    	{
		LOG("Page %zu found in cache (data from %zu to %zu).\n",
		    page_index, fs->offset_bytes_of_file, fs->offset_bytes_of_file + fs->offset_bytes_of_arr);
		return EXIT_SUCCESS; 
    	}

    	LOG("Page %zu not in cache. Reading new chunk.\n", page_index);
    	fs->offset_bytes_of_file = requested_page_start_offset;
    
    
    	if (fs->offset_bytes_of_file >= fs->size_file) 
    	{
		LOG("Attempting to read a chunk starting at or after EOF.\n");
		fs->offset_bytes_of_arr = 0; 
		return EXIT_SUCCESS; 
        
    	}

    	LOG("New chunk will start reading from file offset: %zu\n", fs->offset_bytes_of_file);

    	if (fseek(fs->data_file, fs->offset_bytes_of_file, SEEK_SET) != 0) 
    	{
		ERR("fseek error to %zu (SEEK_SET): %s\n", fs->offset_bytes_of_file, strerror(errno));
		fs->offset_bytes_of_arr = 0; 
		return EXIT_FAILURE;
    	}

    	size_t bytes_to_read_per_chunk_const = 5 * LINES_PER_PAGE * BYTES_PER_LINE;
    	if (bytes_to_read_per_chunk_const == 0) bytes_to_read_per_chunk_const = 1024; 
    
    	fs->offset_bytes_of_arr = 0; 
   	ssize_t bytes_read_total_for_chunk = 0;

    	LOG("Starting read loop. file_bytes_capacity: %zu. Reading in chunks of up to %zu bytes.\n",
        	file_bytes_capacity, bytes_to_read_per_chunk_const);

    	while (bytes_read_total_for_chunk < file_bytes_capacity) 
    	{
		size_t bytes_to_attempt_this_iteration = bytes_to_read_per_chunk_const;

		
		if (bytes_read_total_for_chunk + bytes_to_attempt_this_iteration > file_bytes_capacity) 
		{
			bytes_to_attempt_this_iteration = file_bytes_capacity - bytes_read_total_for_chunk;
		}

		
		size_t file_pos_for_read = fs->offset_bytes_of_file + bytes_read_total_for_chunk;
		if (file_pos_for_read >= fs->size_file) 
		{
			LOG("Reached (or passed) EOF before starting fread iteration. file_pos_for_read: %zu, fs->size_file: %zu\n",
		        	file_pos_for_read, fs->size_file);
		   	 break; 
		}
		
		if (bytes_to_attempt_this_iteration > fs->size_file - file_pos_for_read)
		{
			bytes_to_attempt_this_iteration = fs->size_file - file_pos_for_read;
		}

		if (bytes_to_attempt_this_iteration == 0) 
		{
			LOG("bytes_to_attempt_this_iteration is 0. Breaking read loop.\n");
		   	 break; 
		}

		LOG("fread: Reading %zu bytes from file offset %zu into buffer offset %zu\n",
		    bytes_to_attempt_this_iteration, file_pos_for_read, bytes_read_total_for_chunk);

		ssize_t bytes_read_this_iteration = fread(fs->file_bytes + bytes_read_total_for_chunk,
		                                          1, 
		                                          bytes_to_attempt_this_iteration,
		                                          fs->data_file);

		if (bytes_read_this_iteration < 0) 
		{
			ERR("fread error: %s\n", strerror(errno));

		    	fs->offset_bytes_of_arr = 0;
		    	return EXIT_FAILURE;
		}

		if (bytes_read_this_iteration == 0) 
		{
			LOG("fread reached EOF or read 0 bytes.\n");
		    	break; 
		}

		LOG("Read %zd bytes in this iteration.\n", bytes_read_this_iteration);
		bytes_read_total_for_chunk += bytes_read_this_iteration;
    	}
    
	fs->offset_bytes_of_arr = bytes_read_total_for_chunk; 
    	LOG("File chunk read successfully. Total %zu bytes loaded into fs->file_bytes, starting from file offset %zu.\n",
        	fs->offset_bytes_of_arr, fs->offset_bytes_of_file);
    	LOG("fs->file_bytes: %p, fs->page_buffer: %p\n", (void*)fs->file_bytes, (void*)fs->page_buffer);

    	return EXIT_SUCCESS;
}


int open_file(struct _FILE_STATE* fs, const char* filename)
{
	fs->data_file = fopen(filename, "r+b");
	if (!fs->data_file)
	{
		ERR("Error open file\n");
		return EXIT_FAILURE;
	}

	CALL_INIT_FILE_STATE(fs, filename);
	return EXIT_SUCCESS;
}

