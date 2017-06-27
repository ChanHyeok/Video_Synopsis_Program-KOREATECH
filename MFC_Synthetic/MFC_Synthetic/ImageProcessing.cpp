#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "MFC_SyntheticDlg.h"
#include "afxdialogex.h"


// 객체 추출 또는 합성을 용이하게 하기 위한
// 이미지를 깔끔하게 처리하기 위한 함수들로 구성되어 있음

// 이미지 평활화 같은 연산들을 구현하여 등록할 수 있음

// 모폴로지 연산
Mat morphologicalOperation(Mat img_binary) {
	//morphological opening 작은 점들을 제거
	erode(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	dilate(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

	//morphological closing 영역의 구멍 메우기
	dilate(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	erode(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	return img_binary;
}

Mat BlendingOperation(Mat background, Mat frame) {
	double alpha = 0.5, beta;
	beta = 1.0 - alpha;
	addWeighted(frame, alpha, background, beta, 0.0, background);

	// 겹침의 상태에 따라서 블렌딩의 정도를 조절(alpha와 beta값을 이용하여)


	return background;


}

