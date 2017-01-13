// 32-bit BMP drawn as a knitted image.
// by Greg Frazier circa... 2012? probably.
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <math.h>

// Adjust these to change the resolution of the output, 15 x 8 is 1:1 ratio.
// Note though, the knit is a constant 15x8 pixels (unless you change it below)
#define BLOCK_WIDTH 15
#define BLOCK_HEIGHT 8

#define APPNAME "KnitArt"


char szAppName[] = APPNAME;
char szTitle[]   = APPNAME;
char *pWindowText;
HINSTANCE g_hInst;
long width, height;
BITMAPFILEHEADER bitmapFileHeader;
BITMAPINFOHEADER bitmapInfoHeader;
char *filecontents;
int segments, offset, y;

// gray-scale knit image. Yeah, I'm using int instead of char, lol.
int knit_table[181] = {
	119,109,109,104,255,255,255,255,255,255,255,255,88, 58, 52,
	146,175,175,135,93, 255,255,255,255,255,255,99, 109,96, 61,
	140,197,200,176,90, 104,255,255,255,255,90, 124,153,117,58,
	117,197,206,203,158,96, 255,255,255,255,93, 175,190,140,90,
	85, 171,205,207,200,157,67, 52, 61, 124,169,184,187,158,88,
	67, 140,200,206,207,186,106,82, 109,172,199,195,184,137,82,
	52, 106,189,204,197,180,135,93, 148,195,184,158,121,79, 61,
	64, 61, 153,186,173,151,129,99, 155,193,149,106,82, 61, 49,
	255,255,255,255,131,171,187,142,151,178,148,117,255,255,255,
	255,255,255,255,255,137,180,129,137,160,119,255,255,255,255,
	255,255,255,255,255,255,142,112,117,112,255,255,255,255,255,
	255,255,255,255,255,255,99, 82, 67, 52, 255,255,255,255,255
};

// Load the bmp into mem
char *LoadFile(char *pszFileName, long *lgSize){
   	FILE *TARGET;
	char *mem;
	char *ptr2;
	long h = 0;

	fopen_s(&TARGET, pszFileName, "rb");

	if(!TARGET)
	{
		printf("LoadFile(): Unable to open file specified. %s\n", pszFileName);
		*lgSize = 0;
		return 0;
	}

	fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER),1,TARGET);
	fread(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER),1,TARGET);

	if(bitmapInfoHeader.biBitCount != 24){
	    printf("LoadFile(): Not a 24-bit BMP File!\n");
	    *lgSize = 0;
	    return 0;
	}

	fseek(TARGET, bitmapFileHeader.bfOffBits, SEEK_SET);
	h = (long) bitmapFileHeader.bfSize-bitmapFileHeader.bfOffBits;

	mem = (char*) malloc(h+1);
	ptr2 = &mem[0];
	if(mem == NULL)
	{
		printf("LoadFile(): Could Not Allocate Memory.\n");
		*lgSize = 0;
		return 0;
	}
	fread(mem,sizeof(BYTE),h,TARGET);
	*lgSize = h;
	width = bitmapInfoHeader.biWidth;
	height = bitmapInfoHeader.biHeight;
	fclose(TARGET);
	return mem;
}

void DrawKnit(HDC hdc, int r, int g, int b, int x, int y)
{
	//Draw the Knit, mask color 255.
	int yOffset = 0, xOffset = 0;
	double newR, newG, newB, newKnit;
	for(int n = 0; n < 180; n++)
	{
		if(xOffset == 15){
			xOffset = 0;
			yOffset++;
		}

		// Yeah let's use wide vars as a questionable way to prevent overflow.
		newKnit = ((double)knit_table[n]) / 255;
		newR = ((double)r) * newKnit;
		newG = ((double)g) * newKnit;
		newB = ((double)b) * newKnit;

		// nice and slow
		if(knit_table[n] != 255)
			SetPixel(hdc, x+xOffset, y+yOffset, RGB( (int)newR, (int)newG, (int)newB ));
		xOffset++;
	}
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int segment, xLocation, yLocation, block;
	switch (message)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_KEYDOWN:
			if (VK_ESCAPE == wParam)
				DestroyWindow(hwnd);
			break;
		// Yup, does it on every repaint. I don't recommend resizing the window, lol
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC         hdc;
			RECT        rc;
			hdc = BeginPaint(hwnd, &ps);
			GetClientRect(hwnd, &rc);
			SetBkMode(hdc, TRANSPARENT);
			// Scan in nxm sections, work backwards because BMPs are upside-down.
			for(segment = segments; segment > -1; segment--)
					for(xLocation = 0; xLocation < y; xLocation += (3 * BLOCK_WIDTH)){
						int pixelsProcessed = 0, r = 0, g = 0, b = 0;
						for(yLocation = segment * BLOCK_HEIGHT; yLocation < (segment + 1) * BLOCK_HEIGHT; yLocation++)
							for(block = xLocation; block < xLocation + (3 * BLOCK_WIDTH); block++){
								r += filecontents[(yLocation*y) + xLocation + 2 + (yLocation*offset)] & 0x00FF;
								g += filecontents[(yLocation*y) + xLocation + 1 + (yLocation*offset)] & 0x00FF;
								b += filecontents[(yLocation*y) + xLocation     + (yLocation*offset)] & 0x00FF;
								pixelsProcessed++;
							}
						r /= pixelsProcessed;
						g /= pixelsProcessed;
						b /= pixelsProcessed;
						// just send in an averaged color that will be applied to the "knit"
						DrawKnit(hdc, r, g, b, ((xLocation/(3*BLOCK_WIDTH))*14), (segments-segment)*8);
					}
			EndPaint(hwnd, &ps);
			break;
		}
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

int main(int argc, char **argv){
	
	long w, h, filesize;
	MSG msg;
	WNDCLASS wc;
	HWND hwnd;
	
	offset = 0;

	if (argc != 2)
	{
		printf("usage: file.bmp\n");
		return -1;
	}

	//-------------------------------------------------
	// This really shouldn't be here, but laziness.
	//-------------------------------------------------
	filecontents = LoadFile(argv[1], &filesize);
    if(filecontents == 0){
        return -1;
    }
	w = width;
	h = height;
	if((w*3) % 4 != 0)
		offset = (4-((w*3)%4));
	y = (w * 3);
	segments = ((int)(h / BLOCK_HEIGHT)) - 1;
	//-------------------------------------------------

	ZeroMemory(&wc, sizeof wc);
	wc.hInstance     = GetModuleHandle(0);
	wc.lpszClassName = szAppName;
	wc.lpfnWndProc   = (WNDPROC)WndProc;
	wc.style         = CS_DBLCLKS|CS_VREDRAW|CS_HREDRAW;
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	
	if (FALSE == RegisterClass(&wc)){
		printf("Could not register window class\n");
		return -1;
	}

	hwnd = CreateWindow(
		szAppName,
		szTitle,
		WS_OVERLAPPEDWINDOW|WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		800,
		600,
		0,
		0,
		g_hInst,
		0);

	if (NULL == hwnd){
		printf("Could not create window\n");
		return -1;
	}
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	free(filecontents);
	return msg.wParam;
}