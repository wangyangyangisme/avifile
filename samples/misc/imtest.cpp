#include <avm_default.h>
#include <image.h>
#include <cpuinfo.h>
#include <stdio.h>

static const int WIDTH=640;
static const int HEIGHT=480;
static const char* names[]={"RGB15", "RGB16", "RGB24", "RGB32", "YUY2", "YV12"};
static BitmapInfo formats[6];

int main()
{
    formats[0]=BitmapInfo(WIDTH, HEIGHT, 15);
    formats[1]=BitmapInfo(WIDTH, HEIGHT, 16);
    formats[2]=BitmapInfo(WIDTH, HEIGHT, 24);
    formats[3]=BitmapInfo(WIDTH, HEIGHT, 32);
    formats[4]=BitmapInfo(WIDTH, HEIGHT, 24);
    formats[4].SetSpace(fccYUY2);
    formats[5]=BitmapInfo(WIDTH, HEIGHT, 24);
    formats[5].SetSpace(fccYV12);
    long t1, t2;
    printf("Quality of conversion ( 0 ideal, 255 worst ):\n");
    printf("\tRGB15\tRGB16\tRGB24\tRGB32\tYUY2\tYV12\n");
    for(int i=0; i<6; i++)
    {
	printf("%s\t", names[i]);
	CImage* im=new CImage(&formats[i]);
	CImage* im2;
        CImage* im3;
	int j;
	unsigned char* p=im->Data();
	for(j=0; j<im->Bytes(); j++)p[j]=(((j*64377)^8529)%200)+20;
	for(int j=0; j<6; j++)
	{
	    double sum=0;
	    im2=new CImage(im, &formats[j]);
	    im3=new CImage(im2, &formats[2]);
	    CImage* im4=new CImage(im, &formats[2]);
	    for(int k=0; k<im4->Bytes(); k++)
		sum+=abs(im4->Data()[k]-im3->Data()[k]);
	    printf("%.2f\t", sum/im4->Bytes());
	    delete im4;
	    delete im3;
	    delete im2;
	}
	printf("\n");
	delete im;
    }
    printf("Without flipping:\n");
    printf("\tRGB15\tRGB16\tRGB24\tRGB32\tYUY2\tYV12\n");
    for(int i=0; i<6; i++)
    {
	printf("%s\t", names[i]);
	CImage* im=new CImage(&formats[i]);
	CImage* im2;
	int j;
	unsigned char* p=im->Data();
	for(j=0; j<im->Bytes(); j++)p[j]=(((j*64377)^8529)%200)+20;
	for(int j=0; j<6; j++)
	{
	    t1=localcount();
	    im2=new CImage(im, &formats[j]);
	    t2=localcount();
	    printf("%.2f\t", double(t2-t1)/(WIDTH*HEIGHT));
	    delete im2;
	}
	printf("\n");
	delete im;
    }
    printf("With flipping:\n");
    printf("\tRGB15\tRGB16\tRGB24\tRGB32\tYUY2\tYV12\n");
    for(int i=0; i<6; i++)
    {
	printf("%s\t", names[i]);
	CImage* im=new CImage(&formats[i]);
	CImage* im2;
	int j;
	unsigned char* p=im->Data();
	for(j=0; j<im->Bytes(); j++)p[j]=(((j*64377)^8529)%200)+20;
	for(int j=0; j<6; j++)
	{
//	    printf("%d->%d\n", i, j);	
	    formats[j].biHeight*=-1;
	    t1=localcount();
	    im2=new CImage(im, &formats[j]);
	    t2=localcount();
	    formats[j].biHeight*=-1;
	    printf("%.2f\t", double(t2-t1)/(WIDTH*HEIGHT));
	    delete im2;
	}
	printf("\n");
	delete im;
    }
    return 0;
}
