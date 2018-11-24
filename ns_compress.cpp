

#include "utils.h"

int ReadAndVerify(ocl_args_d_t *ocl, unsigned char* input, cl_uint size, unsigned char* buf)
{
	printf("verify\n");
	cl_int err = CL_SUCCESS;
	//bool result = true;

	//char* buf = (char *)malloc(sizeof(char)*(size * 5 / 4)); //保存压缩后的文件
	char* count_buf = (char *)malloc(sizeof(char)*(size / 4)); //将设备上的output读取到主机

	// Enqueue a command to map the buffer object (ocl->dstMem) into the host address space and returns a pointer to it
	// The map operation is blocking
	//cl_int *resultPtr = (cl_int *)clEnqueueMapBuffer(ocl->commandQueue, ocl->srcB, true, CL_MAP_READ, 0, size, 0, NULL, NULL, &err);
	//char *resultPtr = (char *)clEnqueueMapBuffer(ocl->commandQueue, ocl->srcB, true, CL_MAP_READ, 0, size, 0, NULL, NULL, &err);

	clEnqueueReadBuffer(ocl->commandQueue, ocl->srcB, true, 0, size / 4 * sizeof(unsigned char), count_buf, 0, NULL, NULL);
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


	//FILE *fout = fopen("C:/Users/Administrator/Desktop/data_test/com_test.txt", "wb");
	int num, outsize = 2, integer_num = 0, count;
	buf[0] = '1';   //解压时使用，表明使用ns
	buf[1] = size % 16;
	unsigned char index;
	//outsize += 4; //预留4个字节保存压缩后一共多少字节
	for (int i = 0; i < size / 16; ++i) {
		count = 0;
		index = 0;
		++outsize;
		for (int j = 0; j < 4; ++j) {
			num = count_buf[i];
			//printf("num:%d\n", num);
			count += num;
			switch (num) {
			case 1: {
				break;
			}
			case 2: {
				index |= (1 << (6 - 2 * j));
				break;
			}
			case 3: {
				index |= (1 << (7 - 2 * j));
				break;
			}
			case 4: {
				index |= (3 << (6 - 2 * j));
				break;
			}
			}
			//printf("%d\n", num);
			//buf[outsize++] = input[integer_num * 4 ];
			for (int k = 0; k < num; ++k) {
				//fprintf(fout, "%d ", buf[j++]);
				buf[outsize++] = input[integer_num * 4 + k];
				//printf("char: %c\n", input[integer_num*4+k]);
			}
			++integer_num;
		}
		buf[outsize - count - 1] = index;
		//printf("index pos:%d\n", outsize - count - 1);
	}
	//buf[outsize++] = -1; //标志压缩后内容的末尾
	//printf("%d\n", num);
	//fwrite(buf, sizeof(char), outsize, fout);
	//fclose(fout);
	//printf("output size:%d\n", outsize);
	// Unmapped the output buffer before releasing it
  // err = clEnqueueUnmapMemObject(ocl->commandQueue, ocl->srcB, resultPtr, 0, NULL, NULL);
	if (CL_SUCCESS != err)
	{
		LogError("Error: clEnqueueUnmapMemObject returned %s\n", TranslateOpenCLError(err));
		system("pause");
	}

	return outsize;
}


int ns_compress( MyStruct* compression_options, unsigned char* data, int size, unsigned char* compressed_output, bool with_checksum, int checksum)
{
	cl_int err;
	int outsize;
	unsigned char* buf = (unsigned char *)malloc(sizeof(char)*(size / 4)); //硬件上的输出内存映射

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
	compression_options->ocl.kernel = clCreateKernel(compression_options->ocl.ns_program, "NS", &err);
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
	size_t globalWorkSize[1] = { size / 4+1 };

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
	outsize = ReadAndVerify(&compression_options->ocl, data, size, compressed_output);
	int remaining = size % 16;           //The data that are not as many as 16 bytes
	for (remaining; remaining > 0; --remaining) {
		compressed_output[outsize++] = data[size - remaining];
	}
	// retrieve performance counter frequency
	return outsize;
}

int ns_uncompress(MyStruct* compression_options, unsigned char* in, int insize, unsigned char* out, bool with_checksum, int checksum)
{
	unsigned int inpos, outpos, remaining = in[1];
	inpos = 2;  //跳过标志压缩算法的字节和末尾没有压缩的字节
	outpos = 0;
	unsigned char count, temp;
	unsigned int size = insize;
	insize = insize - remaining;

	while (inpos < insize)
	{
		count = in[inpos++];
		if (count == -1) break; //读到文件末
		temp = (count | 63);
		switch (temp)
		{
		case 63:
			out[outpos++] = in[inpos++];
			out[outpos++] = 0;
			out[outpos++] = 0;
			out[outpos++] = 0;
			break;
		case 127:
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = 0;
			out[outpos++] = 0;
			break;
		case 191:
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = 0;
			break;
		case 255:
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			break;
		}

		temp = (count | 207);
		switch (temp)
		{
		case 207:
			out[outpos++] = in[inpos++];
			out[outpos++] = 0;
			out[outpos++] = 0;
			out[outpos++] = 0;
			break;
		case 223:
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = 0;
			out[outpos++] = 0;
			break;
		case 239:
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = 0;
			break;
		case 255:
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			break;
		}

		temp = (count | 243);
		switch (temp)
		{
		case 243:
			out[outpos++] = in[inpos++];
			out[outpos++] = 0;
			out[outpos++] = 0;
			out[outpos++] = 0;
			break;
		case 247:
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = 0;
			out[outpos++] = 0;
			break;
		case 251:
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = 0;
			break;
		case 255:
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			break;
		}

		temp = (count | 252);
		switch (temp)
		{
		case 252:
			out[outpos++] = in[inpos++];
			out[outpos++] = 0;
			out[outpos++] = 0;
			out[outpos++] = 0;
			break;
		case 253:
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = 0;
			out[outpos++] = 0;
			break;
		case 254:
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = 0;
			break;
		case 255:
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			out[outpos++] = in[inpos++];
			break;
		}


	}

	for ( remaining; remaining > 0; --remaining) {
		out[outpos++] = in[size - remaining];
	}

	return outpos;

}