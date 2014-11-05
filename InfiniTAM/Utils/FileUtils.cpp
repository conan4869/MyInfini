// Copyright 2014 Isis Innovation Limited and the authors of InfiniTAM

#include "FileUtils.h"

#include <stdio.h>
#include <fstream>

#include "opencv/cv.h"
#include "opencv/highgui.h"

static const char *pgm_id = "P5";
static const char *ppm_id = "P6";

typedef enum { PNM_PGM, PNM_PPM, PNM_PGM_16u, PNM_PGM_16s, PNM_UNKNOWN = -1 } PNMtype;

static PNMtype pnm_readheader(FILE *f, int *xsize, int *ysize)
{
	char tmp[1024];
	PNMtype type = PNM_UNKNOWN;
	int xs = 0, ys = 0, max_i = 0;

	/* read identifier */
	fscanf(f, "%[^ \n\t]", tmp);
	if (!strcmp(tmp, pgm_id)) type = PNM_PGM;
	else if (!strcmp(tmp, ppm_id)) type = PNM_PPM;
	else return type;

	/* read size */
	if (!fscanf(f, "%i", &xs)) return PNM_UNKNOWN;
	if (!fscanf(f, "%i", &ys)) return PNM_UNKNOWN;

	if (!fscanf(f, "%i", &max_i)) return PNM_UNKNOWN;
	if (max_i < 0) return PNM_UNKNOWN;
	else if (max_i <= (1 << 8)) { }
	else if ((max_i <= (1 << 15)) && (type == PNM_PGM)) type = PNM_PGM_16s;
	else if ((max_i <= (1 << 16)) && (type == PNM_PGM)) type = PNM_PGM_16u;
	else return PNM_UNKNOWN;
	fgetc(f);

	if (xsize) *xsize = xs;
	if (ysize) *ysize = ys;

	return type;
}

static bool pnm_readdata(FILE *f, int xsize, int ysize, PNMtype type, void *data)
{
	int channels = 0;
	int bytesPerSample = 0;
	switch (type) 
	{
	case PNM_PGM: bytesPerSample = sizeof(unsigned char); channels = 1; break;
	case PNM_PPM: bytesPerSample = sizeof(unsigned char); channels = 3; break;
	case PNM_PGM_16s: bytesPerSample = sizeof(short); channels = 1; break;
	case PNM_PGM_16u: bytesPerSample = sizeof(unsigned short); channels = 1; break;
	case PNM_UNKNOWN: break;
	}
	if (bytesPerSample == 0) return false;

	fread(data, bytesPerSample, xsize*ysize*channels, f);
	return (data != NULL);
}

static bool pnm_writeheader(FILE *f, int xsize, int ysize, PNMtype type)
{
	const char *pnmid = NULL;
	int max = 0;
	switch (type) {
	case PNM_PGM: pnmid = pgm_id; max = 256; break;
	case PNM_PPM: pnmid = ppm_id; max = 255; break;
	case PNM_PGM_16s: pnmid = pgm_id; max = 32767; break;
	case PNM_PGM_16u: pnmid = pgm_id; max = 65535; break;
	case PNM_UNKNOWN: return false;
	}
	if (pnmid == NULL) return false;

	fprintf(f, "%s\n", pnmid);
	fprintf(f, "%i %i\n", xsize, ysize);
	fprintf(f, "%i\n", max);

	return true;
}

static bool pnm_writedata(FILE *f, int xsize, int ysize, PNMtype type, const void *data)
{
	int channels = 0;
	int bytesPerSample = 0;
	switch (type)
	{
	case PNM_PGM: bytesPerSample = sizeof(unsigned char); channels = 1; break;
	case PNM_PPM: bytesPerSample = sizeof(unsigned char); channels = 3; break;
	case PNM_PGM_16s: bytesPerSample = sizeof(short); channels = 1; break;
	case PNM_PGM_16u: bytesPerSample = sizeof(unsigned short); channels = 1; break;
	case PNM_UNKNOWN: break;
	}
	fwrite(data, bytesPerSample, channels*xsize*ysize, f);
	return true;
}

void SaveImageToFile(const ITMUChar4Image* image, const char* fileName, bool flipVertical)
{
	FILE *f = fopen(fileName, "wb");
	if (!pnm_writeheader(f, image->noDims.x, image->noDims.y, PNM_PPM)) {
		fclose(f); return;
	}

	unsigned char *data = new unsigned char[image->noDims.x*image->noDims.y * 3];

	Vector2i noDims = image->noDims;

	if (flipVertical)
	{
		for (int y = 0; y < noDims.y; y++) for (int x = 0; x < noDims.x; x++)
		{
			int locId_src, locId_dst;
			locId_src = x + y * noDims.x;
			locId_dst = x + (noDims.y - y - 1) * noDims.x;

			data[locId_dst * 3 + 0] = image->GetData(false)[locId_src].r;
			data[locId_dst * 3 + 1] = image->GetData(false)[locId_src].g;
			data[locId_dst * 3 + 2] = image->GetData(false)[locId_src].b;
		}
	}
	else
	{
		for (int i = 0; i < noDims.x * noDims.y; ++i) {
			data[i * 3 + 0] = image->GetData(false)[i].r;
			data[i * 3 + 1] = image->GetData(false)[i].g;
			data[i * 3 + 2] = image->GetData(false)[i].b;
		}
	}

	pnm_writedata(f, image->noDims.x, image->noDims.y, PNM_PPM, data);
	delete[] data;
	fclose(f);
}

void SaveImageToFile(const ITMShortImage* image, const char* fileName)
{
	FILE *f = fopen(fileName, "wb");
	if (!pnm_writeheader(f, image->noDims.x, image->noDims.y, PNM_PGM_16u)) {
		fclose(f); return;
	}
	pnm_writedata(f, image->noDims.x, image->noDims.y, PNM_PGM_16u, image->GetData(false));
	fclose(f);
}

void SaveImageToFile(const ITMFloatImage* image, const char* fileName)
{
	unsigned short *data = new unsigned short[image->dataSize];
	for (int i = 0; i < image->dataSize; i++)
	{
		float localData = image->GetData(false)[i];
		data[i] = localData >= 0 ? (unsigned short)(localData * 1000.0f) : 0;
	}

	FILE *f = fopen(fileName, "wb");
	if (!pnm_writeheader(f, image->noDims.x, image->noDims.y, PNM_PGM_16u)) {
		fclose(f); return;
	}
	pnm_writedata(f, image->noDims.x, image->noDims.y, PNM_PGM_16u, data);
	fclose(f);

	delete[] data;
}

bool ReadImageFromFile(ITMUChar4Image* image, const char* fileName)
{
	int xsize, ysize;
// 	FILE *f = fopen(fileName, "rb");
// 	if (f == NULL) return false;
// 	if (pnm_readheader(f, &xsize, &ysize)!=PNM_PPM) { fclose(f); return false; }
// 
// 	unsigned char *data = new unsigned char[xsize*ysize*3];
// 	if (!pnm_readdata(f, xsize, ysize, PNM_PPM, data)) { fclose(f); delete[] data; return false; }
// 	fclose(f);
	cv::Mat tmp= cv::imread(fileName);
	xsize = tmp.rows;
	ysize = tmp.cols;
	unsigned char *data = tmp.data;

	Vector2i newSize(xsize, ysize);
	image->ChangeDims(newSize);
	for (int i = 0; i < image->noDims.x*image->noDims.y; ++i) 
	{
		image->GetData(false)[i].r = data[i*3+0];
		image->GetData(false)[i].g = data[i*3+1];
		image->GetData(false)[i].b = data[i*3+2];
		image->GetData(false)[i].w = 255; 
	}

//	delete[] data;

	return true;
}

bool ReadImageFromFile(ITMShortImage *image, const char *fileName)
{
	int xsize, ysize;
// 	FILE *f = fopen(fileName, "rb");
// 	if (f == NULL) return false;
// 	PNMtype type = pnm_readheader(f, &xsize, &ysize);
// 	if ((type != PNM_PGM_16s)&&(type != PNM_PGM_16u)) { fclose(f); return false; }
// 	unsigned short *data = new unsigned short[xsize*ysize];
// 	if (!pnm_readdata(f, xsize, ysize, type, data)) { fclose(f); delete[] data; return false; }
// 
	FILE *f = fopen(fileName,"r");
	float tmp = 0;
	

	
	xsize = 640;
	ysize = 480;
	unsigned short *data = new unsigned short[xsize*ysize];
	for (int i = 0;i<640*480;i++)
	{
		fscanf(f,"%f",&tmp);
		data[i] = (unsigned short)(tmp);
	}

	fclose(f);
//	FILE *fp = fopen("de.txt","w");

	Vector2i newSize(xsize, ysize);
	image->ChangeDims(newSize);
	for (int i = 0; i < image->noDims.x*image->noDims.y; ++i) {
//		image->GetData(false)[i] = (data[i]<<8) | ((data[i]>>8)&255);
		image->GetData(false)[i] = data[i];
//		fprintf(fp,"%d ",(data[i]<<8) | ((data[i]>>8)&255));
	}
	delete[] data;

	return true;
}

