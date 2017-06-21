#include "common.h"
#include "dirlist.h"
#include "gallery.h"
#include "screenshot.h"
#include "utils.h"

void displayImage(char * path, int ext)
{
	sf2d_texture * image = NULL;

	if (ext == 0)
	{
		image = sfil_load_PNG_file(path, SF2D_PLACE_RAM);
		setBilinearFilter(image);
	}
	else if (ext == 1)
	{
		image = sfil_load_JPEG_file(path, SF2D_PLACE_RAM);
		setBilinearFilter(image);
	}
	else if (ext == 2) // Supported formats: GIF, HDR, PIC, PNM, PSD, TGA
	{
		image = sfil_load_IMG_file(path, SF2D_PLACE_RAM);
		setBilinearFilter(image);
	}
	
	sf2d_set_clear_color(RGBA8(33, 39, 43, 255));
	
	int galleryBarY = 0, galleryBarLimY = -35, nameY = 11, nameLimY = -11;
	
	uint64_t start = osGetTime();
	
	bool bothScreens = false;
		
	while (aptMainLoop())
	{
		hidScanInput();
		hidTouchRead(&touch);
		
		sf2d_start_frame(GFX_BOTTOM, GFX_LEFT); // Clear bottom screen
		
		if (bothScreens == true)
			sf2d_draw_texture_part(image, 0, 0, 40, 240, 320, 240);
		
		sf2d_end_frame();
		
		sf2d_start_frame(GFX_TOP, GFX_LEFT);
		
		if ((image->width <= 400) && (image->height <= 240))
			sf2d_draw_texture(image, ((400 - image->width) / 2), ((240 - image->height) / 2));
		else if ((image->width == 400) && ((image->height == 480) || (image->height == 482))) // Both screens
		{
			sf2d_draw_texture_part(image, 0, 0, 0, 0, 400, 240);
			bothScreens = true;
		}
		
		if (osGetTime() - start >= (2000))
		{
			nameY -= 2;
			galleryBarY -= 2;
		}
		
		if (galleryBarY == -35)
			galleryBarY = galleryBarLimY;
		if (nameY == -11)
			nameY = nameLimY; 
			
		sf2d_draw_texture(galleryBar, 0, galleryBarY);
		sftd_draw_textf(font, 30, nameY, RGBA8(255, 255, 255, 255), 11, "%.60s", fileName);
		
		if (kPressed & KEY_TOUCH)
		{
			galleryBarY = 0;
			nameY = 11;
			start = osGetTime();
		}
		
		endDrawing();
		
		if (kPressed & KEY_B)
		{
			wait(100000000);
			break;
		}
		
		if ((kHeld & KEY_L) && (kHeld & KEY_R))
			captureScreenshot();
	}
	
	//delete image
	sf2d_free_texture(image);
}