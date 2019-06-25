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
	//读取车牌图片
	srcImage = imread(path);
	if (srcImage.empty())
	{
		cout << "错误的路径!" << endl;
/*		exit(-1);*/
	}
	cvtColor(srcImage, markedImage, CV_16S);
}

void LPR::showSrc()
{
	//显示原图
	//imshow("原图",srcImage);
}

void LPR::gaussFilter()
{
	//高斯滤波
	GaussianBlur(srcImage, gaussImage, Size(3, 3), 0);
/*	imshow("【效果图】高斯滤波", gaussImage);*/
}

void LPR::sobel()
{
	//sobel运算
	//创建 grad_x 和 grad_y 矩阵  
	Mat grad_x, grad_y;
	Mat abs_grad_x, abs_grad_y;

	//求 X方向梯度  
	Sobel(gaussImage, grad_x, CV_16S, 1, 0, 3, 1, 1, BORDER_DEFAULT);
	convertScaleAbs(grad_x, abs_grad_x);
/*	imshow("【效果图】 X方向Sobel", abs_grad_x);*/

	//求Y方向梯度  
	Sobel(gaussImage, grad_y, CV_16S, 0, 1, 3, 1, 1, BORDER_DEFAULT);
	convertScaleAbs(grad_y, abs_grad_y);
/*	imshow("【效果图】Y方向Sobel", abs_grad_y);*/

	//合并梯度(近似)  
	addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, sobelImage);
	//imshow("Sobel边缘检测", sobelImage);
}

void LPR::thresholding()
{
	//颜色空间转换函数
	cvtColor(sobelImage, binImage, CV_RGB2GRAY);

	//threshold 方法是通过遍历灰度图中点，将图像信息二值化，处理过后的图片只有二种色值。
	threshold(binImage, binImage, 180, 255, CV_THRESH_BINARY);
	//imshow("二值化处理", binImage);
}

void LPR::closeOperation()
{
	//定义核
	Mat element = getStructuringElement(MORPH_RECT, Size(17, 17));
	//进行形态学操作
	morphologyEx(binImage, closeImage, MORPH_CLOSE, element);
	//imshow("闭运算", closeImage);
}

void LPR::removeLittle()
{
	//腐蚀操作
	Mat ele = getStructuringElement(MORPH_RECT, Size(1, 1));//getStructuringElement返回值定义内核矩阵（5，5）
	erode(closeImage, closeImage, ele);//erode函数直接进行腐蚀操作
	//imshow("【效果图】去除小区域", closeImage);

	//膨胀操作
	ele = getStructuringElement(MORPH_RECT, Size(17, 10));//getStructuringElement返回值定义内核矩阵（17，5）
	dilate(closeImage, closeImage, ele);//dilate函数直接进行膨胀操作
	imshow("腐蚀再膨胀", closeImage);
}

void LPR::getMaxArea()
{
	//取轮廓
	vector< vector< Point> > contours;
	findContours(closeImage,
		contours, // a vector of contours
		CV_RETR_EXTERNAL, // 提取外部轮廓
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
	//绘制最大区域
	for (size_t i = 0; i < maxArea.size(); i++)
	{
		circle(markedImage, maxArea[i], 1, Scalar(240, 255, 25));
	}

	//绘制最小外接矩形
	minRect = minAreaRect(maxArea);
	minRect.points(P);
	for (int j = 0; j <= 3; j++)
	{
		line(markedImage, P[j], P[(j + 1) % 4], Scalar(255), 2);
	}
	//imshow("【效果图】标记", markedImage);
}

void LPR::affine()
{
	//仿射变换
	Point2f srcTri[3];
	Point2f dstTri[3];
	Mat rot_mat(2, 3, CV_32FC1);
	Mat warp_mat(2, 3, CV_32FC1);

	//设置三个点来计算仿射变换
	//左上
	int tep = markedImage.cols;
	for (size_t i = 0; i < maxArea.size(); i++)
	{
		if (maxArea[i].x < tep)
		{
			tep = maxArea[i].x;
			srcTri[0] = maxArea[i];
		}
	}
	//左下
	tep = 0;
	for (size_t i = 0; i < maxArea.size(); i++)
	{
		if (maxArea[i].y > tep)
		{
			tep = maxArea[i].y;
			srcTri[1] = maxArea[i];
		}
	}
	//右下
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

	imshow("车牌区域", markedImage);

	dstTri[0] = Point2f(0, 0);
	dstTri[1] = Point2f(0, minRect.size.height);
	dstTri[2] = Point2f(minRect.size.width, minRect.size.height);

	//计算仿射变换矩阵
	warp_mat = getAffineTransform(srcTri, dstTri);
	//对加载图形进行仿射变换操作
	warpAffine(gaussImage, plateImage, warp_mat, minRect.size);
	imshow("仿射变换", plateImage);

}

void LPR::reThreshold()
{
	//再次二值化
	threshold(plateImage, reBinImage, 180, 255, CV_THRESH_BINARY_INV);
	//imshow("再次二值化", reBinImage);
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
	//高斯滤波
	gaussFilter();
	//sobel运算
	sobel();
	//二值化
	thresholding();
	//闭运算
	closeOperation();
	//去除小区域
	removeLittle();
	//取最大轮廓
	getMaxArea();
	//仿射变换
	affine();
	//再次二值化
	reThreshold();
	//识别字符
	recognition();
}


LPR::~LPR()
{
	destroyAllWindows();
}

