#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <vector>
#include <string>
#include "CL\cl.h"
#include "CL\cl_ext.h"
//#include <d3d9.h>


#pragma once

/* Convenient container for all OpenCL specific objects used in the sample
 *
 * It consists of two parts:
 *   - regular OpenCL objects which are used in almost each normal OpenCL applications
 *   - several OpenCL objects that are specific for this particular sample
 *
 * You collect all these objects in one structure for utility purposes
 * only, there is no OpenCL specific here: just to avoid global variables
 * and make passing all these arguments in functions easier.
 */
struct ocl_args_d_t
{
	ocl_args_d_t();
	~ocl_args_d_t();

	// Regular OpenCL objects:
	cl_context       context;           // hold the context handler
	cl_device_id     device;            // hold the selected device handler
	cl_command_queue commandQueue;      // hold the commands-queue handler
	cl_program       ns_program;           // hold the program handler
	cl_program       rle_program;
	cl_program       delta_program;
	cl_program       undelta_program;
	cl_kernel        kernel;            // hold the kernel handler
	float            platformVersion;   // hold the OpenCL platform version (default 1.2)
	float            deviceVersion;     // hold the OpenCL device version (default. 1.2)
	float            compilerVersion;   // hold the device OpenCL C version (default. 1.2)

	// Objects that are specific for algorithm implemented in this sample
	cl_mem           srcA;              // hold first source buffer
	cl_mem           srcB;              // hold second source buffer
	cl_mem           srcC;
	cl_mem           srcD;
};

struct MyStruct {
	char* data_type;
	int algo;        //选择要使用的压缩算法，1：null suppression  2：run length coding
	ocl_args_d_t ocl;
};

int FPGA_Compress( MyStruct* compression_options, unsigned char* data, int size, unsigned char* compressed_output, bool with_checksum, int checksum);
int FPGA_Uncompress(MyStruct* compression_options, unsigned char* data, int size, unsigned char* raw_data_output, bool with_checksum, int checksum);

int ns_compress( MyStruct* compression_options, unsigned char* data, int size, unsigned char* compressed_output, bool with_checksum, int checksum);
int ns_uncompress(MyStruct* compression_options, unsigned char* data, int size, unsigned char* raw_data_output, bool with_checksum, int checksum);
int rle_compress(MyStruct* compression_options, unsigned char* data, int size, unsigned char* compressed_output, bool with_checksum, int checksum);
int rle_uncompress(MyStruct* compression_options, unsigned char* data, int size, unsigned char* raw_data_output, bool with_checksum, int checksum);
int delta_compress(MyStruct* compression_options, int* data, int size, int* compressed_output, bool with_checksum, int checksum);
int delta_uncompress(MyStruct* compression_options, int* data, int size, int* raw_data_output, bool with_checksum, int checksum);

int CreateBufferArguments(ocl_args_d_t *ocl, unsigned char* input, unsigned char* output, int size);
int CreateAndBuildProgram(ocl_args_d_t *ocl, int algo);
int SetupOpenCL(ocl_args_d_t *ocl, cl_device_type deviceType);
int CreateAndBuildProgram(ocl_args_d_t *ocl);
cl_uint SetKernelArguments(ocl_args_d_t *ocl, int size);

const char* TranslateOpenCLError(cl_int errorCode);

// Print useful information to the default output. Same usage as with printf
void LogInfo(const char* str, ...);

// Print error notification to the default output. Same usage as with printf
void LogError(const char* str, ...);

// Read OpenCL source code from fileName and store it in source. The number of read bytes returns in sourceSize
int ReadSourceFromFile(const char* fileName, char** source, size_t* sourceSize);

