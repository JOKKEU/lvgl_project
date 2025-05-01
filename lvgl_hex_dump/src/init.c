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
	
	
	fs->cleanup_file 			= cleanup_f;
	fs->file_bytes 				= NULL;

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
		
	}

}



















