__kernel void DELTA(__global int* input, __global int* output, const int insize) // size以4字节为单位
{
	int id = get_global_id(0);

	unsigned char count = 1;

	if (id == 0)
		output[0] = input[0];
	else if (id < insize)
		output[id] = input[id] - input[id-1];

}