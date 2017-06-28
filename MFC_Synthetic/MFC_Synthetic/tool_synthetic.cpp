#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "MFC_SyntheticDlg.h"
#include "afxdialogex.h"
#include <opencv2\opencv.hpp>
#include <opencv\highgui.h>
#include <opencv\cv.h>

Mat Syn_Background_Foreground(Mat background, Mat firstForegroundImage, Mat secondForegroundImage, int rows, int cols) {
	Mat result(rows, cols, CV_8UC1);
	// 합성한 프레임 만들기
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) { // 배경과 전경이 겹치는 부분(전경값이 0이 아닌 부분)
			if (firstForegroundImage.data[i * cols + j] > 20 || secondForegroundImage.data[i * cols + j] > 20) {	//전경이미지에서 전경 부분
				result.data[i * cols + j] =
					(firstForegroundImage.data[i * cols + j] + secondForegroundImage.data[i * cols + j]) / 2;
			}
			else { // 전경이 아닌 부분
				result.data[i * cols + j] = background.data[i * cols + j];
			}
		}
	}
	return result;
}

Mat printObjOnBG(Mat background, Mat frame, segment obj, int* labelMap){
	// TO DO :: 블렌딩 보완하기, 강건화
	for (int i = obj.top; i < obj.bottom; i++) {
		for (int j = obj.left + 1; j < obj.right; j++) {
			Vec3b colorB = background.at<Vec3b>(Point(j, i));
			Vec3b colorO = frame.at<Vec3b>(Point(j - obj.left, i - obj.top));

			if (labelMap[i * background.cols + j] == 1) {	//이전 객체가 이미 있을 경우
				colorB[0] = (colorB[0] + colorO[0]) / 2;
				colorB[1] = (colorB[1] + colorO[1]) / 2;
				colorB[2] = (colorB[2] + colorO[2]) / 2;
				background.at<Vec3b>(Point(j, i)) = colorB;
			} //Blending
			else {
				labelMap[i * background.cols + j] = 1;		//객체를 그렸다고 표시
				colorB[0] = colorO[0];
				colorB[1] = colorO[1];
				colorB[2] = colorO[2];
				background.at<Vec3b>(Point(j, i)) = colorB;

			}
		}
	}

	return background;
}
