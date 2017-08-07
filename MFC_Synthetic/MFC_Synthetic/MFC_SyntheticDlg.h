
// MFC_SyntheticDlg.h : header file
//

#pragma once
#include <string>
#include <opencv2/opencv.hpp>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include <iostream>
#include "afxcmn.h"
#include "afxwin.h"
#include <time.h>
using namespace std;
using namespace cv;


#define BUFFER 8096 // 객체 프레임 데이터를 저장할 버퍼의 크기 

// fileName 상수 관련
#define RESULT_TEXT_FILENAME  "obj_data_"
#define RESULT_TEXT_DETAIL_FILENAME  "obj_detail_"
#define RESULT_BACKGROUND_FILENAME "background_"
const string SEGMENTATION_DATA_DIRECTORY_NAME = "data";

// component vector Queue 관련
const int MAXSIZE_OF_COMPONENT_VECTOR_QUEUE = 8;
#define NEXT(index) ((index+1)%MAXSIZE_OF_COMPONENT_VECTOR_QUEUE)

// segment 임시 버퍼 관련
const int MAX_SEGMENT_TEMP_BUFFER = MAXSIZE_OF_COMPONENT_VECTOR_QUEUE;

// segmentation structure
typedef struct _segment {
	string fileName;
	unsigned int timeTag;
	unsigned int msec;
	unsigned int frameCount;
	unsigned int index;
	int left;
	int top;
	int right;
	int bottom;
	int width;
	int height;
	bool first_timeTagFlag; // 타임태그에 첫번 째 객체임을 판별하는 변수
	_segment() {
		fileName = "";
		timeTag = 0;
		msec = 0;
		frameCount = 0;
		index = 0;
		left = 0;
		top = 0;
		right = 0;
		bottom = 0;
		width = 0;
		height = 0;
		first_timeTagFlag = false;
	}
}segment;
//component model
typedef struct _component {
	string fileName;
	unsigned int timeTag;
	unsigned int label;
	unsigned int size;
	float centerOfX;
	float centerOfY;
	unsigned int left;
	unsigned int right;
	unsigned int top;
	unsigned int bottom;
	int width;
	int height;
	int area;
	_component() {
		fileName = "";
		timeTag = 0;
		label = 0;
		size = 0;
		centerOfX = 0.0;
		centerOfY = 0.0;
		left = 0;
		right = 0;
		top = 0;
		bottom = 0;
		width = 0;
		height = 0;
		area = 0;
	}
}component;

// segment를 위한 큐
typedef struct node //노드 정의
{
	segment segment_data;
	int indexOfSegmentArray;
	struct node *next;
}node;

// segmentQueue
typedef struct Queue //Queue 구조체 정의
{
	node *front; //맨 앞(꺼낼 위치)
	node *rear; //맨 뒤(보관할 위치)
	int count;//보관 개수
}Queue;
void InitQueue(Queue *);
int IsEmpty(Queue *);
void Enqueue(Queue *, segment, int);
segment Dequeue(Queue *);
int Getqueue_IndexOfSegmentArray(Queue *queue);

// componentVector 타입이 그대로 저장되는 '원형' Queue
typedef struct ComponentVectorQueue // Component Vector을 위한 크기 5인 원형 Queue 구조체 정의
{
	vector<component> buf[MAXSIZE_OF_COMPONENT_VECTOR_QUEUE]; // 배열 요소요소를 담당하는 벡터
	int front; // 앞쪽 (다음 데이터가 나갈 위치)
	int rear; // 뒤쪽 (다음 데이터가 들어올 위치)
}_ComponentVectorQueue;

void InitComponentVectorQueue(ComponentVectorQueue *componentVectorQueue);
bool IsComponentVectorQueueEmpty(ComponentVectorQueue *componentVectorQueue);
bool IsComponentVectorQueueFull(ComponentVectorQueue *componentVectorQueue);
void PutComponentVectorQueue(ComponentVectorQueue *componentVectorQueue, vector<component> componentVector);
void RemoveComponentVectorQueue(ComponentVectorQueue *componentVectorQueue);
vector<component> GetComponentVectorQueue(ComponentVectorQueue *componentVectorQueue, int point);

// MAIN ****
vector<component> humanDetectedProcess2(vector<component> humanDetectedVector, vector<component> prevHumanDetectedVector_Array
	, ComponentVectorQueue prevHumanDetectedVector_Queue, Mat frame, int frameCount, int videoStartMsec, unsigned int currentMsec, FILE *fp, vector<pair<int, int>>*, int*);

// addition function of MAIN
bool segmentationTimeInputException(CString str_h, CString str_m);
bool IsComparePrevDetection(vector<component> curr_detected, vector<component> prev_detected, int curr_index, int prev_index);
bool IsSaveComponent(component curr_component, component prev_component);
Mat morphologicalOperation(Mat);
stringstream timeConvertor(int t);

bool IsObjectOverlapingDetector(segment, segment);
Mat backgroundInit(VideoCapture *vc_Source);

int readSegmentTxtFile(segment* );

// connectecComponentLabelling.cpp
vector<component> connectedComponentsLabelling(Mat frame, int rows, int cols, int, int, int, int);
bool labelSizeFiltering(int width, int height, int, int, int, int);
bool IsEnqueueFiltering(segment *segment_array, int cur_index);

// tool_background.cpp, tool_foreground.cpp
Mat ExtractForegroundToMOG2(Mat frameimg);
Mat ExtractFg(Mat, Mat, int, int);
int temporalMedianBG(Mat frameimg, Mat bgimg, int rows, int cols);

// FileProcessing.cpp
String getFileName(CString f_path, char find_char, BOOL);
Mat loadJPGObjectFile(segment obj, string file_name);
bool saveSegmentationData(string video_name, component object, Mat object_frame
	, int currentMsec, int frameCount, FILE *txt_fp, FILE*, int, int, vector<pair<int, int>>*, int*);

string getTextFilePath(string video_name);
string getDetailTextFilePath(string video_name);
string getBackgroundFilePath(string video_name);
string getDirectoryPath(string video_name);
string getObjDirectoryPath(string video_name);

bool isDirectory(string dir_name);
int makeDataRootDirectory();
int makeDataSubDirectory(string video_name);

// tool_synthetic.cpp
Mat Syn_Background_Foreground(Mat, Mat, Mat, int, int);
Mat printObjOnBG(Mat background, segment obj, int* labelMap, string);

// CMFC_SyntheticDlg dialog
class CMFC_SyntheticDlg : public CDialogEx{
// Construction
public:
	CMFC_SyntheticDlg(CWnd* pParent = NULL);	// standard constructor
	//~CMFC_SyntheticDlg();
// Dialog Data
	enum { IDD = IDD_MFC_SYNTHETIC_DIALOG };

	Mat mat_frame;
	CImage *cimage_mfc;
	CStatic m_picture;

	VideoCapture capture, capture_for_background;

	boolean isPlayBtnClicked;
	CRect m_rectCurHist;
	CEdit *m_pEditBoxStartHour;
	CEdit *m_pEditBoxStartMinute;

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

public:
	void DisplayImage(int IDC_PICTURE_TARGET, Mat targetMat, int);
	//afx_msg void OnTimer(UINT_PTR nIDEvent);


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CImage cImage;
	// afx_msg void OnClose();
	afx_msg void OnCancel();
	afx_msg void OnDestroy();

	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedBtnSegmentation();
	afx_msg void OnBnClickedGroup1Seg();
	CSliderCtrl m_sliderSearchStartTime;
	CSliderCtrl m_sliderSearchEndTime;
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnClickedBtnPlay();
	CSliderCtrl m_sliderFps;
	int mRadioPlay;
	afx_msg void OnBnClickedBtnMenuLoad();
	
	afx_msg void loadFile();

	afx_msg void SetRadioStatus(UINT value);
	afx_msg void OnBnClickedBtnPause();

	afx_msg bool checkSegmentation();

	// slider : range of detecting object
	CSliderCtrl m_SliderWMIN;
	CSliderCtrl m_SliderWMAX;
	CSliderCtrl m_SliderHMIN;
	CSliderCtrl m_SliderHMAX;
	afx_msg void OnBnClickedBtnStop();
	afx_msg void layoutInit(); 
	afx_msg void setSliderRange(int,int,int,int);
	afx_msg void updateUI(int, int, int, int);
	afx_msg void segmentationOperator(VideoCapture* vc_Source, int, int, int, int, int, int);
	afx_msg void OnBnClickedBtnRewind();
	CProgressCtrl m_LoadingProgressCtrl;

	CSliderCtrl m_SliderPlayer;
	afx_msg void OnReleasedcaptureSliderPlayer(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg Mat getSyntheticFrame(Mat);
	//콤보박스
	CComboBox mComboStart;
	CComboBox mComboEnd;
	afx_msg bool isDirectionMatch(int);
	
	CButton mButtonSynSave;
	afx_msg void OnBnClickedBtnSynSave();
	afx_msg bool inputSegmentQueue(int obj1_TimeTag, int obj2_TimeTag, int segmentCount, segment*);

};

