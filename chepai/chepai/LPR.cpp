#include "LPR.h"

bool comp(RotatedRect a, RotatedRect b)
{
	return a.center.x < b.center.x;
}

float PR(Mat &src, Mat &dst, int num)
{
	int temp = 0;
	float result = 0;
// 	imshow("src", src);
// 	imshow("dst", dst);
// 	waitKey(0);

	for (int i = 0; i < src.rows; i++)
	{
		for (int j = 0; j < src.cols; j++)
		{
			if (src.ptr(i)[j] == dst.ptr(i)[j])
			{
				temp++;
			}
		}
	}
	result = (float)temp / (src.rows*src.cols);
	return result;
}

LPR::LPR()
{

}

LPR::LPR(String path)
{
	this->load(path);
}

void LPR::load(String path)
{
	//��ȡ����ͼƬ
	srcImage = imread(path);
	if (srcImage.empty())
	{
		cout << "�����·��!" << endl;
/*		exit(-1);*/
	}
	cvtColor(srcImage, markedImage, CV_16S);
}

void LPR::showSrc()
{
	//��ʾԭͼ
	//imshow("ԭͼ",srcImage);
}

void LPR::gaussFilter()
{
	//��˹�˲�
	GaussianBlur(srcImage, gaussImage, Size(3, 3), 0);
/*	imshow("��Ч��ͼ����˹�˲�", gaussImage);*/
}

void LPR::sobel()
{
	//sobel����
	//���� grad_x �� grad_y ����  
	Mat grad_x, grad_y;
	Mat abs_grad_x, abs_grad_y;

	//�� X�����ݶ�  
	Sobel(gaussImage, grad_x, CV_16S, 1, 0, 3, 1, 1, BORDER_DEFAULT);
	convertScaleAbs(grad_x, abs_grad_x);
/*	imshow("��Ч��ͼ�� X����Sobel", abs_grad_x);*/

	//��Y�����ݶ�  
	Sobel(gaussImage, grad_y, CV_16S, 0, 1, 3, 1, 1, BORDER_DEFAULT);
	convertScaleAbs(grad_y, abs_grad_y);
/*	imshow("��Ч��ͼ��Y����Sobel", abs_grad_y);*/

	//�ϲ��ݶ�(����)  
	addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, sobelImage);
	//imshow("Sobel��Ե���", sobelImage);
}

void LPR::thresholding()
{
	//��ɫ�ռ�ת������
	cvtColor(sobelImage, binImage, CV_RGB2GRAY);

	//threshold ������ͨ�������Ҷ�ͼ�е㣬��ͼ����Ϣ��ֵ������������ͼƬֻ�ж���ɫֵ��
	threshold(binImage, binImage, 180, 255, CV_THRESH_BINARY);
	//imshow("��ֵ������", binImage);
}

void LPR::closeOperation()
{
	//�����
	Mat element = getStructuringElement(MORPH_RECT, Size(17, 17));
	//������̬ѧ����
	morphologyEx(binImage, closeImage, MORPH_CLOSE, element);
	//imshow("������", closeImage);
}

void LPR::removeLittle()
{
	//��ʴ����
	Mat ele = getStructuringElement(MORPH_RECT, Size(1, 1));//getStructuringElement����ֵ�����ں˾���5��5��
	erode(closeImage, closeImage, ele);//erode����ֱ�ӽ��и�ʴ����
	//imshow("��Ч��ͼ��ȥ��С����", closeImage);

	//���Ͳ���
	ele = getStructuringElement(MORPH_RECT, Size(17, 10));//getStructuringElement����ֵ�����ں˾���17��5��
	dilate(closeImage, closeImage, ele);//dilate����ֱ�ӽ������Ͳ���
	imshow("��ʴ������", closeImage);
}

void LPR::getMaxArea()
{
	//ȡ����
	vector< vector< Point> > contours;
	findContours(closeImage,
		contours, // a vector of contours
		CV_RETR_EXTERNAL, // ��ȡ�ⲿ����
		CV_CHAIN_APPROX_NONE); // all pixels of each contours
	int max = 0;
	for (size_t i = 0; i < contours.size(); i++)
	{
		vector<Point> p;
		p = contours[i];
		if (p.size() > max)
		{
			max = p.size();
			maxArea = p;
		}
	}
	//�����������
	for (size_t i = 0; i < maxArea.size(); i++)
	{
		circle(markedImage, maxArea[i], 1, Scalar(240, 255, 25));
	}

	//������С��Ӿ���
	minRect = minAreaRect(maxArea);
	minRect.points(P);
	for (int j = 0; j <= 3; j++)
	{
		line(markedImage, P[j], P[(j + 1) % 4], Scalar(255), 2);
	}
	//imshow("��Ч��ͼ�����", markedImage);
}

void LPR::affine()
{
	//����任
	Point2f srcTri[3];
	Point2f dstTri[3];
	Mat rot_mat(2, 3, CV_32FC1);
	Mat warp_mat(2, 3, CV_32FC1);

	//�������������������任
	//����
	int tep = markedImage.cols;
	for (size_t i = 0; i < maxArea.size(); i++)
	{
		if (maxArea[i].x < tep)
		{
			tep = maxArea[i].x;
			srcTri[0] = maxArea[i];
		}
	}
	//����
	tep = 0;
	for (size_t i = 0; i < maxArea.size(); i++)
	{
		if (maxArea[i].y > tep)
		{
			tep = maxArea[i].y;
			srcTri[1] = maxArea[i];
		}
	}
	//����
	tep = 0;
	for (size_t i = 0; i < maxArea.size(); i++)
	{
		if (maxArea[i].x > tep)
		{
			tep = maxArea[i].x;
			srcTri[2] = maxArea[i];
		}
	}

	//Scalar(blue,green,red)
	circle(markedImage, srcTri[0], 5, Scalar(255));
	circle(markedImage, srcTri[1], 5, Scalar(0, 255));
	circle(markedImage, srcTri[2], 5, Scalar(0, 0, 255));

	imshow("��������", markedImage);

	dstTri[0] = Point2f(0, 0);
	dstTri[1] = Point2f(0, minRect.size.height);
	dstTri[2] = Point2f(minRect.size.width, minRect.size.height);

	//�������任����
	warp_mat = getAffineTransform(srcTri, dstTri);
	//�Լ���ͼ�ν��з���任����
	warpAffine(gaussImage, plateImage, warp_mat, minRect.size);
	imshow("����任", plateImage);

}

void LPR::reThreshold()
{
	//�ٴζ�ֵ��
	threshold(plateImage, reBinImage, 180, 255, CV_THRESH_BINARY_INV);
	//imshow("�ٴζ�ֵ��", reBinImage);
}

char* U2G(const char* utf8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);
	len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);
	if (wstr) delete[] wstr;
	return str;
}

char* G2U(const char* gb2312)
{
	int len = MultiByteToWideChar(CP_ACP, 0, gb2312, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_ACP, 0, gb2312, -1, wstr, len);
	len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
	if (wstr) delete[] wstr;
	return str;
}

void LPR::recognition()
{
	TessBaseAPI orc;
	const char *langue = "eng+chi_sim";
	//char *langue1 = "";
	orc.Init(NULL, langue, OEM_TESSERACT_ONLY);
	orc.SetVariable("tessedit_write_images", "1");
	orc.SetPageSegMode(PSM_SINGLE_BLOCK);
	orc.SetImage(reBinImage.data, reBinImage.cols, reBinImage.rows, reBinImage.elemSize(), reBinImage.cols);
	char *text = orc.GetUTF8Text();//.GetUTF8Text();
	String s = U2G(text);
	cout << s << endl;
	waitKey(0);
}

void LPR::processing()
{
	//��˹�˲�
	gaussFilter();
	//sobel����
	sobel();
	//��ֵ��
	thresholding();
	//������
	closeOperation();
	//ȥ��С����
	removeLittle();
	//ȡ�������
	getMaxArea();
	//����任
	affine();
	//�ٴζ�ֵ��
	reThreshold();
	//ʶ���ַ�
	recognition();
}


LPR::~LPR()
{
	destroyAllWindows();
}

