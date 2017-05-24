
// MFC_SyntheticDlg.h : header file
//

#pragma once

#include <opencv2/opencv.hpp>
#include "afxcmn.h"
using namespace std;
using namespace cv;


#define BUFFER 8096 // 객체 프레임 데이터를 저장할 버퍼의 크기 

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
	}
}segment;
//component model
typedef struct _component {
	string fileName;
	unsigned int timeTag;
	unsigned int label;
	unsigned int sumOfX;
	unsigned int sumOfY;
	unsigned int size;
	float centerOfX;
	float centerOfY;
	unsigned int firstPixel;
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
		sumOfX = 0;
		sumOfY = 0;
		size = 0;
		centerOfX = 0.0;
		centerOfY = 0.0;
		firstPixel = 0;
		left = 0;
		right = 0;
		top = 0;
		bottom = 0;
		width = 0;
		height = 0;
		area = 0;
	}
}component;

// 큐
typedef struct node //노드 정의
{
	unsigned int timeTag;
	int indexOfSegmentArray;
	struct node *next;
}node;
typedef struct Queue //Queue 구조체 정의
{
	node *front; //맨 앞(꺼낼 위치)
	node *rear; //맨 뒤(보관할 위치)
	int count;//보관 개수
}Queue;
void InitQueue(Queue *);
int IsEmpty(Queue *);
void Enqueue(Queue *, int, int);
node Dequeue(Queue *);

Mat ExtractFg(Mat, Mat, int, int);
Mat morphologicalOperation(Mat );
vector<component> connectedComponentsLabelling(Mat frame, int rows, int cols);
vector<component> humanDetectedProcess(vector<component> humanDetectedVector, vector<component> prevHumanDetectedVector, Mat, int, int, unsigned int, FILE *fp);
void humonDetector(VideoCapture* vc_Source, int, int);
Mat getSyntheticFrame(Mat);
// file_io.cpp
void saveSegmentation_JPG(component, Mat, int, int, int, unsigned int);	//캡쳐한 Components를 jpg파일로 저장하는 함수
void saveSegmentation_TXT(component, int, int, FILE *, int);	//components의 Data를 txt로 저장하는 함수
stringstream timeConvertor(int t);


// tool_synthetic.cpp
Mat Syn_Background_Foreground(Mat, Mat, Mat, int, int);
Mat printObjOnBG(Mat, segment, int*);

// CMFC_SyntheticDlg dialog
class CMFC_SyntheticDlg : public CDialogEx{
// Construction
public:
	CMFC_SyntheticDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_MFC_SYNTHETIC_DIALOG };

	VideoCapture capture;
	Mat mat_frame;
	CImage *cimage_mfc;
	CStatic m_picture;

	boolean isPlayBtnClicked;
	CRect m_rectCurHist;
	CEdit *m_pEditBoxStartHour;
	CEdit *m_pEditBoxStartMinute;


	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

public:
	void DisplayImage(int IDC_PICTURE_TARGET, Mat targetMat, int);
	//afx_msg void OnDestroy();
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
	afx_msg void OnBnClickedOk();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedBtnSegmentation();
	afx_msg void OnBnClickedGroup1Seg();
	afx_msg void OnEnChangeEdit1();
	CSliderCtrl m_sliderSearchStartTime;
	CSliderCtrl m_sliderSearchEndTime;
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnClickedBtnSynPlay();
	CSliderCtrl m_sliderFps;
};

