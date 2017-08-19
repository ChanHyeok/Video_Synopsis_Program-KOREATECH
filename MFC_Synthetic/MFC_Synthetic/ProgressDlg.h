#pragma once
#include "afxcmn.h"
#include <string>
#include <opencv2/opencv.hpp>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "afxwin.h"
#include "MFC_SyntheticDlg.h"

// CProgressDlg dialog
using namespace std;
using namespace cv;

class CProgressDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CProgressDlg)

private :
	CWnd* m_pParentWnd;//부모
public:
	CProgressDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CProgressDlg();
	//멤버변수
	VideoCapture vc_Source;
	string videoFilePath;
	int mode;
	int ROWS, COLS;
	int WMIN, WMAX, HMIN, HMAX;
	int FRAMES_FOR_MAKE_BACKGROUND, FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND;
	Mat frame, frame_g; // Mat(height, width, channel)
	Mat bg;
	Mat bg_gray;
	int totalFrame, count;
	int videoStartMsec;
	std::string fileNameNoExtension;
	BOOL isWorkCompleted;

	FILE *fp;
	vector<component> humanDetectedVector, prevHumanDetectedVector;
	ComponentVectorQueue prevHumanDetectedVector_queue;


	VideoWriter outputVideo;
	Queue segment_queue;
	segment *segmentArray;
	unsigned int obj1_TimeTag;
	unsigned int obj2_TimeTag;
	// Dialog Data
	enum { IDD = IDD_MFC_PROGRESS_DIALOG};

	

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CProgressCtrl m_ProgressCtrl;
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedMyok();
	afx_msg void OnBnClickedMycancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CStatic m_StaticMessage;
	CButton m_ButtonOK;
};
