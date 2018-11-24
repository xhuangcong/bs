
#include "utils.h"

int init(MyStruct *compression_options)
{
	cl_int err;
	cl_device_type deviceType = CL_DEVICE_TYPE_GPU;

	if (CL_SUCCESS != SetupOpenCL(&compression_options->ocl, deviceType))
	{
		return -1;
	}

	if (CL_SUCCESS != CreateAndBuildProgram(&compression_options->ocl))
	{
		return -1;
	}
}


int main(int argc, int argv[])
{
	int algo = 3;  //选择压缩算法 algo=1使用空制抑制算法，algo=2使用游程编码；

	//LARGE_INTEGER perfFrequency;
	//LARGE_INTEGER performanceCountNDRangeStart;
	//LARGE_INTEGER performanceCountNDRangeStop;

	MyStruct compression_options;
	compression_options.algo = algo;

	init(&compression_options);      //初始化

	FILE *f = fopen("C:/Users/hc/Desktop/data_test/test1.txt", "rb");
	if (f == NULL)
		printf("read failed\n");
	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	fseek(f, 0, SEEK_SET);
	unsigned char* data = (unsigned char *)malloc(sizeof(char)*(size + 1));//输入
	unsigned char* uncompressed_data = (unsigned char *)malloc(sizeof(char)*(size + 1));
	unsigned char* compressed_output = (unsigned char *)malloc(sizeof(char)*(size * 5 / 4 + 5));// 压缩后的文件
	unsigned char* raw_data_output = (unsigned char *)malloc(sizeof(char)*(size + 1));

	if (NULL == data || NULL == compressed_output)
	{
		LogError("Error: failed to allocate buffers.\n");
		return -1;
	}
	printf("size of input data:%d\n", size);
	//读取文件内容到input指针
	if (size != fread(data, sizeof(char), size, f)) {
		free(data);
		LogError("Error: read text failed.\n");
		exit(EXIT_FAILURE);
	}
	fclose(f);

	printf("original data:\n");
	for (int i = 0; i < size; ++i) {
		printf("%c", data[i]);
	}
	printf("\n");

	int outsize;
	printf("compressing....\n");
	outsize = FPGA_Compress(&compression_options, data, size, compressed_output, false, 0);   //调用FPGA_Compress函数
	printf("size of compressed data: %d\n", outsize);
	printf("uncompressing....\n");
	outsize = FPGA_Uncompress(&compression_options, compressed_output, outsize, uncompressed_data, false, 0);

	printf("uncompressed data:\n");
	for (int i = 0; i < outsize; ++i) {
		printf("%c", uncompressed_data[i]);
	}
	printf("\n");

	printf("size of output data: %d\n", outsize);

	system("pause");

	return 0;
}


/*
int _tmain(int argc, TCHAR* argv[])
{
	cl_int err;
	ocl_args_d_t ocl;
	cl_device_type deviceType = CL_DEVICE_TYPE_GPU;

	if (CL_SUCCESS != SetupOpenCL(&ocl, deviceType))
	{
		return -1;
	}

	if (CL_SUCCESS != CreateAndBuildProgram(&ocl, 2))
	{
		return -1;
	}
	LARGE_INTEGER perfFrequency;
	LARGE_INTEGER performanceCountNDRangeStart;
	LARGE_INTEGER performanceCountNDRangeStop;


	float t;
	MyStruct compression_options;
	compression_options.algo = 2;

	FILE *f = fopen("C:/Users/Administrator/Desktop/data_test/time.txt", "w");

	int size;

	for (int i = 1; i < 2; i += 2)
	{
		size = 1024*1024 * i;

		unsigned char* data = (unsigned char *)malloc(sizeof(char)*(size + 1));//输入
		unsigned char* compressed_output = (unsigned char *)malloc(sizeof(char)*(size * 5 / 4 + 5 ));// 压缩后的文件

		if (NULL == data || NULL == compressed_output)
		{
			LogError("Error: failed to allocate buffers.\n");
			return -1;
		}
		//printf("size of input data:%d\n", size);
		//读取文件内容到input指针


		t = FPGA_Compress(&ocl, &compression_options, data, size, compressed_output, false, 0);   //调用FPGA_Compress函数
		fprintf(f, "%f,", t);
		printf("kernel executing time %f ms.\n", t);

		//FPGA_Uncompress(NULL, compressed_output, size, raw_data_output, false, 0);   //调用FPGA_Uncompress函数

	}
	fclose(f);
	system("pause");

	return 0;
}*/