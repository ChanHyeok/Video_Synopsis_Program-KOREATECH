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

int colorPicker(Vec3b pixel) {
	unsigned char H = pixel[0];
	unsigned char S = pixel[1];
	unsigned char V = pixel[2];

	//Black인지 White인지 판별
	if (V <= 38) {
		return BLACK;
	}
	else if (S <= 38 && V >= 166) {
		return WHITE;
	}
	else if (S <= 25) {	//Gray인지 판별
		return GRAY;
	}
	else if (H >= 165 || H <= 8) {
		return RED;
	}
	else if (H <= 22) {
		return ORANGE;
	}
	else if (H <= 37) {
		return YELLOW;
	}
	else if (H <= 85) {
		return GREEN;
	}
	else if (H <= 140) {
		return BLUE;
	}
	else if (H <= 164) {
		return MAGENTA;
	}
	else
		return-1;
}
