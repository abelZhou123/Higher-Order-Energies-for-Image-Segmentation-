#pragma once
#include <ezi/Image2D.h>
#include <ezi/Table2D.h>
#include <SparseMatrix.h>
#include <assert.h>


// get average sigma_square for smoothness term
double getsmoothnessterm(const Table2D<RGB> &img,vector<PointPair> & pointpairs,int connecttype);
void rgb2indeximg(Table2D<int> & indeximg,const Table2D<RGB> & img,double channelstep);
int getcompactlabel(Table2D<int> & colorlabel,double channelstep,vector<int> & compacthist);
// edge-constrast sensitive smoothness penalty
inline double fn(const double dI, double lambda,double sigma_square);

class Image{
public:
	Image();
	Image(Table2D<RGB> img_, double channelstep, const char * imgname_ = "", int connecttype_ = 16);
	Image(const char * imgpath, const char * imgname_, double channelstep, int connecttype_);
	void computesmoothnesscost();
	void print();                        //��ӡ�����Ϣ

	Table2D<RGB> img;
	const char * imgname;
	int img_w;
	int img_h;
	int img_size;
	double sigma_square;
	vector<PointPair> pointpairs;
	vector<double> smoothnesscosts;
	int colorbinnum;
	Table2D<int> colorlabel;
	vector<int> compacthist; // color bin histogram (whole image) ���ֱ��ͼ��ȥ����Ϊ0��bin��
	int connecttype; // can be 4 or 8 or 16

	vector<Trituple<double>> pair_arcs;
};

Image::Image()
{

}

void Image::computesmoothnesscost()
{
	// number of neighboring pairs of pixels
	int numNeighbor = pointpairs.size();
	smoothnesscosts = vector<double>(numNeighbor);
	for(int i=0;i<numNeighbor;i++)
	{
		PointPair pp = pointpairs[i];
		smoothnesscosts[i] = fn(dI(img[pp.p1],img[pp.p2]),1.0,sigma_square)/(pp.p1-pp.p2).norm();
	} 
	//dI(a,b): a��b����ɫrgb�����ƽ�������
	//fn: exp(-...)  
	//(pp.p1-pp.p2).norm():  p1��p2��ŷ�Ͼ���

	pair_arcs = vector<Trituple<double>>();
	for(int i=0;i<numNeighbor;i++)
	{
		Point p1 = pointpairs[i].p1;
		Point p2 = pointpairs[i].p2;
		pair_arcs.push_back(Trituple<double>(p1.x+p1.y*img_w,p2.x+p2.y*img_w,smoothnesscosts[i]));
		pair_arcs.push_back(Trituple<double>(p2.x+p2.y*img_w,p1.x+p1.y*img_w,smoothnesscosts[i]));
	}

}

//��������һ��Image�����ʱ�򣬾ͻ��Զ��õ��ܶණ��������
//ͼ��ͼ������֣������������������ڽӣ��������е�sigma^2
//pointpair������PointPair����smoothnesscosts������double����pair_arcs��������Ԫ�飩�� ���ǳ��ȶ����ھӶԵ�����
//ֱ��ͼcompacthist(û��ֵΪ0��bin)��ֱ��ͼ�ĳ���colorbinnum
//��С��ͼ��һ���ľ���colorlabel
Image::Image(Table2D<RGB> img_, double channelstep, const char * imgname_, int connecttype_)
{
	assert((connecttype_==4)||(connecttype_==8)||(connecttype_==16),"wrong connect type!");
	imgname = imgname_;
	img = Table2D<RGB>(img_);
	img_w = img.getWidth();
	img_h = img.getHeight();
	img_size = img_w * img_h;

	connecttype = connecttype_;

	//������������Ǽ�����sigma^2, ���õ���pointpairs����
	sigma_square = getsmoothnessterm(img,pointpairs,connecttype);
	
	colorlabel= Table2D<int>(img_w,img_h);

	//���Ǻܹؼ���һ��������������colorlabel: ÿ�����ص���indexΪ���ٵ�ֱ��ͼbin��ͷ
	rgb2indeximg(colorlabel,img,channelstep);   //channelstep��ֱ��ͼ��ÿ��bin�Ŀ��

	//��ԭֱ��ͼ��Ϊ0��binȥ�����µ�ֱ��ͼ��bin����������colorbinnum
	//�µ�ֱ��ͼ����compacthist
	//colorlabelҲ�ᱻ�޸�,�洢����ÿ�����������µ�ֱ��ͼ�е�bin�����
	colorbinnum = getcompactlabel(colorlabel,channelstep,compacthist);

	//�������������������ȶ����ھӶԵ�����
	//smoothnesscosts������������ھӼ�Ķ����������ֵ
	//pair_arcs������Ԫ�飬�ֱ��ǵ�һ�����ڶ��� �ھ� ��ͼ���е�index�������Ƕ�Ӧ��smoothnesscosts
	computesmoothnesscost();
}


Image::Image(const char * imgpath, const char * imgname_, double channelstep, int connecttype_)
{
	Table2D<RGB> img_ = loadImage<RGB>(imgpath);
	new (this)Image(img_, channelstep, imgname_, connecttype_);
}
void Image::print()
{
	cout<<"sigma_square: "<<sigma_square<<endl;   //sigma^2,�����ڶ�����ĵط��õ�
	cout<<"image width: "<<img_w<<endl;
	cout<<"image height: "<<img_h<<endl;
	cout<<"# of pairs of neighborhoods"<<pointpairs.size()<<endl;    //�ھӵ������������������
	cout<<"# of color bins"<<colorbinnum<<endl;                      //rgb��ͨ����ֱ��ͼ����һ����߽���У�����ô���bin����ʽ����
	cout<<"size of compact hist: "<<compacthist.size()<<endl;
}

//����һ���ܹؼ��ĺ�������Ϊ���õ��˺ܶ���Ϣ��
//�õ���������Ҫ�ģ�sigma^2  ��  ���е��ھӹ�ϵ�ԣ�pointpairs����
double getsmoothnessterm(const Table2D<RGB> &img,vector<PointPair> & pointpairs, int connecttype)
{
	int node_id = 0;
	int img_w = img.getWidth();
	int img_h = img.getHeight();
	double sigma_sum = 0;
	double sigma_square_count = 0;
	Point kernelshifts [] = {Point(1,0),Point(0,1),Point(1,1),Point(1,-1),
		Point(2,-1),Point(2,1),Point(1,2),Point(-1,2),};
	for (int y=0; y<img_h; y++) // adding edges (n-links)
	{
		for (int x=0; x<img_w; x++) 
		{ 
			Point p(x,y);
			for(int i=0;i<connecttype/2;i++)
			{
				Point q = p + kernelshifts[i];
				if(img.pointIn(q))
				{
					sigma_sum += dI(img[p],img[q]);
					pointpairs.push_back(PointPair(p,q));
					sigma_square_count ++;
				}
			}
		}
	}
	return sigma_sum/sigma_square_count;
}

//���������Ϊ�˵õ�indeximg
//indeximg��һ��ͼ���С�ľ���ÿ����ֵ��ʾ�������λ�õ����ص�������3ͨ��ֱ��ͼ�ڼ���bin
/*eg:rgbֵ=��50��10��30�� channelstep=16,��ʾ����bin�Ŀ�ȣ����ԣ� 0~15 16~31 32~47 48~63 ...
��ô������ص���rgb�ֱ�Ϊ (r_idx,g_idx,b_idx) = (3,0,2) ��bin��ͷ��
����ͨ����ֱ��ͼ�У�һ���� ��256/channelstep�� ^3 = 4096��bin����ŷֱ��� 0��4095
��ô������ؾ͵��� 3 + 0*16 +2*16*16 ���bin����
*/
void rgb2indeximg(Table2D<int> & indeximg,const Table2D<RGB> & img,double channelstep)
{
	RGB rgb_v;
	int r_idx =0, g_idx = 0, b_idx = 0, idx =0;
	int channelbin = (int)ceil(256.0/channelstep);  //ÿ��ͨ���� ֱ��ͼ�� bin����
	for(unsigned int j=0;j<img.getHeight();j++)
	{
		for(unsigned int i=0;i<img.getWidth();i++)
		{
			rgb_v = img[i][j];
			r_idx = (int)(rgb_v.r/channelstep);
			g_idx = (int)(rgb_v.g/channelstep);
			b_idx = (int)(rgb_v.b/channelstep);
			idx = r_idx + g_idx*channelbin+b_idx*channelbin*channelbin;
			indeximg[i][j] = idx;
		}
	}
}

//��ԭͼ��ֱ��ͼ�Ŀյ�binȥ�����õ����µ�ֱ��ͼcompacthist
//�������أ��µ�ֱ��ͼ��bin������
//colorlabel���ں����б��޸�:�洢������compacthist�е����
int getcompactlabel(Table2D<int> & colorlabel,double channelstep,vector<int> & compacthist)
{
	int channelbin = (int)ceil(256/channelstep);
	vector<int> colorhist(channelbin*channelbin*channelbin,0);
	for(unsigned int j=0;j<colorlabel.getHeight();j++)
	{
		for(unsigned int i=0;i<colorlabel.getWidth();i++)
		{
			colorhist[colorlabel[i][j]] = colorhist[colorlabel[i][j]]+1;
		}
	}  //�õ�ԭͼ��ֱ��ͼ
	
	vector<int> correspondence(colorhist.size(),-1);
	int compactcount = 0;
	for(unsigned int i=0;i<colorhist.size();i++)
	{
		if(colorhist[i]!=0)
		{
			compacthist.push_back(colorhist[i]);
			correspondence[i] = compactcount;
			compactcount++;
		}
	} //��ԭ��ֱ��ͼcolorhist��Ϊ0��binȥ�����õ�compacthist�� 
	  //�µ�ֱ��ͼcompacthist��ÿһ��bin����Ӧ����ԭֱ��ͼ�е�bin����ţ���������conrespondence��ͷ

	for(unsigned int j=0;j<colorlabel.getHeight();j++)
	{
		for(unsigned int i=0;i<colorlabel.getWidth();i++)
		{
			colorlabel[i][j] = correspondence[colorlabel[i][j]];
		}
	}
	return compacthist.size();
}

inline double fn(const double dI, double lambda,double sigma_square) 
{
	return lambda*(exp(-dI/2/sigma_square));
}