#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "MFC_SyntheticDlg.h"
#include "afxdialogex.h"
#include <opencv2\opencv.hpp>
#include <opencv\highgui.h>
#include <opencv\cv.h>

// People Area Detection Parameter
const int MINWIDTH = 89;
const int MINHEIGHT = 178;
const int MAXWIDTH = 336;
const int MAXHEIGHT = 325;


component dataAllocateAtComponent(Mat stats, component c, int indexOflables) {
	// 영역 저장하기
	c.area = stats.at<int>(indexOflables, CC_STAT_AREA);

	// 사각 끝점 저장하기
	c.left = stats.at<int>(indexOflables, CC_STAT_LEFT); // Left
	c.top = stats.at<int>(indexOflables, CC_STAT_TOP); // Top
	c.right = stats.at<int>(indexOflables, CC_STAT_LEFT) + stats.at<int>(indexOflables, CC_STAT_WIDTH); // right
	c.bottom = stats.at<int>(indexOflables, CC_STAT_TOP) + stats.at<int>(indexOflables, CC_STAT_HEIGHT); // Height

	// 높이 너비 저장하기
	c.height = stats.at<int>(indexOflables, CC_STAT_HEIGHT);
	c.width = stats.at<int>(indexOflables, CC_STAT_WIDTH);

	return c;
}

// 사각형 쳐주고 만드는 함수
Rect savingRectangle(Mat frame, component c) {
	// 사각형 화면에 쳐주기
	rectangle(frame, Point(c.left, c.top), Point(c.right, c.bottom), Scalar(0, 255, 0), 1);
	// 사각형 저장
	Rect objectRegion(c.left, c.top, c.width, c.height);
	return objectRegion;
}

// 레이블 크기를 사람정도 들어갈만하게 거르는 함수
int labelSizeFiltering(Mat frame, int width, int height) {
	if (width > MINWIDTH && height > MINHEIGHT
		&& width < MAXWIDTH && height < MAXHEIGHT)
		return 1; // true
	else
		return 0; // false
}

// Component Labelling(opencv내 함수 connectedComponentsWithStats를 이용하여)
vector<component> connectedComponentsLabelling(Mat frame, int rows, int cols) {
	vector<component> result;
	result.clear();
	Rect objectRegion(0, 0, 30, 30); // 레이블 저장할 사각형
	component *componentArray = (component*)calloc(999, sizeof(component)); // componentArray에 공간 할당

	Mat img_labels, stats, centroids;
	int numOfLables = connectedComponentsWithStats(frame, img_labels,
		stats, centroids, 8, CV_32S); // label 갯수 반환

	int index = 0;
	for (int i = 1; i < numOfLables; i++) {
		// height, width를 미리 지정
		int height = stats.at<int>(i, CC_STAT_HEIGHT);
		int width = stats.at<int>(i, CC_STAT_WIDTH);
		//printf("%d %d %d\n", i, width, height);
		// 영역박스 그리기, 레이블 크기를 필터링 하여(사람크기에 해당될 만큼)
		if (labelSizeFiltering(frame, width, height)) {
			// 유효한 레이블 인덱스를 저장
			componentArray[index].label = index;

			// Component에 데이터 저장
			componentArray[index] = dataAllocateAtComponent(stats, componentArray[index], i);
			// Rect타입 변수에 레이블된 오브젝트 저장
			objectRegion = savingRectangle(frame, componentArray[index]);
			result.push_back(componentArray[index]);

			index++;
		}
	}

	return result;
}