__kernel void RLE(__global unsigned char* input, __global unsigned char* output, const int insize) // size以字节为单位
{ 
	int id = get_global_id(0);

	unsigned char count = 1;
	int i = id;

	while(i<insize-1 && input[i]==input[i+1] && count < 127){    //从当前位置开始，有多少个重复字节
		++i;
		++count;
	}
	output[id] = count;
	/*
	if(id <256){                    //直方图初始化
		histogram[id] = 0;
	}

	if(id < insize){ 
		atomicAdd(&histogram[input[id]], 1);   //原子操作才能避免写冲突
	}
	*/

}