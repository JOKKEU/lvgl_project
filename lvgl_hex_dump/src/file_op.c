#include "../hd/shared_header.h"



int getting_bytes_from_a_file(struct _FILE_STATE* fs)
{
    if (!fs) 
    {
        ERR("fs is NULL");
        return EXIT_FAILURE;
    }
    if (!fs->data_file) 
    {
        ERR("error file not opened");
        return EXIT_FAILURE;
    }

    
    if (fs->file_bytes) 
    {
        free(fs->file_bytes);
        fs->file_bytes = NULL;
        fs->size_file = 0;
    }
    if (fs->page_buffer) 
    {
        free(fs->page_buffer);
        fs->page_buffer = NULL;
        fs->page_buffer_size = 0;
    }

    if (fseek(fs->data_file, 0, SEEK_END) != 0) 
    {
        ERR("fseek error (SEEK_END)");
        return EXIT_FAILURE;
    }

    long file_size_long = ftell(fs->data_file);
    if (file_size_long < 0) {
         ERR("ftell error");
         return EXIT_FAILURE;
    }
    fs->size_file = (size_t)file_size_long;

    if (fs->size_file == 0) 
    {
        LOG("File is empty");
        fs->file_bytes = NULL; 
        fs->total_pages = 1;   
        fs->current_page = 0;
        fs->bytes_per_page = 0;

        
        fs->page_buffer_size = 20; 
        fs->page_buffer = (char*)malloc(fs->page_buffer_size);
        if (!fs->page_buffer) {
            ERR("Failed to allocate page buffer for empty file");
            fs->page_buffer_size = 0;
            return EXIT_FAILURE;
        }
        strcpy(fs->page_buffer, "(Empty)"); 
        return EXIT_SUCCESS;
    }

    fs->file_bytes = (unsigned char*)malloc(fs->size_file);
    if (!fs->file_bytes) 
    {
        ERR("error alloc file_bytes (%zu bytes)", fs->size_file);
        fs->size_file = 0;
        return EXIT_FAILURE;
    }

    if (fseek(fs->data_file, 0, SEEK_SET) != 0) 
    {
        ERR("fseek error (SEEK_SET)");
        free(fs->file_bytes);
        fs->file_bytes = NULL;
        fs->size_file = 0;
        return EXIT_FAILURE;
    }
    

    size_t bytes_read = fread(fs->file_bytes, 1, fs->size_file, fs->data_file);
    if (bytes_read < fs->size_file) 
    {
        if (feof(fs->data_file)) 
        {
             LOG("fread reached EOF earlier than expected (%zu read vs %zu size)", bytes_read, fs->size_file);
             fs->size_file = bytes_read; 
             
        } 
        else 
        {
             ERR("file read error: %s", strerror(errno));
             free(fs->file_bytes);
             fs->file_bytes = NULL;
             fs->size_file = 0;
             
             return EXIT_FAILURE;
        }
    }

    fs->bytes_per_page = LINES_PER_PAGE * BYTES_PER_LINE;
    if (fs->bytes_per_page == 0)
     { 
        ERR("bytes_per_page is zero, check LINES_PER_PAGE/BYTES_PER_LINE constants");
        free(fs->file_bytes);
        fs->file_bytes = NULL;
        fs->size_file = 0;
        return EXIT_FAILURE;
    }
    fs->total_pages = (fs->size_file + fs->bytes_per_page - 1) / fs->bytes_per_page;
     if (fs->total_pages == 0 && fs->size_file > 0) 
     { 
        fs->total_pages = 1;
    }

    fs->current_page = 0; 

    fs->page_buffer_size = (LINES_PER_PAGE * MAX_CHARS_PER_LINE) + 1; 
    fs->page_buffer = (char*)malloc(fs->page_buffer_size);
    if (!fs->page_buffer) 
    {
        ERR("Failed to allocate page buffer (%zu bytes)", fs->page_buffer_size);
        fs->page_buffer_size = 0;
        free(fs->file_bytes);
        fs->file_bytes = NULL;
        fs->size_file = 0;
        return EXIT_FAILURE;
    }
    fs->page_buffer[0] = '\0'; 

    LOG("File read successfully (%zu bytes), Paginated: %zu pages, Buffer size: %zu\n",
        fs->size_file, fs->total_pages, fs->page_buffer_size);

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




































