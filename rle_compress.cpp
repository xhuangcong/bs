#include "utils.h"

static void _RLE_WriteRep(unsigned char *out, unsigned int *outpos,
	unsigned char marker, unsigned char symbol, unsigned int count)
{
	unsigned int i, idx;

	idx = *outpos;
	if (count <= 3)
	{
		if (symbol == marker)
		{
			out[idx++] = marker;
			out[idx++] = count - 1;
		}
		else
		{
			for (i = 0; i < count; ++i)
			{
				out[idx++] = symbol;
			}
		}
	}
	else
	{
		out[idx++] = marker;
		--count;
		if (count >= 128)
		{
			out[idx++] = (count >> 8) | 0x80;
		}
		out[idx++] = count & 0xff;
		out[idx++] = symbol;
	}
	*outpos = idx;
}

static void _RLE_WriteNonRep(unsigned char *out, unsigned int *outpos,
	unsigned char marker, unsigned char symbol)
{
	unsigned int idx;

	idx = *outpos;
	if (symbol == marker)
	{
		out[idx++] = marker;
		out[idx++] = 0;
	}
	else
	{
		out[idx++] = symbol;
	}
	*outpos = idx;
}

int ReadAndProcess(ocl_args_d_t *ocl, unsigned char* input, cl_uint insize, unsigned char* out)
{
	cl_int err = CL_SUCCESS;
	//bool result = true;

	//char* buf = (char *)malloc(sizeof(char)*(size * 5 / 4)); //保存压缩后的文件
	char* count_buf = (char *)malloc(sizeof(char)*insize); //将设备上的output读取到主机
	//int* hash_buf = (int *)malloc(sizeof(int) * 256);

	// Enqueue a command to map the buffer object (ocl->dstMem) into the host address space and returns a pointer to it
	// The map operation is blocking
	//cl_int *resultPtr = (cl_int *)clEnqueueMapBuffer(ocl->commandQueue, ocl->srcB, true, CL_MAP_READ, 0, size, 0, NULL, NULL, &err);
	//char *resultPtr = (char *)clEnqueueMapBuffer(ocl->commandQueue, ocl->srcB, true, CL_MAP_READ, 0, size, 0, NULL, NULL, &err);

	clEnqueueReadBuffer(ocl->commandQueue, ocl->srcB, true, 0, insize * sizeof(unsigned char), count_buf, 0, NULL, NULL);

	if (CL_SUCCESS != err)
	{
		LogError("Error: clEnqueueMapBuffer returned %s\n", TranslateOpenCLError(err));
		return 0;
	}

	// Call clFinish to guarantee that output region is updated
	err = clFinish(ocl->commandQueue);
	if (CL_SUCCESS != err)
	{
		LogError("Error: clFinish returned %s\n", TranslateOpenCLError(err));
	}

	unsigned char byte1, byte2, marker;
	unsigned int  inpos, outpos, count, i, histogram[256];


	/* Do we have anything to compress? */
	if (insize < 1)
	{
		return 0;
	}

	/* Create histogram */
	for (i = 0; i < 256; ++i)
	{
		histogram[i] = 0;
	}
	for (i = 0; i < insize; ++i)
	{
		++histogram[input[i]];
	}

	/* Find the least common byte, and use it as the repetition marker */
	marker = 0;
	for (i = 1; i < 256; ++i)
	{
		if (histogram[i] < histogram[marker])
		{
			marker = i;
		}
	}

	/* Remember the repetition marker for the decoder */
	out[0] = '2'; //解压时使用，表明使用rle算法
	out[1] = marker;
	outpos = 2;

	byte1 = input[0];
	inpos = 0;
	count = count_buf[0];
	//printf("%c\n", marker);

	while (inpos < insize) {
		if (count > 1) {
			_RLE_WriteRep(out, &outpos, marker, byte1, count);
			//printf("%c %d\n", byte1, count);
		}
		else {
			_RLE_WriteNonRep(out, &outpos, marker, byte1);
			//printf("%c %d\n", byte1, count);
		}
		inpos += count;
		count = count_buf[inpos];
		byte1 = input[inpos];
	}

	//printf("%d\n", outpos);

	//FILE *f = fopen("C:/Users/Administrator/Desktop/data_test/compress_test1.txt", "wb");
	//if (f == NULL)
	//	printf("read failed\n");
	//fwrite(out, sizeof(char), outpos, f);

	return outpos;

}

int rle_compress(MyStruct* compression_options, unsigned char* data, int size, unsigned char* compressed_output, bool with_checksum, int checksum)
{
	cl_int err;
	int outsize;
	unsigned char* buf = (unsigned char *)malloc(sizeof(char)*size); //硬件上的输出内存映射

	//initialize Open CL objects (context, queue, etc.)

	if (CL_SUCCESS != CreateBufferArguments(&compression_options->ocl, data, buf, size))
	{
		return -1;
	}
	

	// Create and build the OpenCL program
//	if (CL_SUCCESS != CreateAndBuildProgram(ocl, 1))
//	{
//		return -1;
//	}

	// Program consists of kernels.
	// Each kernel can be called (enqueued) from the host part of OpenCL application.
	// To call the kernel, you need to create it from existing program.
	compression_options->ocl.kernel = clCreateKernel(compression_options->ocl.rle_program, "RLE", &err);
	if (CL_SUCCESS != err)
	{
		LogError("Error: clCreateKernel returned %s\n", TranslateOpenCLError(err));
		return -1;
	}

	// Passing arguments into OpenCL kernel.
	if (CL_SUCCESS != SetKernelArguments(&compression_options->ocl, size))
	{
		return -1;
	}
	

	// Execute (enqueue) the kernel
	err = CL_SUCCESS;

	// Define global iteration space for clEnqueueNDRangeKernel.
	size_t globalWorkSize[1] = { size };

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
	outsize = ReadAndProcess(&compression_options->ocl, data, size, compressed_output);

	// retrieve performance counter frequency
	return outsize;
}

int rle_uncompress(MyStruct* compression_options, unsigned char* in, int insize, unsigned char* out, bool with_checksum, int checksum)
{
	unsigned char marker, symbol;
	unsigned int  i, inpos, outpos, count;


	/* Do we have anything to uncompress? */
	if (insize < 1)
	{
		return 0;
	}

	/* Get marker symbol from input stream */
	inpos = 1; //跳过第一个标志算法的字节
	marker = in[inpos++];

	/* Main decompression loop */
	outpos = 0;
	do
	{
		symbol = in[inpos++];
		if (symbol == marker)
		{
			/* We had a marker byte */
			count = in[inpos++];
			if (count <= 2)
			{
				/* Counts 0, 1 and 2 are used for marker byte repetition
				   only */
				for (i = 0; i <= count; ++i)
				{
					out[outpos++] = marker;
					//printf("%c ", marker);
				}
			}
			else
			{
				if (count & 0x80)
				{
					count = ((count & 0x7f) << 8) + in[inpos++];
				}
				symbol = in[inpos++];
				for (i = 0; i <= count; ++i)
				{
					out[outpos++] = symbol;
					//printf("%c ", symbol);
				}
			}
		}
		else
		{
			/* No marker, plain copy */
			out[outpos++] = symbol;
			//printf("%c ", symbol);
		}
	} while (inpos < insize);
	return outpos;
}