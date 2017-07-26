#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "MFC_SyntheticDlg.h"
#include "afxdialogex.h"
#include <opencv2\opencv.hpp>
#include <opencv\highgui.h>
#include <opencv\cv.h>
#include <direct.h>
#include <io.h>

// 파일 처리와 관련된 모든 함수들을 선언합니다.
// segment의 fileName할당, JPG 파일 저장, txt파일 저장
void saveSegmentation_JPG(component object, Mat frame, string video_path);
void saveSegmentation_TXT(component object, FILE *fp);
void saveSegmentation_TXT_detail(component object, FILE *fp, int, int);
int directionChecker(component object, int ROWS, int COLS);
string allocatingComponentFilename(int timeTag, int currentMsec, int frameCount, int indexOfhumanDetectedVector);

// segment 폴더 안에 Segmentation된 Obj만을 jpg파일로 저장하는 함수
Mat objectCutting(component object, Mat img, unsigned int ROWS, unsigned int COLS);

// Video Path에서 file이름만 빼서 확장자를 제거하여 반환하는 함수
String getFileName(CString f_path, char find_char, BOOL extension) {
	// 마지막 \ 뒤의 문자열
	// 검색대상 :: "Video (*.avi, *.MP4) | *.avi;*.mp4; | All Files(*.*)|*.*||",
	string f_name;
	char final_index;
	for (int i = 0; i < f_path.GetLength(); i++) {
		if (f_path[i] == find_char)
			final_index = i;
	} // '\'를 찾고

	for (int i = final_index + 1; i < f_path.GetLength(); i++) {
		if (f_path[final_index + 1] == NULL) break; // 예외처리
		if (extension == false){
			if (f_path[i] == '.') break; // 확장자 제거 (. 이후에 문자는 무시)
			// 폴더를 만드는데 확장자가 붙으면 확장자에 맞는 파일이 생성되는 오류가 있어서 일단 뺌
			// 혹시나 확장자가 필요할 경우 함수를 하나 더 생성하던지 해야할 듯
		}
		char c = f_path[i];
		f_name += c;
	}// 마지막에 나오는 '\' 이후의 문자열을 추출함

	return f_name;
}

// 전체 segment 데이터의 파일들을 저장하는 모듈
bool saveSegmentationData(string video_name, component object, Mat object_frame
	, int timeTag, int currentMsec, int frameCount, int indexOfhumanDetectedVector, FILE *txt_fp, FILE * txt_fp_detail, int ROWS, int COLS, vector<pair<int, int>>* vectorDetailTXTInedx, int* detailTxtIndex) {

	// object의 파일이름 할당
	object.fileName = allocatingComponentFilename(timeTag, currentMsec, frameCount, indexOfhumanDetectedVector);

	// jpg파일로 저장
	saveSegmentation_JPG(object, object_frame, getObjDirectoryPath(video_name));

	// txt파일로 저장
	saveSegmentation_TXT(object, txt_fp);

	//방향 정보 텍스트 파일 저장
	if (object.timeTag == currentMsec){//현재 오브젝트가 객체의 처음 일 경우
		saveSegmentation_TXT_detail(object, txt_fp_detail, ROWS, COLS);	//새롭게 텍스트 파일에 기록
		vectorDetailTXTInedx->push_back(std::make_pair(object.timeTag, (*detailTxtIndex)++));
	}
	else{	//첫 오브젝트가 아닐 경우 해당 객체 위치로 이동하여 last 위치 덮어쓰기
		int index = -1,i=0;
		long seek;
		int stamp;
		int tempFirst;
		int tempLast;
		char strTemp[255];

		for (i = 0; i < vectorDetailTXTInedx->size(); i++)	//벡터 검색
		if (vectorDetailTXTInedx->at(i).first == object.timeTag){
			index = vectorDetailTXTInedx->at(i).second;	//key(timetag)에 대한 value(텍스트 파일에서 몇 번째 라인인지)저장
			break;
		}

		//원하는 라인으로 이동
		if (index != -1){
			fseek(txt_fp_detail, 0, SEEK_SET);
			for (i = 0; i < index; i++)
				fgets(strTemp, sizeof(strTemp), txt_fp_detail);

			seek = ftell(txt_fp_detail);//덮어 쓰기를 할 위치
			fscanf(txt_fp_detail, "%d %d %d\n", &stamp, &tempFirst, &tempLast);
			fseek(txt_fp_detail, seek, SEEK_SET);//덮어쓸 위치로 이동
			tempLast = directionChecker(object, ROWS, COLS);//Last 위치 갱신
			//덮어쓰기
			fputs((to_string(object.timeTag).append(" ").append(to_string(tempFirst)).append(" ").append(to_string(tempLast)).append("\n")).c_str(), txt_fp_detail);

			fseek(txt_fp_detail, 0, SEEK_END);//파일 포인터 끝으로 이동
		}
		else
			perror("No such timetag");
	}

	return true;
}

void saveSegmentation_JPG(component object, Mat frame, string video_path) {
	string fullPath;
	// 저장할 이미지를 받아옴
	Mat img = objectCutting(object, frame, frame.rows, frame.cols);

	// 저장 될 경로를 따라 파일 이름을 생성
	stringstream ss;
	ss << video_path << "/" << object.fileName << ".jpg";
	fullPath = ss.str();
	ss.str("");

	// 저장하면서 빨간 사각형을 그리는 함수
	rectangle(img, Point(0, 0),
		Point(object.right - object.left - 1, object.bottom - object.top - 1),
		Scalar(0, 0, 255), 2);

	// 해당 이미지를 얻어온 파일이름을 통해 jpg파일로 저장
	int check_writeFullPath = imwrite(fullPath, img);
}

// Segmentation된 Obj의 Data를 txt파일로 저장하는 함수
// format : FILE_NAME(sec_frameCount) X Y WIDTH HEIGHT
void saveSegmentation_TXT(component object, FILE *fp) {
	string info;
	stringstream ss;
	ss << object.fileName << " " << object.left << " " << object.top << " " << object.right << " " << object.bottom
		<< " " << object.right - object.left << " " << object.bottom - object.top << '\n';
	info = ss.str();
	fprintf(fp, info.c_str());

	return;
}

// Segmentation된 Obj의 Data의 방향정보를 txt파일로 저장하는 함수
// format : FILE_NAME first(입장) last(퇴장)
void saveSegmentation_TXT_detail(component object, FILE *fp, int ROWS, int COLS) {
	string info;
	stringstream ss;
	ss << object.timeTag << " " << directionChecker(object, ROWS, COLS) << " 10\n";
	info = ss.str();
	fprintf(fp, info.c_str());

	return;
}

int directionChecker(component object, int ROWS, int COLS){
	int result = 10;
	int padding = 10;	//가장자리라고 허용할 위치 오차 픽셀값
	//좌
	if (object.left<padding){
		result += 10;
	}
	//우
	else if (object.right>COLS - padding - 1){
		result += 5;
	}
	else{	//가운데

	}

	//상
	if (object.top<padding){
		result += 3;
	}
	//하
	else if (object.bottom>ROWS - padding - 1){
		result += 9;
	}
	else{	//가운데

	}

	return result;
}

// jpg로 저장된 object를 반환해 주는 함수
Mat loadJPGObjectFile(segment obj, string file_name) {
	stringstream ss;
	String fullPath;

	// segment_파일이름 폴더 내에 파일 객체 이름 포맷으로 stringstream에 저장함
	ss << getObjDirectoryPath(file_name) << "/" << obj.fileName.c_str();
	fullPath = ss.str();
	ss.str("");
	Mat frame = imread(fullPath);
	return frame;
}

// ROI영역만 추출하는 함수
Mat objectCutting(component object, Mat img, unsigned int ROWS, unsigned int COLS) {
	return img(Rect(object.left, object.top, object.right - object.left, object.bottom - object.top)).clone();
	//잘린 이미지 반환
}

// 텍스트 파일(세그먼트 정보가 저장된) 이름을 반환하는 함수
string getTextFilePath(string video_name) {
	return SEGMENTATION_DATA_DIRECTORY_NAME + "/" + video_name
		+ "/" + RESULT_TEXT_FILENAME + video_name + (".txt");
}

// 텍스트 파일(위치 정보가 저장되는) 이름을 반환하는 함수
string getDetailTextFilePath(string video_name) {
	return SEGMENTATION_DATA_DIRECTORY_NAME + "/" + video_name
		+ "/" + RESULT_TEXT_DETAIL_FILENAME + video_name + (".txt");
}

// 배경 파일 이름을 반환하는 함수
string getBackgroundFilePath(string video_name) {
	return SEGMENTATION_DATA_DIRECTORY_NAME + "/" + video_name
		+ "/" + RESULT_BACKGROUND_FILENAME + video_name + (".jpg");
}

// 세그먼트들이 통합적인 정보들이 저장된 폴더이름을 반환하는 함수 , 폴더 및 경로 :: /data/(비디오 이름)
string getDirectoryPath(string video_name) {
	return SEGMENTATION_DATA_DIRECTORY_NAME + "/" + video_name;
}

// 세그먼트(object)들이 저장된 폴더이름을 반환하는 함수 , 폴더 및 경로 :: /data/(비디오 이름)/obj
string getObjDirectoryPath(string video_name) {
	return SEGMENTATION_DATA_DIRECTORY_NAME + "/" + video_name + "/" + "obj";
}

// 프로젝트 내에 해당 디렉토리가 있는 지 체크하는 함수
bool isDirectory(string dir_name) {
	// string type을 const char* 로 바꾸는 연산
	std::vector<char> writable(dir_name.begin(), dir_name.end());
	writable.push_back('\0');
	char *ptr_name = &writable[0];

	// 폴더가 있을 경우에는 _access(ptr, 0) 값을 0을 반환하여 true, 그렇지 않으면 false.
	return _access(ptr_name, 0) == 0;
}

// data 폴더를 생성하는 함수
int makeDataRootDirectory() {
	// 세그먼테이션 결과 데이터가 들어있는 폴더가 없는 경우 만들어 줌
	return _mkdir(SEGMENTATION_DATA_DIRECTORY_NAME.c_str());
}

// data 폴더 안에 해당 비디오의 경로 및 이름을 갖는 디렉토리를 생성하는 함수
int makeDataSubDirectory(string video_path) {
	// 해당 비디오의 세그먼테이션 결과가 들어갈 폴더경로에 하위 폴더 생성(/data/(video_name))
	return _mkdir(video_path.c_str());
}

// 파일의 이름부분을 저장
string allocatingComponentFilename(int timeTag, int currentMsec, int frameCount, int indexOfhumanDetectedVector) {
	string name;
	return name.append(to_string(timeTag)).append("_")
		.append(to_string(currentMsec)).append("_")
		.append(to_string(frameCount)).append("_")
		.append(to_string(indexOfhumanDetectedVector));
}