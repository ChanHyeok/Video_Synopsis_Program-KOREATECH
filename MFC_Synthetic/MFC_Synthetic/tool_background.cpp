#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "MFC_SyntheticDlg.h"
#include "afxdialogex.h"
#include <opencv2\opencv.hpp>
#include <opencv\highgui.h>
#include <opencv\cv.h>

//TemporalMedian 방식으로 배경을 만들어 사용하기
Mat temporalMedianBG(Mat frameimg, Mat bgimg, int rows, int cols) {
	int cnt = 0; // 현재픽셀값과 이전 픽셀값과 비교하여 바뀌지 않으면(같으면) 카운팅함(딱히쓸일없음)
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			if (frameimg.data[i * frameimg.cols + j] > bgimg.data[i * bgimg.cols + j]) {//현재 픽셀이 배경 픽셀보다 클 때
				if (bgimg.data[i * bgimg.cols + j] == 255) // 연산할 이미지 배열의 값이 255가 넘을 경우( 최대값 )
					bgimg.data[i * bgimg.cols + j] = 255;
				else
					bgimg.data[i * bgimg.cols + j] += 1;//1씩 증가

			} // 배경 프레임과 비교하여 현재 프레임의 화소 값이 높은 경우, 다음 배경 프레임의 화소를 증가 
			else if (frameimg.data[i * frameimg.cols + j] < bgimg.data[i * bgimg.cols + j]) {//현재 픽셀이 배경 픽셀보다 작을 때
				if (bgimg.data[i * bgimg.cols + j] == 0) // 연산할 이미지 배열의 값이 0보다 작을 경우( 최소값 )
					bgimg.data[i *bgimg.cols + j] = 0;
				else
					bgimg.data[i * bgimg.cols + j] -= 1;//1씩 감소
			} // 배경 프레임과 비교하여 현재 프레임의 화소 값이 낮은 경우, 다음 배경 프레임의 화소를 감소
			else if (frameimg.data[i * frameimg.cols + j] == bgimg.data[i * bgimg.cols + j]) {
				cnt++;
			}
		}
	}

	return bgimg;
}
