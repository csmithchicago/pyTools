/*
 * tiff_io.c
 *
 *  Created on: Sep 29, 2016
 *      Author: CSmith
 *
 *  This is a file used for importing and exporting tiff files. These tiff files
 *  may be single images or tiff stacks.
 *
 *  Saving the tif stacks is as floaxt32 stacks or slices.
 *
 *  For reading in the images, the following websites were helpful
 *  	https://www.ibm.com/developerworks/jp/linux/library/l-libtiff/
 *  	https://www.ibm.com/developerworks/jp/linux/library/l-libtiff2/
 *  	https://www.ibm.com/developerworks/linux/library/l-libtiff/read.c
 *  	http://www.simplesystems.org/libtiff/man/index.html
 *  	http://www.awaresystems.be/imaging/tiff/tifftags/baseline.html
 *
 *  For writing the images, the additional sites were helpful
 *  	https://www.ibm.com/developerworks/linux/library/l-libtiff2/write.c
 *  	http://research.cs.wisc.edu/graphics/Courses/638-f1999/libtiff_tutorial.htm
 *
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <tiff.h>
#include <tiffio.h>
#include <errno.h>

#include "tiff_io.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))


int main(int argc, char** argv)
{
	printf("[%s] - Starting...\n", argv[0]);
	printf("Num Arguments %i\n", argc);

	return 0;
}


int TiffInfo(const char* filename, int* shape){
	TIFF *image;
	uint16 photo, bps, spp, fillorder;
	uint32 width, length;
	tsize_t stripSize;
	unsigned long imageOffset, result;
	int stripMax, stripCount, bytesPerSample;
	char tempbyte;
	unsigned long bufferSize, count;
	int readWholeStrip = -1;
	int num_slices;
	int num_elements;
	int slice_num;
	int i;
	long slice_elements;
	char type[5];


	// check to make sure file is a tiff file
	IdentifyImageType(type, filename);
	if (strcmp(type, "TIFF") != 0)
	{
		fprintf(stderr, "ERROR: Import files must be of type TIFF\n");
		exit(42);
	}

	// Open the TIFF image
	if((image = TIFFOpen(filename, "r")) == NULL){
		fprintf(stderr, "Could not open incoming image\n");
		exit(42);
	}

	// Check the Data Type
	if((TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &bps) == 0)){
		fprintf(stderr, "Undefined number of bits per sample\n");
		exit(42);
	}

	if((TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &spp) == 0) || (spp != 1)){
		spp = 1;
		fprintf(stderr, "Either undefined or unsupported number of "
				"samples per pixel\n");
		//exit(42);
	}

	// get the image length, width, and num slices
	if(TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &width) == 0){
		fprintf(stderr, "Image does not define its width\n");
		exit(42);
	}
	if(TIFFGetField(image, TIFFTAG_IMAGELENGTH, &length) == 0){
		fprintf(stderr, "Image does not define its length\n");
		exit(42);
	}

	// Get the number of slices
	num_slices = NumSlices(filename);

	shape[0] = num_slices;
	shape[1] = length;
	shape[2] = width;

	TIFFClose(image);

	return 0;
}


int PythonReadTiffStacks(float* img_array, int* shape, const char* filename)
{
	TIFF *image;
	uint16 photo, bps, spp, fillorder;
	uint32 width, length;
	tsize_t stripSize;
	unsigned long imageOffset, result;
	int stripMax, stripCount, bytesPerSample;
	char tempbyte;
	unsigned long bufferSize, count;
	int readWholeStrip = -1;
	int num_slices;
	int num_elements;
	int slice_num;
	int i;
	long slice_elements;
	char type[5];


	// check to make sure file is a tiff file
	IdentifyImageType(type, filename);
	if (strcmp(type, "TIFF") != 0)
	{
		fprintf(stderr, "ERROR: Import files must be of type TIFF\n");
		exit(42);
	}

	// Open the TIFF image
	if((image = TIFFOpen(filename, "r")) == NULL){
		fprintf(stderr, "Could not open incoming image\n");
		exit(42);
	}

	// Check the Data Type
	if((TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &bps) == 0)){
		fprintf(stderr, "Undefined number of bits per sample\n");
		exit(42);
	}

	if((TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &spp) == 0) || (spp != 1)){
		spp = 1;
		fprintf(stderr, "Either undefined or unsupported number of "
				"samples per pixel\n");
		//exit(42);
	}

	// get the image length, width, and num slices
	if(TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &width) == 0){
		fprintf(stderr, "Image does not define its width\n");
		exit(42);
	}
	if(TIFFGetField(image, TIFFTAG_IMAGELENGTH, &length) == 0){
		fprintf(stderr, "Image does not define its length\n");
		exit(42);
	}

	// Get the number of slices
	num_slices = NumSlices(filename);

	shape[0] = num_slices;
	shape[1] = length;
	shape[2] = width;

	// allocate memory for the array
	num_elements = MAX(1,length) * MAX(1,width) * MAX(1,num_slices);
	slice_elements = MAX(1,length) * MAX(1,width);

	// Read in the possibly multiple strips and slices
	bytesPerSample = MAX(bps/8,1);
	stripSize = TIFFStripSize (image);
	stripMax = TIFFNumberOfStrips (image);
	bufferSize = TIFFNumberOfStrips (image) * stripSize;

	if (bps == 1 || bps == 8)
	{
		unsigned char* buffer;
		if((buffer = (unsigned char *) malloc(bufferSize)) == NULL){
			fprintf(stderr, "Could not allocate enough memory for "
					"the uncompressed image\n");
			exit(42);
		}
		//loop through the slices (multi pages or multiple directories)
		slice_num = 0;
		do {
			// read in the slice
			imageOffset = 0;
			for (stripCount = 0; stripCount < stripMax; stripCount++){
				if((result = TIFFReadEncodedStrip (image, stripCount,
						buffer + imageOffset, readWholeStrip)) == -1){
					fprintf(stderr, "Read error on input strip number"
							" %d\n", stripCount);
					exit(42);
				}
				imageOffset += result/bytesPerSample;
			}
			// transfer the slice to the image array
			for (i = 0; i < slice_elements; i++)
			{
				img_array[i + slice_num*slice_elements] = (float) *(buffer + i);
			}

		    slice_num++;
		} while (TIFFReadDirectory(image));
		free(buffer);
	}
	else if (bps == 16)
	{
		uint16* buffer;
		if((buffer = (uint16 *) malloc(bufferSize)) == NULL){
			fprintf(stderr, "Could not allocate enough memory for the"
					" uncompressed image\n");
			exit(42);
		}
		//loop through the slices (multi pages or multiple directories)
		slice_num = 0;
		do {
			// read in the slice
			imageOffset = 0;
			for (stripCount = 0; stripCount < stripMax; stripCount++){
				if((result = TIFFReadEncodedStrip (image, stripCount,
						buffer + imageOffset, readWholeStrip)) == -1){
					fprintf(stderr, "Read error on input strip number"
							" %d\n", stripCount);
					exit(42);
				}
				imageOffset += result/bytesPerSample;
			}
			// transfer the slice to the image array
			for (i = 0; i < slice_elements; i++)
			{
				img_array[i + slice_num*slice_elements] = (float) *(buffer + i);
			}

		    slice_num++;
		} while (TIFFReadDirectory(image));
		free(buffer);
	}
	else if (bps == 32)
	{
		float* buffer;
		if((buffer = (float *) malloc(bufferSize)) == NULL){
			fprintf(stderr, "Could not allocate enough memory for the"
					" uncompressed image\n");
			exit(42);
		}
		//loop through the slices (multi pages or multiple directories)
		slice_num = 0;
		do {
			// read in the slice
			imageOffset = 0;
			for (stripCount = 0; stripCount < stripMax; stripCount++){
				if((result = TIFFReadEncodedStrip (image, stripCount,
						buffer + imageOffset, readWholeStrip)) == -1){
					fprintf(stderr, "Read error on input strip number "
							"%d\n", stripCount);
					exit(42);
				}
				imageOffset += result/bytesPerSample;
			}
			// transfer the slice to the image array
			for (i = 0; i < slice_elements; i++)
			{
				img_array[i + slice_num*slice_elements] = (float) *(buffer + i);
			}

		    slice_num++;
		} while (TIFFReadDirectory(image));
		free(buffer);
	}
	else if (bps == 64)
	{
		double* buffer;
		if((buffer = (double *) malloc(bufferSize)) == NULL){
			fprintf(stderr, "Could not allocate enough memory for the"
					" uncompressed image\n");
			exit(42);
		}
		//loop through the slices (multi pages or multiple directories)
		slice_num = 0;
		do {
			// read in the slice
			imageOffset = 0;
			for (stripCount = 0; stripCount < stripMax; stripCount++){
				if((result = TIFFReadEncodedStrip (image, stripCount,
						buffer + imageOffset, readWholeStrip)) == -1){
					fprintf(stderr, "Read error on input strip number "
							"%d\n", stripCount);
					exit(42);
				}
				imageOffset += result/bytesPerSample;
			}
			// transfer the slice to the image array
			for (i = 0; i < slice_elements; i++)
			{
				img_array[i + slice_num*slice_elements] = (float) *(buffer + i);
			}

		    slice_num++;
		} while (TIFFReadDirectory(image));
		free(buffer);
	}
	else
	{
		fprintf(stderr, "Image does not have the correct bit information\n");
		return -1;
	}

	TIFFClose(image);

	printf("done importing tiff stack!\n");


	return 0;
}

int WriteTiffStacks(float* img_array, int* shape, const char* filename)
{
	/* This functions is for writing 32 bit images*/
	TIFF *image;
	uint16 photo, bps, spp, fillorder;
	uint32 width, length;
	tsize_t stripSize;
	unsigned long imageOffset, result;
	int stripMax, stripCount, bytesPerSample;
	char tempbyte;
	unsigned long bufferSize, count;
	int readWholeStrip = -1;
	int num_slices;
	int num_elements;
	int slice_num;
	int i;
	long slice_elements;
	float* buffer;
	int bitsPerPixel, bytesPerPixel;

	num_slices = shape[0];
	length = shape[1];
	width = shape[2];

	bitsPerPixel = 32;
	bytesPerPixel = 4;
	bufferSize = length * width * bytesPerPixel;
	slice_elements = length * width;

	// Open the TIFF image for writing
	if((image = TIFFOpen(filename, "w")) == NULL){
		fprintf(stderr, "Could not open incoming image\n");
		exit(42);
	}

	// Creating the buffer for the image slices
	if((buffer = (float *) malloc(bufferSize)) == NULL){
		fprintf(stderr, "Could not allocate enough memory for "
				"the uncompressed image\n");
		exit(42);
	}

	for (slice_num = 0; slice_num < num_slices; slice_num++)
	{
		// transfer slice into the buffer
		for (i = 0; i < slice_elements; i++)
			buffer[i] = (float) img_array[i + slice_num*slice_elements];

		// Adding the Tag information
		TIFFSetField(image, TIFFTAG_IMAGEWIDTH, width);
		TIFFSetField(image, TIFFTAG_IMAGELENGTH, length);
		TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, bitsPerPixel);
		TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 1);
		TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(image,
				length * width * bytesPerPixel));

		TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
		TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
		TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
		TIFFSetField(image, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);

		// Write the information to the file
		if((result = TIFFWriteEncodedStrip(image, 0, buffer, length * width * bytesPerPixel)) == -1){
			fprintf(stderr, "Read error on input strip number %d\n", 0);
			exit(42);
		}

		// Write info and go to next page
		TIFFWriteDirectory(image);
	}

	TIFFClose(image);
	free(buffer);

	printf("done saving tiff stack!\n");

	return 0;
}

int ReadTiffStacks(float** img_array, int* shape, const char* filename)
{
	TIFF *image;
	uint16 photo, bps, spp, fillorder;
	uint32 width, length;
	tsize_t stripSize;
	unsigned long imageOffset, result;
	int stripMax, stripCount, bytesPerSample;
	char tempbyte;
	unsigned long bufferSize, count;
	int readWholeStrip = -1;
	int num_slices;
	int num_elements;
	int slice_num;
	int i;
	long slice_elements;
	char type[5];


	// check to make sure file is a tiff file
	IdentifyImageType(type, filename);
	if (strcmp(type, "TIFF") != 0)
	{
		fprintf(stderr, "ERROR: Import files must be of type TIFF\n");
		exit(42);
	}

	// Open the TIFF image
	if((image = TIFFOpen(filename, "r")) == NULL){
		fprintf(stderr, "Could not open incoming image\n");
		exit(42);
	}

	// Check the Data Type
	if((TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &bps) == 0)){
		fprintf(stderr, "Undefined number of bits per sample\n");
		exit(42);
	}

	if((TIFFGetField(image, TIFFTAG_SAMPLESPERPIXEL, &spp) == 0) || (spp != 1)){
		fprintf(stderr, "Either undefined or unsupported number of "
				"samples per pixel\n");
		exit(42);
	}

	// get the image length, width, and num slices
	if(TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &width) == 0){
		fprintf(stderr, "Image does not define its width\n");
		exit(42);
	}
	if(TIFFGetField(image, TIFFTAG_IMAGELENGTH, &length) == 0){
		fprintf(stderr, "Image does not define its length\n");
		exit(42);
	}

	// Get the number of slices
	num_slices = NumSlices(filename);

	shape[0] = num_slices;
	shape[1] = length;
	shape[2] = width;

	// allocate memory for the array
	num_elements = MAX(1,length) * MAX(1,width) * MAX(1,num_slices);
	slice_elements = MAX(1,length) * MAX(1,width);
	*img_array = (float*) malloc(num_elements * sizeof(float));
    if (*img_array == NULL)
    {
        perror("malloc failed");
        fprintf(stderr, "Value of errno: %d\n", errno);
        exit(EXIT_FAILURE);
    }

	// Read in the possibly multiple strips and slices
	bytesPerSample = MAX(bps/8,1);
	stripSize = TIFFStripSize (image);
	stripMax = TIFFNumberOfStrips (image);
	bufferSize = TIFFNumberOfStrips (image) * stripSize;

	if (bps == 1 || bps == 8)
	{
		unsigned char* buffer;
		if((buffer = (unsigned char *) malloc(bufferSize)) == NULL){
			fprintf(stderr, "Could not allocate enough memory for "
					"the uncompressed image\n");
			exit(42);
		}
		//loop through the slices (multi pages or multiple directories)
		slice_num = 0;
		do {
			// read in the slice
			imageOffset = 0;
			for (stripCount = 0; stripCount < stripMax; stripCount++){
				if((result = TIFFReadEncodedStrip (image, stripCount,
						buffer + imageOffset, readWholeStrip)) == -1){
					fprintf(stderr, "Read error on input strip number"
							" %d\n", stripCount);
					exit(42);
				}
				imageOffset += result/bytesPerSample;
			}
			// transfer the slice to the image array
			for (i = 0; i < slice_elements; i++)
			{
				(*img_array)[i + slice_num*slice_elements] = (float) *(buffer + i);
			}

		    slice_num++;
		} while (TIFFReadDirectory(image));
		free(buffer);
	}
	else if (bps == 16)
	{
		uint16* buffer;
		if((buffer = (uint16 *) malloc(bufferSize)) == NULL){
			fprintf(stderr, "Could not allocate enough memory for the"
					" uncompressed image\n");
			exit(42);
		}
		//loop through the slices (multi pages or multiple directories)
		slice_num = 0;
		do {
			// read in the slice
			imageOffset = 0;
			for (stripCount = 0; stripCount < stripMax; stripCount++){
				if((result = TIFFReadEncodedStrip (image, stripCount,
						buffer + imageOffset, readWholeStrip)) == -1){
					fprintf(stderr, "Read error on input strip number"
							" %d\n", stripCount);
					exit(42);
				}
				imageOffset += result/bytesPerSample;
			}
			// transfer the slice to the image array
			for (i = 0; i < slice_elements; i++)
			{
				(*img_array)[i + slice_num*slice_elements] = (float) *(buffer + i);
			}

		    slice_num++;
		} while (TIFFReadDirectory(image));
		free(buffer);
	}
	else if (bps == 32)
	{
		float* buffer;
		if((buffer = (float *) malloc(bufferSize)) == NULL){
			fprintf(stderr, "Could not allocate enough memory for the"
					" uncompressed image\n");
			exit(42);
		}
		//loop through the slices (multi pages or multiple directories)
		slice_num = 0;
		do {
			// read in the slice
			imageOffset = 0;
			for (stripCount = 0; stripCount < stripMax; stripCount++){
				if((result = TIFFReadEncodedStrip (image, stripCount,
						buffer + imageOffset, readWholeStrip)) == -1){
					fprintf(stderr, "Read error on input strip number "
							"%d\n", stripCount);
					exit(42);
				}
				imageOffset += result/bytesPerSample;
			}
			// transfer the slice to the image array
			for (i = 0; i < slice_elements; i++)
			{
				(*img_array)[i + slice_num*slice_elements] = (float) *(buffer + i);
			}

		    slice_num++;
		} while (TIFFReadDirectory(image));
		free(buffer);
	}
	else if (bps == 64)
	{
		double* buffer;
		if((buffer = (double *) malloc(bufferSize)) == NULL){
			fprintf(stderr, "Could not allocate enough memory for the"
					" uncompressed image\n");
			exit(42);
		}
		//loop through the slices (multi pages or multiple directories)
		slice_num = 0;
		do {
			// read in the slice
			imageOffset = 0;
			for (stripCount = 0; stripCount < stripMax; stripCount++){
				if((result = TIFFReadEncodedStrip (image, stripCount,
						buffer + imageOffset, readWholeStrip)) == -1){
					fprintf(stderr, "Read error on input strip number "
							"%d\n", stripCount);
					exit(42);
				}
				imageOffset += result/bytesPerSample;
			}
			// transfer the slice to the image array
			for (i = 0; i < slice_elements; i++)
			{
				(*img_array)[i + slice_num*slice_elements] = (float) *(buffer + i);
			}

		    slice_num++;
		} while (TIFFReadDirectory(image));
		free(buffer);
	}
	else
	{
		fprintf(stderr, "Image does not have the correct bit information\n");
		return -1;
	}

	TIFFClose(image);

	printf("done importing tiff stack!\n");


	return 0;
}

int NumSlices(const char* filename)
{
	int dircount;

    TIFF* tif = TIFFOpen(filename, "r");
    if (tif) {
	dircount = 0;
	do {
	    dircount++;
	} while (TIFFReadDirectory(tif));
	TIFFClose(tif);
    }

    return dircount;
}

int IdentifyImageType(char *Type, const char *FileName)
{
    FILE *File;
    uint32_t Magic;


    Type[0] = '\0';

    if(!(File = fopen(FileName, "rb")))
        return 0;

    /* Determine the file format by reading the first 4 bytes */
    Magic = ((uint32_t)getc(File));
    Magic |= ((uint32_t)getc(File)) << 8;
    Magic |= ((uint32_t)getc(File)) << 16;
    Magic |= ((uint32_t)getc(File)) << 24;

    /* Test for errors */
    if(ferror(File))
    {
        fclose(File);
        return 0;
    }

    fclose(File);

    if((Magic & 0x0000FFFFL) == 0x00004D42L)                /* BMP */
        strcpy(Type, "BMP");
    else if((Magic & 0x00FFFFFFL) == 0x00FFD8FFL)           /* JPEG/JFIF */
        strcpy(Type, "JPEG");
    else if(Magic == 0x474E5089L)                           /* PNG */
        strcpy(Type, "PNG");
    else if(Magic == 0x002A4949L || Magic == 0x2A004D4DL)   /* TIFF */
        strcpy(Type, "TIFF");
    else if(Magic == 0x38464947L)                           /* GIF */
        strcpy(Type, "GIF");
    else if(Magic == 0x474E4D8AL)                           /* MNG */
        strcpy(Type, "MNG");
    else if((Magic & 0xF0FF00FFL) == 0x0001000AL            /* PCX */
        && ((Magic >> 8) & 0xFF) < 6)
        strcpy(Type, "PCX");
    else
        return 0;

    return 1;
}

