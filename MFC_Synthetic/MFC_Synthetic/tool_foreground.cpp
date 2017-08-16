#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "MFC_SyntheticDlg.h"
#include "afxdialogex.h"
#include <opencv2\opencv.hpp>
#include <opencv\highgui.h>
#include <opencv\cv.h>
// openCV 내장함수 MOG2를 이용하여 background를 분리해내는 함수
Mat ExtractForegroundToMOG2(Mat frameimg) {
	Mat result_frame;
	Ptr<BackgroundSubtractor> pMOG2;

	pMOG2 = createBackgroundSubtractorMOG2();
	pMOG2->apply(frameimg, result_frame);


	return result_frame;
}

Mat ExtractFg(Mat frameimg, Mat bgimg, int rows, int cols) {
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			if (abs(frameimg.data[i * frameimg.cols + j] - bgimg.data[i * bgimg.cols + j]) < FOREGROUND_THRESHOLD)
				frameimg.data[i * bgimg.cols + j] = 0;

		}
	}
	return frameimg;
}
