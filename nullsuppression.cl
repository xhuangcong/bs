__kernel void NS(__global unsigned char* input, __global unsigned char* output, const int insize) //insize以4个字节为单位
{
	int id = get_global_id(0);

	if (id < insize) {
		unsigned char byte1, byte2, byte3, byte4;
		unsigned char num = 1;//每个整数需要多少个字节保存

		byte1 = input[id * 4];
		byte2 = input[id * 4 + 1];
		byte3 = input[id * 4 + 2];
		byte4 = input[id * 4 + 3];

		if (byte4 == 0) {
			if (byte3 == 0) {
				if (byte2 == 0) {
					if (byte1 != 0) {
						num = 1;
					}
				}
				else {
					num = 2;
				}
			}
			else {
				num = 3;
			}
		}
		else {
			num = 4;
		}

		//barrier(CLK_GLOBAL_MEM_FENCE);
		//if (id == 0)
			//output[insize * 5] = -1;
		output[id] = num;
	}
}