#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "MFC_SyntheticDlg.h"
#include "afxdialogex.h"

//색상 정보를 검출하는 함수
/*opencv HSV range
H : 180 S : 255 V : 255
*/

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

	// RGB의 합
	int sumOfRGB = R + G + B;
	
	// 원색에서 차이 범위 +- 조정
	// HSV 채널로 충분히 검출이 가능한 색상들

	// +- 3로 감소 (RGB이용)
	 if ( ( H >= 177 && H >= 180)|| (H >= 0 && H <= 3) && R >= 130)
	//if ( ( H >= 179 && H >= 180)|| (H >= 0 && H <= 1) )  //  H :: 0 -> 0
		colorArray[RED]++;

	// +10로 증가, - 10증가  (RGB이용)
	if (H <= 30 && H >= 10 && R >= 150)
	// if (H <= 24 && H >= 16)  // H :: 39 -> 19.5
		colorArray[ORANGE]++;

	// + 10로 증가, -로 16 증가  (RGB이용)
	 if (H <= 40 && H >= 14 && B <= 150 && abs(R-G) < 30)
	//if (H <= 40 && H >= 18)  // H :: 60 -> 30
		colorArray[YELLOW]++;

	// +- 6로 증가  (RGB이용)
	 	if (H <= 62 && H >= 54 && G >= 110) 
	//if (H <= 62 && H >= 54)  // H :: 120 -> 60
		colorArray[GREEN]++;

	// +- 19으로 증가  (RGB이용)
	 if (H >= 101 && H <= 139 && B >= 130)
	//if (H >= 102 && H <= 138)  // H :: 240 -> 120
		colorArray[BLUE]++;

	//
	if (H <= 154 && H >= 146) // H :: 300 -> 150
		colorArray[MAGENTA]++;

	// RGB를 이용하여 검출을 할 색상들(Black, Gray, White)
	
	// RGB합 < 65
	// if (R >= 0 && R <= 20 && G >= 0 && G <= 20 && B >= 0 && B <= 20) {
	if (sumOfRGB <= 65 && abs(R-B) < 15 && abs(B - G) < 13 && abs(G - R) < 13) {
		colorArray[BLACK]++;
		color_point++;
	}

	// RGB합 > 380
	// if (R >= 90 && R <= 255 && G >= 90 && G <= 255 && B >= 90 && B <= 255) {
	if (sumOfRGB >= 380 && abs(R - B) < 15 && abs(B - G) < 13 && abs(G - R) < 13) {
		colorArray[WHITE]++;
		color_point++;
	}

	// 20 < RGB < 50
	if (R >= 20 && R <= 50 && G >= 20 && G <= 50 && B >= 20 && B <= 50) {	// Gray인지 판별
		colorArray[GRAY]++;
		color_point++;
	}

	return color_point;
}
