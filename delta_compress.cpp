#include "utils.h"


int delta_compress(MyStruct* compression_options, int* data, int size, int* compressed_output, bool with_checksum, int checksum)
{
	if (size == 0)
		return 0;
	cl_int err;
	int outsize;
	int* buf = (int *)malloc(sizeof(char)*size + 1); //硬件上的输出内存映射

	//initialize Open CL objects (context, queue, etc.)

	err = CL_SUCCESS;

	// Create first image based on host memory input
	compression_options->ocl.srcA = clCreateBuffer(compression_options->ocl.context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, size, data, &err);
	if (CL_SUCCESS != err)
	{
		LogError("Error: clCreateBuffer for srcA returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	// Create second image based on host memory inputB
	//ocl->srcB = clCreateBuffer(ocl->context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, size / 4, output, &err);
	compression_options->ocl.srcB = clCreateBuffer(compression_options->ocl.context, CL_MEM_WRITE_ONLY, size, NULL, &err);
	if (CL_SUCCESS != err)
	{
		LogError("Error: clCreateBuffer for srcB returned %s\n", TranslateOpenCLError(err));
		return err;
	}


	// Create and build the OpenCL program
//	if (CL_SUCCESS != CreateAndBuildProgram(ocl, 1))
//	{
//		return -1;
//	}

	// Program consists of kernels.
	// Each kernel can be called (enqueued) from the host part of OpenCL application.
	// To call the kernel, you need to create it from existing program.
	compression_options->ocl.kernel = clCreateKernel(compression_options->ocl.delta_program, "DELTA", &err);
	if (CL_SUCCESS != err)
	{
		LogError("Error: clCreateKernel returned %s\n", TranslateOpenCLError(err));
		return -1;
	}

	// Passing arguments into OpenCL kernel.
	if (CL_SUCCESS != SetKernelArguments(&compression_options->ocl, size/4))
	{
		return -1;
	}


	// Execute (enqueue) the kernel
	err = CL_SUCCESS;

	// Define global iteration space for clEnqueueNDRangeKernel.
	size_t globalWorkSize[1] = { size/4 };

	// execute kernel
	err = clEnqueueNDRangeKernel(compression_options->ocl.commandQueue, compression_options->ocl.kernel, 1, NULL, globalWorkSize, NULL, 0, NULL, NULL);
	if (CL_SUCCESS != err)
	{
		LogError("Error: Failed to run kernel, return %s\n", TranslateOpenCLError(err));
		return err;
	}

	// Wait until the queued kernel is completed by the device
	err = clFinish(compression_options->ocl.commandQueue);
	if (CL_SUCCESS != err)
	{
		LogError("Error: clFinish return %s\n", TranslateOpenCLError(err));
		return err;
	}
	if (CL_SUCCESS != err)
	{
		return -1;
	}


	// The last part of this function: getting processed results back.
	// use map-unmap sequence to update original memory area with output buffer.
	err = CL_SUCCESS;
	//int* count_buf = (int *)malloc(sizeof(char)*size); //将设备上的output读取到主机
	clEnqueueReadBuffer(compression_options->ocl.commandQueue, compression_options->ocl.srcB, true, 0, size * sizeof(unsigned char), compressed_output, 0, NULL, NULL);

	if (CL_SUCCESS != err)
	{
		LogError("Error: clEnqueueMapBuffer returned %s\n", TranslateOpenCLError(err));
		return 0;
	}

	// Call clFinish to guarantee that output region is updated
	err = clFinish(compression_options->ocl.commandQueue);
	if (CL_SUCCESS != err)
	{
		LogError("Error: clFinish returned %s\n", TranslateOpenCLError(err));
	}
	// retrieve performance counter frequency
	return size;
}

int delta_uncompress(MyStruct* compression_options, int* in, int size, int* out, bool with_checksum, int checksum)
{
	if (size == 0)
		return 0;
	unsigned int inpos, outpos;
	inpos = 0;
	outpos = 0;
	int prev, value;
	prev = in[inpos++];
	out[outpos++] = prev;
	while (inpos * 4 < size)
	{
		value = prev + in[inpos++];
		out[outpos++] = value;
		prev = value;
	}

	return outpos * 4;

}