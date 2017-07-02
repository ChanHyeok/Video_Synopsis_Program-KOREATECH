#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "MFC_SyntheticDlg.h"
#include "afxdialogex.h"
#include <opencv2\opencv.hpp>
#include <opencv\highgui.h>
#include <opencv\cv.h>
#include <io.h>

// 파일 처리와 관련된 모든 함수들을 선언합니다.
// JPG 파일 저장, txt파일 저장

// segment 폴더 안에 Segmentation된 Obj만을 jpg파일로 저장하는 함수
Mat objectCutting(component object, Mat img, unsigned int ROWS, unsigned int COLS);

// Video Path에서 file이름만 빼서 반환하는 함수 
String getFileName(CString f_path, char find_char) {
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
		char c = f_path[i];
		f_name += c;
	}// 마지막에 나오는 '\' 이후의 문자열을 추출함

	return f_name;
}

void saveSegmentation_JPG(component object,
	Mat frame, int frameCount, int msec, int index, unsigned int videoStartMsec, string video_fname) {
	Mat img = objectCutting(object, frame, frame.rows, frame.cols);

	std::string folderName = RESULT_FOLDER_NAME;
	string fileName = object.fileName;
	string fullPath;
	stringstream ss;

	// folderName :: segment_연산할 비디오 파일 이름(ex. segment_xxx.mp4)
	folderName.append(video_fname);

	//빨간 사각형을 그리는 함수
	rectangle(img, Point(0, 0),
		Point(object.right - object.left - 1, object.bottom - object.top - 1),
		Scalar(0, 0, 255), 2);

	if (!isDirectory(folderName, fileName)) {	//폴더가 없을 경우
		string folderCreateCommand = "mkdir " + folderName;
		system(folderCreateCommand.c_str());
	}

	// 폴더에 jpg파일을 저장함
	ss << folderName << "/" << fileName << ".jpg";
	fullPath = ss.str();
	ss.str("");

	int check_writeFullPath = imwrite(fullPath, img);
}
// Segmentation된 Obj의 Data를 txt파일로 저장하는 함수
// format : FILE_NAME(sec_frameCount) X Y WIDTH HEIGHT 
void saveSegmentation_TXT(component object, int frameCount, int msec, FILE *fp, int index) {
	string info;
	stringstream ss;
	ss << object.fileName << " " << object.left << " " << object.top << " " << object.right << " " << object.bottom 
		<< " " << object.right - object.left << " " << object.bottom - object.top << '\n';
	info = ss.str();
	fprintf(fp, info.c_str());
}

// jpg로 저장된 object를 반환해 주는 함수
Mat loadJPGObjectFile(segment obj, string file_name) {
	stringstream ss;
	String fullPath;

	// segment_파일이름 폴더 내에 파일 객체 이름 포맷으로 stringstream에 저장함 
	ss << RESULT_FOLDER_NAME << file_name << "/" << obj.fileName.c_str();
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
string getTextFileName(string video_name) {
	return RESULT_TEXT_FILENAME + video_name + (".txt");
}

// 배경 파일 이름을 반환하는 함수
string getBackgroundFilename(string video_name) {
	return RESULT_BACKGROUND_FILENAME + video_name + (".jpg");
}

// 세그먼트들이 저장된 폴더이름을 반환하는 함수
string getDirectoryName(string video_name) {
	return RESULT_FOLDER_NAME + video_name;
}

// 프로젝트 내에 해당 디렉토리가 있는 지 체크하는 함수
bool isDirectory(string dir_name, string video_name) {
	// string type인 folderName을 const char* 로 바꾸어 access 함수에 넣는 연산
	std::vector<char> writable(dir_name.begin(), dir_name.end());
	writable.push_back('\0');
	char* ptr = &writable[0];

	return _access(ptr, 0) == 0;
	// 폴더가 있을 경우에는 _access(ptr, 0) 값을 0을 반환하여 true, 그렇지 않으면 false.
}