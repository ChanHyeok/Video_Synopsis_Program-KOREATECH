#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "MFC_SyntheticDlg.h"
#include "afxdialogex.h"


// 객체 추출 또는 합성을 용이하게 하기 위한
// 이미지를 깔끔하게 처리하기 위한 함수들로 구성되어 있음

// 이미지 평활화 같은 연산들을 구현하여 등록할 수 있음

// 모폴로지 연산
Mat morphologyOpening(Mat img_binary) {
	//morphological opening 작은 점들을 제거
	erode(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	dilate(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	return img_binary;
}

Mat morphologyClosing(Mat img_binary) {
	//morphological closing 영역의 구멍 메우기
	dilate(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	erode(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	return img_binary;
}

Mat pretreatmentOperator(Mat img_binary) {
	// 이진화
	threshold(img_binary, img_binary, 4, 255, CV_THRESH_BINARY);

	// 노이즈 제거
	GaussianBlur(img_binary, img_binary, Size(11, 11), 0, 0, BORDER_DEFAULT);
	img_binary = morphologyOpening(img_binary);
	img_binary = morphologyClosing(img_binary);
	img_binary = morphologyClosing(img_binary);
//	blur(img_binary, img_binary, Size(11, 11));
//	GaussianBlur(img_binary, img_binary, Size(9, 9), 0, 0, BORDER_DEFAULT);
	medianBlur(img_binary, img_binary, 9);

	threshold(img_binary, img_binary, 10, 255, CV_THRESH_BINARY);

	return img_binary;
}