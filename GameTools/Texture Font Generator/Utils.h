#ifndef UTILS_H
#define UTILS_H

inline float
truncf(float f)
{
	return float(int(f));
};
inline float
roundf(float f)
{
	if (f < 0)
		return truncf(f - 0.5f);
	return truncf(f + 0.5f);
};

inline long int
lrintf(float f)
{
	int retval;

	_asm fld f;
	_asm fistp retval;

	return retval;
}

struct Surface
{
	Surface() { pRGBA = NULL; }
	~Surface() { delete[] pRGBA; }
	Surface(const Surface& cpy);
	int iWidth;
	int iHeight;
	int iPitch;
	unsigned char* pRGBA;
};

void
BitmapToSurface(HBITMAP hBitmap, Surface* pSurf);
void
GrayScaleToAlpha(Surface* pSurf);
void
GetBounds(const Surface* pSurf, RECT* out);

bool
SavePNG(FILE* f, char szErrorbuf[1024], const Surface* pSurf);

#endif
