/*
 * tiff_io.h
 *
 *  Created on: Oct 3, 2016
 *      Author: CSmith
 *
 *  Header file used for reading and writing tiff files.
 */

#ifndef TIFF_IO_H_
#define TIFF_IO_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef __UINT32_TYPE__ uint32_t;

/* Function used to determine the type of image using just the first four bits
 * of the header file.
*/
int IdentifyImageType(char *Type, const char *FileName);

/* Function used for reading tiff stacks into memory. The data is arranged
 * in array as [num_slices, n_rows, n_cols] as well as shape.
*/
int ReadTiffStacks(float** array, int* shape, const char* filename);

/* Used to determine the number of slices (directories) are in a tiff image.
 * This function is used when reading in tiff files.
*/
int NumSlices(const char* filename);

/* Function used for writing tiff stacks to file as 32 bit floating point images.
 * The data is arranged as [num_slices, n_rows, n_cols] as well as shape. This
 * function works with both single page or multipage images.
*/
int WriteTiffStacks(float* img_array, int* shape, const char* filename);

/*
    This function is used to determine the size and type of the array before
    loading into memory.
*/
int TiffInfo(const char* filename, int* shape);
/* 
   Used for reading stacks in Python. Memory is allocated in python rather
   than being done dynamically in C
*/
int PythonReadTiffStacks(float* img_array, int* shape, const char* filename);


#ifdef __cplusplus
}
#endif

#endif /* TIFF_IO_H_ */
