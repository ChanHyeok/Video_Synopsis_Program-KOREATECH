#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "MFC_SyntheticDlg.h"
#include "afxdialogex.h"

//색상 정보를 검출하는 함수
/*opencv HSV range
H : 180 S : 255 V : 255
*/
void setColorData(component *object) {
	object->hsv_avarage;
}


int colorPicker(Vec3b pixel_hsv, Vec3b pixel_rgb, int *colorArray) {
	// 검출된 색깔의 갯수 (0일 경우 에러)
	int color_point = 1;
	
	// HSV, RGB 값 각각 할당하기
	unsigned char H = pixel_hsv[0];
	unsigned char S = pixel_hsv[1];
	unsigned char V = pixel_hsv[2];

	unsigned char R = pixel_rgb[0];
	unsigned char G = pixel_rgb[1];
	unsigned char B = pixel_rgb[2];

	// 원색에서 기본 +- 4 수치로 일단 해보겠음
	// HSV 채널로 충분히 검출이 가능한 색상들

	// +- 2로 감소
	if ( ( H >= 178 && H >= 180)|| (H >= 0 && H <= 2) ) { //  H :: 0 -> 0
		colorArray[RED]++;
	}

	// +- 8로 증가
	else if (H <= 27 && H >= 11) { // H :: 39 -> 19.5
		colorArray[ORANGE]++;
	}

	// + 10로 증가
	else if (H <= 40 && H >= 27) { // H :: 60 -> 30
		colorArray[YELLOW]++;
	} 

	// +- 8로 증가
	else if (H <= 68 && H >= 52) { // H :: 120 -> 60
		colorArray[GREEN]++;
	}

	// 
	else if (H <= 124 && H >= 116) { // H :: 240 -> 120
		colorArray[BLUE]++;
	}

	//
	else if (H <= 154 && H >= 146) { // H :: 300 -> 150
		colorArray[MAGENTA]++;
	}
	else
		color_point--;

	// RGB를 이용하여 검출을 할 색상들(Black, Gray, White)
	// RGB < 30
	if (R >= 0 && R <= 30 && G >= 0 && G <= 30 && B >= 0 && B <= 30) {
		colorArray[BLACK]++;
		color_point++;
	}

	// RGB > 50
	if (R >= 50 && R <= 200 && G >= 50 && G <= 200 && B >= 50 && B <= 200) {
		colorArray[WHITE]++;
		color_point++;
	}

	// 30 < RGB < 50
	if (R >= 30 && R <= 50 && G >= 30 && G <= 50 && B >= 30 && B <= 50) {	// Gray인지 판별
		colorArray[GRAY]++;
		color_point++;
	}

	else
		return color_point;

	return color_point;
}
