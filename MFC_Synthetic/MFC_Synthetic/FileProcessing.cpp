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
// void saveSegmentation_JPG(component object, Mat frame, string video_path);
void saveSegmentation_TXT(component object, FILE *fp);
int directionChecker(component object, int ROWS, int COLS);
//string allocatingComponentFilename(int timeTag, int currentMsec, int frameCount, int label_num);

// segment 폴더 안에 Segmentation된 Obj만을 jpg파일로 저장하는 함수
Mat objectCutting(component object, Mat img, unsigned int ROWS, unsigned int COLS);

string readTxt(string path){
	string result;
	ifstream in(path);
	if (in.is_open()){
		in.seekg(0, ios::end);
		int size = in.tellg();
		result.resize(size);
		in.seekg(0, ios::beg);
		in.read(&result[0], size);
		in.close();
		return result;
	}
	else{
		cout << path << "를 읽어올 수 없습니다" << endl;
		return NULL;
	}
}

boolean rewriteTxt(string path, string text){
	ofstream out(path,ios::trunc);
	if (out.is_open()){
		out << text;
		out.close();
		return true;
	}
	else{
		cout << path << "를 읽어올 수 없습니다" << endl;
		return false;
	}
}

boolean appendTxt(string path, string text){
	ofstream out(path,ios::app);
	if (out.is_open()){
		out << text;
		out.close();
		return true;
	}
	else{
		cout << path << "를 읽어올 수 없습니다" << endl;
		return false;
	}
}

string lineMaker_detail(int timetag, int label, int first, int last, int c1, int c2, int c3, int c4, int c5, int c6, int c7, int c8,int c9){
	return to_string(timetag).append(" ").append(to_string(label)).append(" ").append(to_string(first)).append(" ").append(to_string(last)).append(" ").append(to_string(c1)).append(" ").append(to_string(c2)).append(" ").append(to_string(c3)).append(" ")
		.append(to_string(c4)).append(" ").append(to_string(c5)).append(" ").append(to_string(c6)).append(" ").append(to_string(c7)).append(" ").append(to_string(c8)).append(" ").append(to_string(c9)).append("\n");
}

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

bool saveSegmentationData(string fileNameNoExtension, component object, Mat object_frame
	, int currentMsec, int frameCount, FILE *txt_fp, FILE * txt_fp_detail, int ROWS, int COLS
	, vector<pair<int, int>>* vectorDetailTXTInedx, int* detailTxtIndex, int *colorArray) {
	// object의 기본 정보 저장
	// object의 파일이름 할당
	object.fileName = allocatingComponentFilename(object.timeTag, currentMsec, frameCount, object.label);

	// jpg파일로 저장
	saveSegmentation_JPG(object, object_frame, getObjDirectoryPath(fileNameNoExtension));

	// txt파일로 저장
	saveSegmentation_TXT(object, txt_fp);

	// 방향 및 색깔 정보 텍스트 파일 저장
	if (object.timeTag == currentMsec){//현재 오브젝트가 객체의 처음 일 경우
		appendTxt(getDetailTextFilePath(fileNameNoExtension).c_str(),
			lineMaker_detail(object.timeTag, object.label, directionChecker(object, ROWS, COLS), 10
				, 0, 0, 0, 0, 0, 0, 0, 0, 0));
	}
	else 	//첫 오브젝트가 아닐 경우 해당 객체 위치로 이동하여 last 위치 덮어쓰기
		saveColorData(fileNameNoExtension, object, colorArray);

	return true;
}

void saveColorData(string fileNameNoExtension, component object, int *colorArray){
	//색상 정보 텍스트 파일 저장
	//해당 객체 위치로 이동하여 Color 카운트 덮어쓰기
	int stamp;
	int label;
	int tempFirst;
	int tempLast;
	int colors[COLORS] = { 0, };

	string txt = readTxt(getDetailTextFilePath(fileNameNoExtension).c_str());

	size_t posOfTimetag = txt.find(to_string(object.timeTag).append(" ").append(to_string(object.label)));
	if (posOfTimetag != string::npos){
		int posOfNL = txt.find("\n", posOfTimetag);

		string capture = txt.substr(posOfTimetag, posOfNL - posOfTimetag);
		char *line = new char[capture.length() + 1];
		strcpy(line, capture.c_str());

		char *ptr = strtok(line, " ");
		if (ptr != NULL){
			stamp = atoi(ptr);
			ptr = strtok(NULL, " ");
			if (ptr != NULL){
				label = atoi(ptr);
			}
			ptr = strtok(NULL, " ");
			if (ptr != NULL)
				tempFirst = atoi(ptr);
			ptr = strtok(NULL, " ");
			if (ptr != NULL)
				tempLast = atoi(ptr);

			for (int i = 0; i < COLORS; i++){
				ptr = strtok(NULL, " ");
				if (ptr != NULL)
					colors[i] = atoi(ptr);
			}

			txt.erase(posOfTimetag, posOfNL - posOfTimetag + 1);
			txt.insert(posOfTimetag, lineMaker_detail(stamp, label, tempFirst, tempLast, colors[0] + colorArray[0], colors[1] + colorArray[1], colors[2] + colorArray[2], colors[3] + colorArray[3], colors[4] + colorArray[4],
				colors[5] + colorArray[5], colors[6] + colorArray[6], colors[7] + colorArray[7], colors[8] + colorArray[8]));
			rewriteTxt(getDetailTextFilePath(fileNameNoExtension).c_str(), txt.c_str());
		}
		ptr = NULL;
		delete ptr;
		delete[] line;
	}
	return;
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
		Point(object.width, object.height),
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
	fflush(stdout);
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
	return img(Rect(object.left, object.top, object.width, object.height)).clone();
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

// 기본 배경 파일 이름을 반환하는 함수
string getBackgroundFilePath(string video_name) {
	return SEGMENTATION_DATA_DIRECTORY_NAME + "/" + video_name
		+ "/" + RESULT_BACKGROUND_FILENAME + video_name + (".jpg");
}

// 연산에 사용할 배경 파일 이름을 반환하는 함수
string getTempBackgroundFilePath(string video_name) {
	return SEGMENTATION_DATA_DIRECTORY_NAME + "/" + video_name
		+ "/" + RESULT_BACKGROUND_FILENAME + "_T_" + video_name + (".jpg");
}

// 합성영상 출력시 사용할 컬러 배경 파일 이름을 반환하는 함수
string getColorBackgroundFilePath(string video_name) {
	return SEGMENTATION_DATA_DIRECTORY_NAME + "/" + video_name
		+ "/" + RESULT_BACKGROUND_FILENAME+ "_C_" + video_name + (".jpg");
}

// 세그먼트들이 통합적인 정보들이 저장된 폴더이름을 반환하는 함수 , 폴더 및 경로 :: /data/(비디오 이름)
string getDirectoryPath(string video_name) {
	return SEGMENTATION_DATA_DIRECTORY_NAME + "/" + video_name;
}

// 세그먼트(object)들이 저장된 폴더이름을 반환하는 함수 , 폴더 및 경로 :: /data/(비디오 이름)/obj
string getObjDirectoryPath(string video_name) {
	return SEGMENTATION_DATA_DIRECTORY_NAME + "/" + video_name + "/" + "obj";
}

// 세그먼트(object)들이 저장된 폴더이름을 반환하는 함수 , 폴더 및 경로 :: /data/(비디오 이름)/obj
string getObj_for_colorDirectoryPath(string video_name) {
	return SEGMENTATION_DATA_DIRECTORY_NAME + "/" + video_name + "/" + "obj_for_color";
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

//파일 로드시 만드는 회색 배경이 있는 지 확인하는 함수
bool isGrayBackgroundExists(string name) {
	if (FILE *file = fopen(name.c_str(), "r")) {
		fclose(file);
		return true;
	}
	else {
		return false;
	}
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
string allocatingComponentFilename(int timeTag, int currentMsec, int frameCount, int label_num) {
	string name;
	return name.append(to_string(timeTag)).append("_")
		.append(to_string(currentMsec)).append("_")
		.append(to_string(frameCount)).append("_")
		.append(to_string(label_num));
}