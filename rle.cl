__kernel void RLE(__global unsigned char* input, __global unsigned char* output, const int insize) // size���ֽ�Ϊ��λ
{ 
	int id = get_global_id(0);

	unsigned char count = 1;
	int i = id;

	while(i<insize-1 && input[i]==input[i+1] && count < 127){    //�ӵ�ǰλ�ÿ�ʼ���ж��ٸ��ظ��ֽ�
		++i;
		++count;
	}
	output[id] = count;
	/*
	if(id <256){                    //ֱ��ͼ��ʼ��
		histogram[id] = 0;
	}

	if(id < insize){ 
		atomicAdd(&histogram[input[id]], 1);   //ԭ�Ӳ������ܱ���д��ͻ
	}
	*/

}