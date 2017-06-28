// MFC_SyntheticDlg.cpp : implementation file

// To Do :: 이미 segmentation을 진행하고 다시 실행하였을 때 
// txt, jpg파일(세그먼트들)이 있는데도 play를 누를 시 팅기는 버그 

// To Do :: 또한 segmentation 누르고 Play 진행하고 또 segmentation 누르면 Done이라고 뜨는데
// 이후 Play하면 segment 파일이 없다고 뜨는 점
#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "MFC_SyntheticDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/*** 각종 상수들 ***/
#define VIDEO_TIMER 1
#define SYN_RESULT_TIMER 2
#define MAX_STR_BUFFER_SIZE  128 // 문자열 출력에 쓸 버퍼 길이
const int FRAMECOUNT_FOR_MAKE_BACKGROUND = 100; // 배경을 만들기 까지 필요한 프레임카운트

/***  전역변수  ***/
char txtBuffer[100] = { 0, };	//텍스트파일 읽을 때 사용할 buffer
segment *m_segmentArray;
Queue segment_queue; // C++ STL의 queue 키워드와 겹치기 때문에 변수를 조정함
int videoStartMsec, segmentCount, fps;
// 시작 millisecond, 세그먼트 카운팅변수, 초당 프레임수
//은혜
int status;
bool timer = false;
double timer_fps = 0;

// background 전역변수 삭제
// synthesis 중에 배경관련 정보들이 사라지는 버그가 발생하여 수정하면서 m_background를 삭제함

// File 관련
FILE *fp; // frameInfo를 작성할 File Pointer
std::string video_filename(""); // 입력받은 비디오파일 이름
std::string background_filename = RESULT_BACKGROUND_FILENAME; // 배경 파일 이름
std::string txt_filename = RESULT_TEXT_FILENAME; // txt 파일 이름
/////			/////

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFC_SyntheticDlg dialog



CMFC_SyntheticDlg::CMFC_SyntheticDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMFC_SyntheticDlg::IDD, pParent)
	, mRadioPlay(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFC_SyntheticDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SYN_SLIDER_START_TIME, m_sliderSearchStartTime);
	DDX_Control(pDX, IDC_SYN_SLIDER_END_TIME, m_sliderSearchEndTime);
	DDX_Control(pDX, IDC_SYN_SLIDER_FPS, m_sliderFps);
	DDX_Radio(pDX, IDC_RADIO_PLAY1, mRadioPlay);
}

BEGIN_MESSAGE_MAP(CMFC_SyntheticDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CMFC_SyntheticDlg::OnBnClickedOk)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_SEGMENTATION, &CMFC_SyntheticDlg::OnBnClickedBtnSegmentation)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_BTN_SYN_PLAY, &CMFC_SyntheticDlg::OnClickedBtnSynPlay)
	ON_BN_CLICKED(IDC_BTN_MENU_LOAD, &CMFC_SyntheticDlg::OnBnClickedBtnMenuLoad)
	ON_BN_CLICKED(IDC_BTN_PLAY, &CMFC_SyntheticDlg::OnBnClickedBtnPlay)
	ON_BN_CLICKED(IDC_BTN_PAUSE, &CMFC_SyntheticDlg::OnBnClickedBtnPause)
	ON_BN_CLICKED(IDC_BTN_STOP, &CMFC_SyntheticDlg::OnBnClickedBtnStop)
END_MESSAGE_MAP()


// CMFC_SyntheticDlg message handlers

BOOL CMFC_SyntheticDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	ShowWindow(SW_SHOWMAXIMIZED);	//전체화면
	this->GetWindowRect(m_rectCurHist);	//다이얼로그 크기를 얻어옴

	 ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	//*********************************************Layout 세팅********************************************************
	//(http://gandus.tistory.com/530)

	CWnd *pResultImage = GetDlgItem(IDC_RESULT_IMAGE);

	//Dialog
	//현재 dialog 크기 얻어옴
	int dialogWidth = m_rectCurHist.right;
	int dialogHeight = m_rectCurHist.bottom-50;//작업표시줄 크기 빼줌
	int padding = 10;
	SetWindowPos(&wndTop, 0, 0, dialogWidth, dialogHeight, SWP_NOMOVE);//다이얼로그 크기 조정

	//group box - MENU
	CWnd *pGroupMenu = GetDlgItem(IDC_GROUP_MENU);
	CWnd *pStringFileName = GetDlgItem(IDC_MENU_STRING_FILE_NAME);
	CButton *pButtonLoad = (CButton *)GetDlgItem(IDC_BTN_MENU_LOAD);
	CWnd *pRadioBtn1 = GetDlgItem(IDC_RADIO_PLAY1);
	CWnd *pRadioBtn2 = GetDlgItem(IDC_RADIO_PLAY2);
	int box_MenuX = padding;
	int box_MenuY = padding;
	int box_MenuWidth = (dialogWidth-3*padding)*0.2;
	int box_MenuHeight = ((dialogHeight-3*padding)*0.7-padding)*0.3;

	pGroupMenu->MoveWindow(box_MenuX, box_MenuY, box_MenuWidth, box_MenuHeight, TRUE);
	pStringFileName->MoveWindow(box_MenuX + padding, box_MenuY + 2*padding, 230, 20, TRUE);
	pButtonLoad->MoveWindow(box_MenuX + box_MenuWidth - padding - 100, box_MenuY + 3*padding + 20, 100, 20, TRUE);
	pRadioBtn1->MoveWindow(box_MenuX + padding, box_MenuY + 4 * padding + 40, 100, 20, TRUE);
	pRadioBtn2->MoveWindow(box_MenuX + padding, box_MenuY + 5 * padding + 60, 100, 20, TRUE);

	//Picture Control
	CButton *pButtonPlay = (CButton *)GetDlgItem(IDC_BTN_PLAY);
	CButton *pButtonPause = (CButton *)GetDlgItem(IDC_BTN_PAUSE);
	int pictureContorlX = 2 * padding + box_MenuWidth;
	int pictureContorlY = padding;
	int pictureContorlWidth = (dialogWidth - 3 * padding) - box_MenuWidth-15;
	int pictureContorlHeight = (dialogHeight - 3 * padding)*0.7 - 40;
	pResultImage->MoveWindow(pictureContorlX, pictureContorlY, pictureContorlWidth, pictureContorlHeight, TRUE);
	pButtonPlay->MoveWindow(pictureContorlX + pictureContorlWidth*0.5 - padding - 100, pictureContorlY + pictureContorlHeight+10, 100, 20, TRUE);
	pButtonPause->MoveWindow(pictureContorlX + pictureContorlWidth*0.5 + padding, pictureContorlY + pictureContorlHeight + 10, 100, 20, TRUE);

	//group box - segmetation
	CWnd *pGroupSegmentation = GetDlgItem(IDC_GROUP_SEG);
	CWnd *pStringStartTime = GetDlgItem(IDC_SEG_STRING_VIDEO_START_TIME);
	CWnd *pStringColon = GetDlgItem(IDC_SEG_STRING_COLON);
	m_pEditBoxStartHour = (CEdit *)GetDlgItem(IDC_SEG_EDITBOX_START_HOUR);	
	m_pEditBoxStartMinute = (CEdit *)GetDlgItem(IDC_SEG_EDITBOX_START_MINUTE); 
	CWnd *pGroupSegWidth = GetDlgItem(IDC_GROUP_SEG_WIDTH);
	CWnd *pStringWMIN = GetDlgItem(IDC_SEG_STRING_MIN_W);
	CWnd *pStringWMAX = GetDlgItem(IDC_SEG_STRING_MAX_W);
	CWnd *pSegSliderWMIN = GetDlgItem(IDC_SEG_SLIDER_WMIN);
	CWnd *pSegSliderWMAX = GetDlgItem(IDC_SEG_SLIDER_WMAX);
	CWnd *pGroupSegHeight = GetDlgItem(IDC_GROUP_SEG_HEIGHT);
	CWnd *pStringHMIN = GetDlgItem(IDC_SEG_STRING_MIN_H);
	CWnd *pStringHMAX = GetDlgItem(IDC_SEG_STRING_MAX_H);
	CWnd *pSegSliderHMIN = GetDlgItem(IDC_SEG_SLIDER_HMIN);
	CWnd *pSegSliderHMAX = GetDlgItem(IDC_SEG_SLIDER_HMAX);
	CButton *pButtonSegmentation = (CButton *)GetDlgItem(IDC_BTN_SEG_SEGMENTATION);
	int box_segmentationX = padding;
	int box_segmentationY = 2 * padding + box_MenuHeight;
	int box_segmentationWidth = box_MenuWidth;
	int box_segmentationHeight = ((dialogHeight - 3 * padding)*0.7 - padding) - box_MenuHeight;
	pGroupSegmentation->MoveWindow(box_segmentationX, box_segmentationY, box_segmentationWidth, box_segmentationHeight, TRUE);
	pStringStartTime->MoveWindow(box_segmentationX + padding, box_segmentationY + 2*padding, 230, 20, TRUE);
	m_pEditBoxStartHour->MoveWindow(box_segmentationX + padding + box_segmentationWidth*0.5, box_segmentationY + 3 * padding + 20, 20, 20, TRUE);
	pStringColon->MoveWindow(box_segmentationX + padding + 25 + box_segmentationWidth*0.5, box_segmentationY + 3 * padding + 20, 20, 20, TRUE);
	m_pEditBoxStartMinute->MoveWindow(box_segmentationX + padding + 35 + box_segmentationWidth*0.5, box_segmentationY + 3 * padding + 20, 20, 20, TRUE);
	pGroupSegWidth->MoveWindow(box_segmentationX + padding, box_segmentationY + 4 * padding+40, box_segmentationWidth-2*padding, 80, TRUE);
	pStringWMIN->MoveWindow(box_segmentationX + 2 * padding, box_segmentationY + 6 * padding + 40, 40, 20, TRUE);
	pSegSliderWMIN->MoveWindow(box_segmentationX + 3 * padding + 40, box_segmentationY + 6 * padding + 40, box_segmentationWidth - 5 * padding-40 , 20, TRUE);
	pStringWMAX->MoveWindow(box_segmentationX + 2 * padding, box_segmentationY + 7 * padding + 60, 40, 20, TRUE);
	pSegSliderWMAX->MoveWindow(box_segmentationX + 3 * padding + 40, box_segmentationY + 7 * padding + 60, box_segmentationWidth - 5 * padding - 40, 20, TRUE);
	pGroupSegHeight->MoveWindow(box_segmentationX + padding, box_segmentationY + 5 * padding + 120, box_segmentationWidth - 2 * padding, 80, TRUE);
	pStringHMIN->MoveWindow(box_segmentationX + 2 * padding, box_segmentationY + 7 * padding + 120, 40, 20, TRUE);
	pSegSliderHMIN->MoveWindow(box_segmentationX + 3 * padding + 40, box_segmentationY + 7 * padding + 120, box_segmentationWidth - 5 * padding - 40, 20, TRUE);
	pStringHMAX->MoveWindow(box_segmentationX + 2 * padding, box_segmentationY + 8 * padding + 140, 40, 20, TRUE);
	pSegSliderHMAX->MoveWindow(box_segmentationX + 3 * padding + 40, box_segmentationY + 8 * padding + 140, box_segmentationWidth - 5 * padding - 40, 20, TRUE);
	pButtonSegmentation->MoveWindow(box_segmentationX + box_segmentationWidth - padding - 100, box_segmentationY + box_segmentationHeight - 30, 100, 20, TRUE);
	

	//group box - Play Settings
	CWnd *pGroupSynthetic = GetDlgItem(IDC_GROUP_PLAY_SETTINGS);
	CWnd *pStringSearchStartTime = GetDlgItem(IDC_STRING_SEARCH_START_TIME);
	CWnd *pStringSearchEndTime = GetDlgItem(IDC_STRING_SEARCH_END_TIME);
	CWnd *pStringFps = GetDlgItem(IDC_STRING_FPS);
	CWnd *pStringSearchStartTimeSlider = GetDlgItem(IDC_STRING_SEARCH_START_TIME_SLIDER);
	CWnd *pStringSearchEndTimeSlider = GetDlgItem(IDC_STRING_SEARCH_END_TIME_SLIDER);
	CWnd *pStringFpsSlider = GetDlgItem(IDC_STRING_FPS_SLIDER);

	int box_syntheticX = padding;
	int box_syntheticY = box_segmentationY + box_segmentationHeight+padding;
	int box_syntheticWidth = dialogWidth - 3 * padding;
	int box_syntheticHeight = (dialogHeight - 3 * padding)*0.3-40;
	pGroupSynthetic->MoveWindow(box_syntheticX, box_syntheticY, box_syntheticWidth, box_syntheticHeight, TRUE);
	pStringSearchStartTime->MoveWindow(box_syntheticX + padding, box_syntheticY + box_syntheticHeight*0.3, 100, 20, TRUE);
	m_sliderSearchStartTime.MoveWindow(box_syntheticX + padding, box_syntheticY + box_syntheticHeight*0.3+20+padding, 140, 20, TRUE);
	pStringSearchStartTimeSlider->MoveWindow(box_syntheticX + padding+40, box_syntheticY + box_syntheticHeight*0.3 + 40 + padding*2, 140, 20, TRUE);
	
	pStringSearchEndTime->MoveWindow(box_syntheticX + padding + 150, box_syntheticY + box_syntheticHeight*0.3, 100, 20, TRUE);
	m_sliderSearchEndTime.MoveWindow(box_syntheticX + padding + 150, box_syntheticY + box_syntheticHeight*0.3 + 20 + padding, 140, 20, TRUE);
	pStringSearchEndTimeSlider->MoveWindow(box_syntheticX + padding + 40 + 150, box_syntheticY + box_syntheticHeight*0.3 + 40 + padding * 2, 140, 20, TRUE);

	pStringFps->MoveWindow(box_syntheticX + padding + 300, box_syntheticY + box_syntheticHeight*0.3, 100, 20, TRUE);
	m_sliderFps.MoveWindow(box_syntheticX + padding+300, box_syntheticY + box_syntheticHeight*0.3 + 20 + padding, 140, 20, TRUE);
	pStringFpsSlider->MoveWindow(box_syntheticX + padding + 60 + 300, box_syntheticY + box_syntheticHeight*0.3 + 40 + padding * 2, 30, 20, TRUE);

	

	
	/*
	slider m_sliderSearchStartTime, m_sliderSearchEndTime, m_sliderFps 설정
	*/
	m_sliderSearchStartTime.SetRange(0, 500);
	m_sliderSearchEndTime.SetRange(0, 500);
	m_sliderFps.SetRange(0, 100);

	//***************************************************************************************************************

	

	//실행시 비디오 파일 불러옴
	loadFile();

	isPlayBtnClicked = false;

	//edit box default
	m_pEditBoxStartHour->SetWindowTextA("0");
	m_pEditBoxStartMinute->SetWindowTextA("0");
	//slider default
	SetDlgItemText(IDC_STRING_SEARCH_START_TIME_SLIDER, _T("00 : 00 : 00"));
	SetDlgItemText(IDC_STRING_SEARCH_END_TIME_SLIDER, _T("00 : 00 : 00"));
	SetDlgItemText(IDC_STRING_FPS_SLIDER, to_string(fps).c_str());
	m_sliderSearchStartTime.SetPos(0);
	m_sliderSearchEndTime.SetPos(0);
	m_sliderFps.SetPos(fps);

	SetTimer(VIDEO_TIMER, fps, NULL);

	return TRUE;  // return TRUE  unless you set the focus to a control
}
void CMFC_SyntheticDlg::loadFile(){
	//파일 다이얼로그 호출해서 segmentation 할 영상 선택	
	char szFilter[] = "Video (*.avi, *.MP4) | *.avi;*.mp4; | All Files(*.*)|*.*||";	//검색 옵션
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter, AfxGetMainWnd());	//파일 다이얼로그 생성
	dlg.DoModal();	//다이얼로그 띄움

	// Path를 받아와서 filename만을 떼어서 저장함(배경파일 이름을 결정할 때 사용)
	CString cstrImgPath = dlg.GetPathName();
	String temp = "File Name : ";
	video_filename = "";
	video_filename = getFileName(cstrImgPath, '\\');
	txt_filename = RESULT_TEXT_FILENAME;
	txt_filename.append(video_filename).append(".txt"); // frameinfo txt파일 재설정
	CWnd *pStringFileName = GetDlgItem(IDC_MENU_STRING_FILE_NAME);
	char *cstr = new char[temp.length() + 1];
	strcpy(cstr, temp.c_str());
	strcat(cstr, video_filename.c_str());

	pStringFileName->SetWindowTextA(cstr);
	capture.open((string)cstrImgPath);
	if (!capture.isOpened()) { //예외처리. 해당이름의 파일이 없는 경우
		perror("No Such File!\n");
		::SendMessage(GetSafeHwnd(), WM_CLOSE, NULL, NULL);	//다이얼 로그 종료
	}

	fps = capture.get(CV_CAP_PROP_FPS);


	isPlayBtnClicked = false;

	//edit box default
	m_pEditBoxStartHour->SetWindowTextA("0");
	m_pEditBoxStartMinute->SetWindowTextA("0");
	//slider default
	SetDlgItemText(IDC_STRING_SEARCH_START_TIME_SLIDER, _T("00 : 00 : 00"));
	SetDlgItemText(IDC_STRING_SEARCH_END_TIME_SLIDER, _T("00 : 00 : 00"));
	SetDlgItemText(IDC_STRING_FPS_SLIDER, to_string(fps).c_str());
	m_sliderSearchStartTime.SetPos(0);
	m_sliderSearchEndTime.SetPos(0);
	m_sliderFps.SetPos(fps);
}

void CMFC_SyntheticDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMFC_SyntheticDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMFC_SyntheticDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CMFC_SyntheticDlg::OnBnClickedOk()
{
	AfxMessageBox("OK 버튼 눌림");
}


// 디스플레이 함수
void CMFC_SyntheticDlg::DisplayImage(int IDC_PICTURE_TARGET, Mat targetMat, int TIMER_ID){
	if (targetMat.empty()) {	//예외처리. 프레임이 없음
		perror("Empty Frame");
		KillTimer(TIMER_ID);
		return ;
	}
	BITMAPINFO bitmapInfo;
	memset(&bitmapInfo, 0, sizeof(bitmapInfo));
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	bitmapInfo.bmiHeader.biWidth = targetMat.cols;
	bitmapInfo.bmiHeader.biHeight = -targetMat.rows;

	// IplImage 주소설정
	IplImage *tempImage;
	IplImage tempImageAddress;
	tempImage = &tempImageAddress;

	if (targetMat.channels() == 1)
	{
		tempImage = cvCreateImage(targetMat.size(), IPL_DEPTH_8U, 3);
		cvCvtColor(&IplImage(targetMat), tempImage, CV_GRAY2BGR);
	}
	else if (targetMat.channels() == 3)
	{
		tempImage = cvCloneImage(&IplImage(targetMat));
	}

	bitmapInfo.bmiHeader.biBitCount = tempImage->depth * tempImage->nChannels;

	CDC* pDC;
	pDC = GetDlgItem(IDC_RESULT_IMAGE)->GetDC();
	CRect rect;
	GetDlgItem(IDC_RESULT_IMAGE)->GetClientRect(&rect);

	//http://blog.naver.com/PostView.nhn?blogId=hayandoud&logNo=220851430885&categoryNo=0&parentCategoryNo=0&viewDate=&currentPage=1&postListTopCurrentPage=1&from=postView
	pDC->SetStretchBltMode(COLORONCOLOR);

	// 영상 비유을을 계산하여 Picture control에 출력
	float fImageRatio = float(tempImage->width) / float(tempImage->height);
	float fRectRatio = float(rect.right) / float(rect.bottom);
	float fScaleFactor;
	if (fImageRatio < fRectRatio){
		fScaleFactor = float(rect.bottom) / float(tempImage->height);	//TRACE("%f",fScaleFactor);
		int rightWithRatio = tempImage->width * fScaleFactor;
		float halfOfDif = ((float)rect.right - (float)rightWithRatio) / (float)2;
		rect.left = halfOfDif;
		rect.right = rightWithRatio;
	}
	else{
		fScaleFactor = float(rect.right) / float(tempImage->width);	//TRACE("%f",fScaleFactor);
		int bottomWithRatio = tempImage->height * fScaleFactor;
		float halfOfDif = ((float)rect.bottom - (float)bottomWithRatio) / (float)2;
		rect.top = halfOfDif;
		rect.bottom = bottomWithRatio;
	}
	//이미지 출력 (https://thebook.io/006796/ch03/04/01_01/)
	::StretchDIBits(pDC->GetSafeHdc(),
		rect.left, rect.top, rect.right, rect.bottom,
		0, 0, tempImage->width, tempImage->height,
		tempImage->imageData, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);

	cvReleaseImage(&tempImage);
}

//타이머
void CMFC_SyntheticDlg::OnTimer(UINT_PTR nIDEvent)
{
	CDialogEx::OnTimer(nIDEvent);
	switch (nIDEvent){
		timer_fps++;
	case VIDEO_TIMER:
		if (true){
			printf("$");
			Mat temp_frame;
			capture.read(temp_frame);
			DisplayImage(IDC_RESULT_IMAGE, temp_frame, VIDEO_TIMER);
			temp_frame.release();
		}
		status = 1;	//은혜
		break;

	case SYN_RESULT_TIMER:
		if (isPlayBtnClicked == true){
			printf("#");
			//TODO mat에 합성된 결과를 넣어준다.
			std::string BackgroundFilename = getBackgroundFilename(video_filename);
			Mat background = imread(BackgroundFilename);
			printf("불러올 배경파일 이름 :: %s, SYN_RESULT 타이머 호출!!!\n", BackgroundFilename.c_str());

			// 불러온 배경을 이용하여 합성을 진행
			Mat syntheticResult = getSyntheticFrame(background);
			DisplayImage(IDC_RESULT_IMAGE, syntheticResult, SYN_RESULT_TIMER);
			syntheticResult.release();			
		}
		status = 2;	//은혜
		break;
	}
	

}

// segmentation Button을 눌렀을 때의 연산
// 시와 분을 입력받아 segmentation을 진행함
void CMFC_SyntheticDlg::OnBnClickedBtnSegmentation()
{
	CString str_startHour, str_startMinute;
	//Edit Box로부터 시작 시간과 분을 읽어옴
	m_pEditBoxStartHour->GetWindowTextA(str_startHour);
	m_pEditBoxStartMinute->GetWindowTextA(str_startMinute);

	// Edit box에 문자 입력, 또는 범위외 입력 시 예외처리
	if (segmentationTimeInputException(str_startHour, str_startMinute) ) 
		segmentationOperator(&capture, atoi(str_startHour), atoi(str_startMinute));	//Object Segmentation
	
	// 범위 외 입력시 예외처리
	else {
	}
}

// segmentation 기능 수행, 물체 추적 및 파일로 저장
void segmentationOperator(VideoCapture* vc_Source, int videoStartHour, int videoStartMin)
{
	videoStartMsec = (videoStartHour * 60 + videoStartMin) * 60 * 1000;

	unsigned int COLS = (int)vc_Source->get(CV_CAP_PROP_FRAME_WIDTH);	//가로 길이
	unsigned int ROWS = (int)vc_Source->get(CV_CAP_PROP_FRAME_HEIGHT);	//세로 길이

	unsigned char* result = (unsigned char*)malloc(sizeof(unsigned char)* ROWS * COLS);

	/* 영상에 텍스트 출력 */
	char strBuffer[MAX_STR_BUFFER_SIZE] = { 0, }; // fps출력
	char timeBuffer[MAX_STR_BUFFER_SIZE] = { 0, }; // time 출력
	Scalar color(0, 0, 255); // B/G/R
	int thickness = 3;	// line thickness

	// humanDetector Vector
	vector<component> humanDetectedVector, prevHumanDetectedVector;

	/* Mat */
	Mat frame(ROWS, COLS, CV_8UC3); // Mat(height, width, channel)
	Mat frame_g(ROWS, COLS, CV_8UC1);
	Mat background(ROWS, COLS, CV_8UC3); // 배경 프레임과 원본 프레임
	Mat background_gray(ROWS, COLS, CV_8UC1);

	//frame 카운터와 현재 millisecond
	int frameCount = 0;
	unsigned int currentMsec;

	// 얻어낸 객체 프레임의 정보를 써 낼 텍스트 파일 정의
	fp = fopen(txt_filename.c_str(), "w");	// 쓰기모드
	fprintf(fp, to_string(videoStartMsec).append("\n").c_str());	//첫줄에 영상시작시간 적어줌
	
	while (1) {
		vc_Source->read(frame); //get single frame
		if (frame.empty()) {	//예외처리. 프레임이 없음
			perror("Empty Frame");
			break;
		}

		// 배경생성부분
		if (frameCount <= FRAMECOUNT_FOR_MAKE_BACKGROUND) {
			BackgroundMaker(frame, background, ROWS*3, COLS); 
			if (frameCount == (FRAMECOUNT_FOR_MAKE_BACKGROUND - 1)) {
				background_filename.append(video_filename).append(".jpg");
				int background_write_check = imwrite(background_filename, background);
				printf("background Making Complete!!\n");
			}
		}

		//그레이스케일 변환
		cvtColor(frame, frame_g, CV_RGB2GRAY);
		cvtColor(background, background_gray, CV_RGB2GRAY);

		// 전경 추출
		frame_g = ExtractFg(frame_g, background_gray, ROWS, COLS);
		// frame_g = ExtractForegroundToMOG2(frame_g);

		// 이진화
		threshold(frame_g, frame_g, 5, 255, CV_THRESH_BINARY);

		// 노이즈 제거
		frame_g = morphologicalOperation(frame_g);
		blur(frame_g, frame_g, Size(9, 9));

		// MAT형으로 라벨링
		humanDetectedVector.clear();
		humanDetectedVector = connectedComponentsLabelling(frame_g, ROWS, COLS);

		// 영상의 포착 가져오기
		currentMsec = vc_Source->get(CV_CAP_PROP_POS_MSEC);

		// 영상을 처리하여 파일로 저장하기
		humanDetectedVector = humanDetectedProcess(humanDetectedVector, prevHumanDetectedVector,
			frame, frameCount, videoStartMsec, currentMsec, fp);

		// 현재 검출한 데이터를 이전 데이터에 저장하기
		prevHumanDetectedVector = humanDetectedVector;

		frameCount++;	//increase frame count
	}

	//메모리 해제
	free(result); 	frame.release(); frame_g.release(); background.release();
	vector<component>().swap(humanDetectedVector);
	vector<component>().swap(prevHumanDetectedVector);
	fclose(fp);	// 텍스트 파일 닫기

	//HWND hWnd = ::FindWindow(NULL, "Dude, Wait");
	//if (hWnd){ ::PostMessage(hWnd, WM_CLOSE, 0, 0); }
	MessageBox(0, "Done!!", "ding-dong", MB_OK);
}

// 파일의 이름부분을 저장
string allocatingComponentFilename(vector<component> humanDetectedVector, int timeTag, int currentMsec, int frameCount, int indexOfhumanDetectedVector) {
	string name;
	return name.append(to_string(timeTag)).append("_")
		.append(to_string(currentMsec)).append("_")
		.append(to_string(frameCount)).append("_")
		.append(to_string(indexOfhumanDetectedVector));
}

// 현재와 이전에 검출한 결과를 비교, (현재는 두 ROI가 겹쳤는지를 판별, 겹치면 true인듯??)
bool IsComparePrevDetection(vector<component> curr_detected, vector<component> prev_detected, int curr_index, int prev_index) {
	return curr_detected[curr_index].left < prev_detected[prev_index].right
		&& curr_detected[curr_index].right < prev_detected[prev_index].left
		&& curr_detected[curr_index].top < prev_detected[prev_index].bottom
		&& curr_detected[curr_index].bottom < prev_detected[prev_index].top;
}

// humanDetectedVector의 파일이름과 timetag를 부여
// 그리고 TXT, JPG파일에 저장하는 모듈
void componentVectorHandling() {
	// TO DO :: 코드의 중복을 줄이기 위해 모듈을 어떻게 만들 것인지 생각하기(매개변수 설정도)
}

vector<component> humanDetectedProcess(vector<component> humanDetectedVector, vector<component> prevHumanDetectedVector
	, Mat frame, int frameCount, int videoStartMsec, unsigned int currentMsec, FILE *fp) {
	int prevTimeTag;
	for (int i = 0; i < humanDetectedVector.size(); i++) {
		// TODO : 현재 프레임에서 이전프레임과 겹치는 obj가 있는지 판단한다. 
		// 이전 오브젝트에 다음오브젝트가 두개가 걸칠 경우 어떻게 처리할 것인가?
		if (prevHumanDetectedVector.empty() == 0) {	//이전 프레임의 검출된 객체가 있을 경우
			bool findFlag = false;
			for (int j = 0; j < prevHumanDetectedVector.size(); j++) {
				if (IsComparePrevDetection(humanDetectedVector ,prevHumanDetectedVector, i, j)) {	// 두 ROI가 겹칠 경우
					// 이전 TimeTag를 할당
					prevTimeTag = prevHumanDetectedVector[j].timeTag;
					
					humanDetectedVector[i].fileName = allocatingComponentFilename(humanDetectedVector, prevTimeTag, currentMsec, frameCount, i);
					humanDetectedVector[i].timeTag = prevTimeTag;
					saveSegmentation_JPG(humanDetectedVector[i], frame, frameCount, currentMsec, i, videoStartMsec, video_filename);
					saveSegmentation_TXT(humanDetectedVector[i], frameCount, currentMsec, fp, i);
					findFlag = true;
					//break;
				}
			}

			if (findFlag == false) { // 새 객체의 출현
				humanDetectedVector[i].timeTag = currentMsec;
				humanDetectedVector[i].fileName = allocatingComponentFilename(humanDetectedVector, currentMsec, currentMsec, frameCount, i);
				saveSegmentation_JPG(humanDetectedVector[i], frame, frameCount, currentMsec, i, videoStartMsec, video_filename);
				saveSegmentation_TXT(humanDetectedVector[i], frameCount, currentMsec, fp, i);
			}
		}
		else {	// 첫 시행이거나 이전 프레임에 검출된 객체가 없을 경우
			// 새로운 이름 할당
			humanDetectedVector[i].timeTag = currentMsec;
			humanDetectedVector[i].fileName = allocatingComponentFilename(humanDetectedVector, currentMsec, currentMsec, frameCount, i);
			saveSegmentation_JPG(humanDetectedVector[i], frame, frameCount, currentMsec, i, videoStartMsec, video_filename);
			saveSegmentation_TXT(humanDetectedVector[i], frameCount, currentMsec, fp, i);
			// 위 네줄에 대한 코드중복이 좀 있음, 정보 추가 시 변형 가능성도 존재(검색 기능 구현할 때)
		}
	}
	return humanDetectedVector;
}

// 합성된 프레임을 가져오는 연산
Mat getSyntheticFrame(Mat backGround) {
		int *labelMap
			= (int*)calloc(backGround.cols * backGround.rows, sizeof(int));	//겹침을 판단하는 용도

		node tempnode;	//DeQueue한 결과를 받을 node
		int countOfObj = segment_queue.count;	//큐 인스턴스의 노드 갯수
		stringstream ss;

		//큐가 비었는지 확인한다
		if (IsEmpty(&segment_queue)){
			free(labelMap);
			return backGround;
		}

		vector<int> vectorPreNodeIndex; // 객체들 인덱스 정보를 저장하기 위한 벡터

		for (int k = 0; k < countOfObj; k++){
			tempnode = Dequeue(&segment_queue);
			//if (m_segmentArray[tempnode.indexOfSegmentArray].timeTag == m_segmentArray[tempnode.indexOfSegmentArray].msec)
			vectorPreNodeIndex.push_back(tempnode.indexOfSegmentArray);	//큐에 있는 객체들의 인덱스 정보 저장
			Enqueue(&segment_queue, tempnode.timeTag, tempnode.indexOfSegmentArray);
		}
		
		// 큐에 들어있는 객체 갯수 만큼 DeQueue. 
		for (int i = 0; i < countOfObj; i++) {
			//dequeue한 객체를 출력한다.
			tempnode = Dequeue(&segment_queue);
			BOOL isCross= false;
			int curIndex = tempnode.indexOfSegmentArray;
			
			//printf("\n%d : %s", i + 1, m_segmentArray[curIndex].fileName);
			//객체가 이전 객체와 겹치는지 비교
			if (i != 0 && m_segmentArray[curIndex].timeTag == m_segmentArray[curIndex].msec){	//처음이 아니고 현재 출력할 객체가 timetag의 첫 프레임 일 때
				for (int j = 0; j< i; j++){	//이전에 그린 객체 모두와 겹치는지 판별
					if (IsObjectOverlapingDetector(m_segmentArray, vectorPreNodeIndex, curIndex, j) ){
						isCross = false; // 겹치지 않음
					}
					else{ //겹침
						isCross = true;
						Enqueue(&segment_queue, tempnode.timeTag, tempnode.indexOfSegmentArray);	//출력하지 않고 다시 큐에 삽입
						break;
					}
				}
			}

			if (isCross == false){	//출력된 객체가 없거나 이전 객체와 겹치지 않는 경우
				//배경에 객체를 올리는 함수
				backGround = printObjOnBG(backGround, m_segmentArray[tempnode.indexOfSegmentArray], labelMap);

				//타임태그를 string으로 변환
				string timetag = "";
				int timetagInSec = (m_segmentArray[tempnode.indexOfSegmentArray].timeTag + videoStartMsec) / 1000;	//영상의 시작시간을 더해준다.
				ss = timeConvertor(timetagInSec);
				timetag = ss.str();

				//커팅된 이미지에 타임태그를 달아준다
				//params : (Mat, String to show, 출력할 위치, 폰트 타입, 폰트 크기, 색상, 굵기) 
				putText(backGround, timetag, Point(m_segmentArray[tempnode.indexOfSegmentArray].left + 5, m_segmentArray[tempnode.indexOfSegmentArray].top - 10), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 150, 150), 2);

				//다음목록에 같은 타임태그를 가진 객체가 있는지 확인한다. 있으면 EnQueue
				if (m_segmentArray[tempnode.indexOfSegmentArray + 1].timeTag == m_segmentArray[tempnode.indexOfSegmentArray].timeTag) {
					Enqueue(&segment_queue, tempnode.timeTag, tempnode.indexOfSegmentArray + 1);
				}
			}

		}

		free(labelMap);
		vector<int>().swap(vectorPreNodeIndex);
	return backGround;
}
// 객체들 끼리 겹침을 판별하는 함수, 하나라도 성립하면 겹치지 않음
bool IsObjectOverlapingDetector(segment *m_segment, vector<int> preNodeIndex_data, int curIndex, int countOfObj_j) {
	return m_segment[curIndex].left > m_segmentArray[preNodeIndex_data.at(countOfObj_j)].right
		|| m_segment[curIndex].right < m_segmentArray[preNodeIndex_data.at(countOfObj_j)].left
		|| m_segment[curIndex].top > m_segmentArray[preNodeIndex_data.at(countOfObj_j)].bottom
		|| m_segment[curIndex].bottom < m_segmentArray[preNodeIndex_data.at(countOfObj_j)].top;
}

//slider control이 움직이면 발생하는 콜백
void CMFC_SyntheticDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar == (CScrollBar*)&m_sliderSearchStartTime)
		SetDlgItemText(IDC_STRING_SEARCH_START_TIME_SLIDER, timeConvertor(m_sliderSearchStartTime.GetPos()).str().c_str());
	else if (pScrollBar == (CScrollBar*)&m_sliderSearchEndTime)
		SetDlgItemText(IDC_STRING_SEARCH_END_TIME_SLIDER, timeConvertor(m_sliderSearchEndTime.GetPos()).str().c_str());
	else if (pScrollBar == (CScrollBar*)&m_sliderFps)
		SetDlgItemText(IDC_STRING_FPS_SLIDER, to_string(m_sliderFps.GetPos()).c_str());
	

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

// Synthesis Button을 눌렀을 경우에 실행되는 핸들러
void CMFC_SyntheticDlg::OnClickedBtnSynPlay()
{
	isPlayBtnClicked = true;
	string path = "./";
	path.append(txt_filename);

	fp = fopen(path.c_str(), "r");
	boolean isPlayable = false;
	if (fp){	//파일을 제대로 불러왔을 경우
		//포인터 끝으로 이동하여 파일 크기 측정
		fseek(fp, 0, SEEK_END);
		if (ftell(fp) != 0)	//파일 크기가 0 이 아닐 경우 실행
			isPlayable = true;
	}

	if (isPlayable){
		//*******************************************텍스트파일을 읽어서 정렬****************************************************************
		m_segmentArray = new segment[BUFFER];  //(segment*)calloc(BUFFER, sizeof(segment));	//텍스트 파일에서 읽은 segment 정보를 저장할 배열 초기화
		segmentCount = 0;
		fseek(fp, 0, SEEK_SET);	//포인터 처음으로 이동
		fgets(txtBuffer, 99, fp);
		sscanf(txtBuffer, "%d", &videoStartMsec);
		
		// frameInfo.txt 파일에서 데이터를 추출 하여 segment array 초기화
		while (!feof(fp)) {
			fgets(txtBuffer, 99, fp);

			// txt파일에 있는 프레임 데이터들 segmentArray 버퍼로 복사
			sscanf(txtBuffer, "%d_%d_%d_%d %d %d %d %d %d %d",
				&m_segmentArray[segmentCount].timeTag, &m_segmentArray[segmentCount].msec,
				&m_segmentArray[segmentCount].frameCount, &m_segmentArray[segmentCount].index,
				&m_segmentArray[segmentCount].left, &m_segmentArray[segmentCount].top,
				&m_segmentArray[segmentCount].right, &m_segmentArray[segmentCount].bottom,
				&m_segmentArray[segmentCount].width, &m_segmentArray[segmentCount].height);

			// filename 저장
			m_segmentArray[segmentCount].fileName
				.append(to_string(m_segmentArray[segmentCount].timeTag)).append("_")
				.append(to_string(m_segmentArray[segmentCount].msec)).append("_")
				.append(to_string(m_segmentArray[segmentCount].frameCount)).append("_")
				.append(to_string(m_segmentArray[segmentCount].index)).append(".jpg");

			// m_segmentArray의 인덱스 증가
			segmentCount++;
		}

		// 버블 정렬 사용하여 m_segmentArray를 TimeTag순으로 정렬
		segment tmp_segment;
		for (int i = 0; i < segmentCount; i++) {
			for (int j = 0; j < segmentCount - 1; j++) {
				if (m_segmentArray[j].timeTag > m_segmentArray[j + 1].timeTag) {
					// m_segmentArray[segmentCount]와 m_segmentArray[segmentCount + 1]의 교체
					tmp_segment = m_segmentArray[j + 1];
					m_segmentArray[j + 1] = m_segmentArray[j];
					m_segmentArray[j] = tmp_segment;
				}
			}
		}

		//정렬 확인 코드
		//{
		//for (int i = 0; i < segmentCount; i++)
		//cout << m_segmentArray[i].fileName << endl;
		//}

		fclose(fp);	// 텍스트 파일 닫기
		//****************************************************************************************************************

		//큐 초기화
		InitQueue(&segment_queue);

		/************************************/
		//TimeTag를 Edit box로부터 입력받음
		unsigned int obj1_TimeTag = m_sliderSearchStartTime.GetPos() * 1000;	//검색할 TimeTag1
		unsigned int obj2_TimeTag = m_sliderSearchEndTime.GetPos() * 1000;	//검색할 TimeTag2

		if (obj1_TimeTag > obj2_TimeTag){
			AfxMessageBox("Search start time can't larger than end time");
			return;
		}

		bool find1 = false;
		bool find2 = false;

		int prevTimetag = 0;
		//출력할 객체를 큐에 삽입하는 부분
		for (int i = 0; i < segmentCount; i++) {
			//start timetag와 end timetag 사이면 enqueue
			if (m_segmentArray[i].timeTag >= obj1_TimeTag && m_segmentArray[i].timeTag <= obj2_TimeTag && prevTimetag != m_segmentArray[i].timeTag) {	//아직 찾지 못했고 일치하는 타임태그를 찾았을 경우
				Enqueue(&segment_queue, m_segmentArray[i].timeTag, i);	//출력해야할 객체의 첫 프레임의 타임태그와 위치를 큐에 삽입
				prevTimetag = m_segmentArray[i].timeTag;
			}
		}
		/***********/


		//실행중인 타이머 종료
		//KillTimer(VIDEO_TIMER);
		//타이머 시작	params = timerID, ms, callback함수 명(NULL이면 OnTimer)
		//SetTimer(SYN_RESULT_TIMER, 1000 / m_sliderFps.GetPos(), NULL);
	}
	else{ //실행 못하는 경우 segmentation을 진행하라고 출력
		AfxMessageBox("You can't play without segmentation results");
	}


}

// segmentation을 할 떄에 입력받는 수의 범위를 한정해주는 함수
bool segmentationTimeInputException(CString str_h, CString str_m) {
	// 시 :: 1~24, 분 :: 1~60
	if ((atoi(str_h) > 0 && atoi(str_h) <= 24)
		&& (atoi(str_m) > 0 && atoi(str_m) <= 60) ){
		return true;
	}

	else if ( (str_h == "0" && atoi(str_h) == 0)
		|| (str_m == "0" && atoi(str_m) == 0) )
		return true;
	// To Do :: 하나는 0, 다른 하나는 문자를 받았을 떄 실행되는 경우가 있음
	else if ((str_h != "0" && atoi(str_h) == 0)
		|| (str_m != "0" && atoi(str_m) == 0))
		return false;

	else
		return false;
}


//00:00:00 형식으로 timetag를 변환
stringstream timeConvertor(int t) {
	int hour;
	int min;
	int sec;
	stringstream s;

	hour = t / 3600;
	min = (t % 3600) / 60;
	sec = t % 60;

	if (t / 3600 < 10)
		s << "0" << hour << " : ";
	else
		s << hour << " : ";

	if ((t % 3600) / 60 < 10)
		s << "0" << min << " : ";
	else
		s << min << " : ";

	if (t % 60 < 10)
		s << "0" << sec;
	else
		s << sec;

	return s;
}

//load 버튼을 누르면 발생하는 콜백
void CMFC_SyntheticDlg::OnBnClickedBtnMenuLoad(){
	loadFile();
}


//은혜
void CMFC_SyntheticDlg::OnBnClickedBtnPlay()
{
	/*
	원본 영상일 경우, 합성 영상일 경우
	
	if(status == 1)
		SetTimer(VIDEO_TIMER, fps, NULL);

	else if(status == 2)
		SetTimer(IDC_RESULT_IMAGE, 1000 / m_sliderFps.GetPos(), NULL);
		
		*/
	
	SetTimer(VIDEO_TIMER, fps, NULL);
	printf("play\n");
	timer = true;
	
	
}


void CMFC_SyntheticDlg::OnBnClickedBtnPause()
{
	/*
	원본 영상일 경우, 합성 영상일 경우
	

	if (status == 1)
		KillTimer(VIDEO_TIMER);

	else if (status == 2)
		KillTimer(IDC_RESULT_IMAGE);
		
		*/
	KillTimer(VIDEO_TIMER);
	printf("pause\n");
	timer = false;
}

void CMFC_SyntheticDlg::OnBnClickedBtnStop()
{
	/*
	if (status == 1)
		KillTimer(VIDEO_TIMER);

	else if (status == 2)
		KillTimer(IDC_RESULT_IMAGE);
		
		*/

	KillTimer(VIDEO_TIMER);
	timer = false;

	timer_fps = 0;
}
