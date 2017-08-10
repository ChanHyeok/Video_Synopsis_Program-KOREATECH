#pragma once
#include "afxcmn.h"
#include <string>
#include <opencv2/opencv.hpp>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "afxwin.h"

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
	int ROWS, COLS;
	int FRAMES_FOR_MAKE_BACKGROUND;
	Mat frame; // Mat(height, width, channel)
	Mat bg;
	Mat bg_gray;
	int totalFrame, count;
	std::string fileNameNoExtension;
	BOOL isBackgroundSaved;
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
