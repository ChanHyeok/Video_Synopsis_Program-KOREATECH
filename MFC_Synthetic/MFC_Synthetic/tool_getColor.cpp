#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "MFC_SyntheticDlg.h"
#include "afxdialogex.h"

// 색상 값 정규화 함수
// opencv에서 제공하는 H(0-180), S(0-255), V(0-255)를  기존의 H(0-360),S(0-100), V(0-100) 색공간으로 바꿔줌
int getColor_H(int h) {
	return int(h * 2);
}
int getColor_S(int s) {
	return int(s * 100 / 255);
}
int getColor_V(int v) {
	return int(v * 100 / 255);
}


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

	// HSV 채널로 충분히 검출이 가능한 색상들
	if (H >= 165 || H <= 8) {
		colorArray[RED]++;
	}
	else if (H <= 22) {
		colorArray[ORANGE]++;
	}
	else if (H <= 37) {
		colorArray[YELLOW]++;
	}
	else if (H <= 85) {
		colorArray[GREEN]++;
	}
	else if (H <= 140) {
		colorArray[BLUE]++;
	}
	else if (H <= 164) {
		colorArray[MAGENTA]++;
	}
	else
		color_point--;

	// RGB를 이용하여 검출을 할 색상들(Black, Gray, White)
	if (R >= 0 && R <= 50 && G >= 0 && G <= 50 && B >= 0 && B <= 50) {
		colorArray[BLACK]++;
		color_point++;
	}
	if (R >= 60 && R <= 120 && G >= 60 && G <= 120 && B >= 60 && B <= 120) {
		colorArray[WHITE]++;
		color_point++;
	}
	if (R >= 30 && R <= 50 && G >= 30 && G <= 50 && B >= 30 && B <= 50) {	// Gray인지 판별
		colorArray[GRAY]++;
		color_point++;
	}

	else
		return color_point;

	return color_point;
}
