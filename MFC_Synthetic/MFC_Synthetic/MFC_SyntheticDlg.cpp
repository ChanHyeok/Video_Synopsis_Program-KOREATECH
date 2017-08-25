// MFC_SyntheticDlg.cpp : implementation file
#include <crtdbg.h>
#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "MFC_SyntheticDlg.h"
#include "afxdialogex.h"
#include "ProgressDlg.h"
#include "InitBGCounts.h"

// 메모리 누수를 점검하는 키워드 (http://codes.xenotech.net/38)
// 점검하기 위해 디버깅 모드로 실행 후, 디버그 로그를 보면 됨
// #include <crtdbg.h> 선언 이후 _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); 
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*** 상수 ***/
#define LOGO_TIMER 0	//Picture Control에 logo를 출력하는 타이머
#define VIDEO_TIMER 1	//Picture Control에 원본 영상을 출력하는 타이머
#define SYN_RESULT_TIMER 2	//Picture Control에 합성 영상을 출력하는 타이머
#define BIN_VIDEO_TIMER 3	//Picture Control에 이진 영상을 출력하는 타이머
#define MAX_STR_BUFFER_SIZE  128 // 문자열 출력에 사용하는 버퍼 길이


//콤보박스 텍스트
const char* ALL = "전체";
const char* LEFT = "왼쪽";
const char* RIGHT = "오른쪽";
const char* ABOVE = "윗쪽";
const char* BELOW = "아랫쪽";
const char* CENTER = "가운데";
const char* LEFTABOVE = "좌상단";
const char* RIGHTABOVE = "우상단";
const char* LEFTBELOW = "좌하단";
const char* RIGHTBELOW = "우하단";

// 배경 생성
<<<<<<< HEAD

const int FRAMES_FOR_MAKE_BACKGROUND = 800;	//영상 Load시 처음에 배경을 만들기 위한 프레임 수
const int FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND = 1000;	//다음 배경을 만들기 위한 시간간격(동적)
=======
int FRAMES_FOR_MAKE_BACKGROUND;//영상 Load시 처음에 배경을 만들기 위한 프레임 수
int FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND;//다음 배경을 만들기 위한 시간간격(동적)
>>>>>>> set_extra_module
// fps가 약 23-25 가량 나오는 영상에서 약 1분이 흐른 framecount 값은 1500

/***  전역변수  ***/
segment *m_segmentArray;
Queue segment_queue; // C++ STL의 queue 키워드와 겹치기 때문에 변수를 조정함
int videoStartMsec, fps, totalFrameCount; // 시작 millisecond, 세그먼트 카운팅변수, 초당 프레임수, 전체 프레임 수
unsigned int videoLength;	//비디오 길이(초)
int radioChoice, preRadioChoice;	//라디오 버튼 선택 결과 저장 변수. 0 - 원본영상, 1 - 합성영상, 2 - 이진영상
boolean isPlayBtnClicked, isPauseBtnClicked;
unsigned int COLS, ROWS;
Mat background_loadedFromFile, background_binaryVideo_gray; // 배경 프레임 , 이진 영상 출력시에 dynamic bg을 저장할 변수
unsigned int* bg_array;

bool synthesisEndFlag; // 합성이 끝남을 알려주는 플래그
bool isAlreadyBGMade; //이미 만든 배경이 있는지. 이진 영상 드래그 시에 변경

// File 관련
FILE *fp_detail; // frameInfo를 작성할 File Pointer
std::string fileNameExtension; // 입력받은 비디오파일 이름
std::string fileNameNoExtension;// 확장자가 없는 파일 이름
std::string txt_filename = RESULT_TEXT_FILENAME; //txt 파일 이름

/*
About Dialog
프로그램에 대해 설명하는 다이얼로그. MFC 윈도우 아이콘 우측 클릭 시 접근 가능
*/
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

// 현재시간을 string type으로 return하는 함수
string currentDateTime() {
	time_t     now = time(0); //현재 시간을 time_t 타입으로 저장
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", &tstruct); // YYYY-MM-DD.HH:mm:ss 형태의 스트링

	return buf;
}

/*
Main Dialog
프로그램을 띄울 Single Dialog
*/
CMFC_SyntheticDlg::CMFC_SyntheticDlg(CWnd* pParent /*=NULL*/)
: CDialogEx(CMFC_SyntheticDlg::IDD, pParent)
, mRadioPlay(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);	//프로그램 아이콘 설정
}

//DDX, 자동으로 데이터를 주고받을 수 있도록 연결 해 줌. 이 프로젝트에서는 활용하지 않고 있음
void CMFC_SyntheticDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SYN_SLIDER_START_TIME, m_sliderSearchStartTime);
	DDX_Control(pDX, IDC_SYN_SLIDER_END_TIME, m_sliderSearchEndTime);
	DDX_Control(pDX, IDC_SYN_SLIDER_FPS, m_sliderFps);
	DDX_Radio(pDX, IDC_RADIO_PLAY1, mRadioPlay);
	DDX_Control(pDX, IDC_SEG_SLIDER_WMIN, m_SliderWMIN);
	DDX_Control(pDX, IDC_SEG_SLIDER_WMAX, m_SliderWMAX);
	DDX_Control(pDX, IDC_SEG_SLIDER_HMIN, m_SliderHMIN);
	DDX_Control(pDX, IDC_SEG_SLIDER_HMAX, m_SliderHMAX);
	DDX_Control(pDX, IDC_SLIDER_PLAYER, m_SliderPlayer);
	DDX_Control(pDX, IDC_COMBO_START, mComboStart);
	DDX_Control(pDX, IDC_COMBO_END, mComboEnd);
	DDX_Control(pDX, IDC_BTN_SYN_SAVE, mButtonSynSave);
}

//message map을 정의하는 부분
BEGIN_MESSAGE_MAP(CMFC_SyntheticDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_SEGMENTATION, &CMFC_SyntheticDlg::OnBnClickedBtnSegmentation)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_BTN_PLAY, &CMFC_SyntheticDlg::OnClickedBtnPlay)
	ON_BN_CLICKED(IDC_BTN_MENU_LOAD, &CMFC_SyntheticDlg::OnBnClickedBtnMenuLoad)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO_PLAY1, IDC_RADIO_PLAY3, &CMFC_SyntheticDlg::SetRadioStatus)
	ON_BN_CLICKED(IDC_BTN_PAUSE, &CMFC_SyntheticDlg::OnBnClickedBtnPause)
	ON_BN_CLICKED(IDC_BTN_STOP, &CMFC_SyntheticDlg::OnBnClickedBtnStop)
	ON_BN_CLICKED(IDC_BTN_REWIND, &CMFC_SyntheticDlg::OnBnClickedBtnRewind)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_PLAYER, &CMFC_SyntheticDlg::OnReleasedcaptureSliderPlayer)
	ON_BN_CLICKED(IDOK, &CMFC_SyntheticDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CMFC_SyntheticDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BTN_SYN_SAVE, &CMFC_SyntheticDlg::OnBnClickedBtnSynSave)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SYN_SLIDER_FPS, &CMFC_SyntheticDlg::OnReleasedcaptureSynSliderFps)
	ON_WM_DRAWITEM()
	ON_BN_CLICKED(IDC_CHECK_ALL, &CMFC_SyntheticDlg::OnBnClickedCheckAll)
END_MESSAGE_MAP()


// CMFC_SyntheticDlg message handlers
BOOL CMFC_SyntheticDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	ShowWindow(SW_SHOWMAXIMIZED);	//전체화면
	this->GetWindowRect(m_rectCurHist);	//다이얼로그 크기를 얻어옴

	//ASSERT : 파라메터가 0 이면 메시지 출력
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	//MFC 아이콘의 오른쪽 클릭했을 경우 표시되는 폼 메뉴를 설정
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

	//프로그램 아이콘 설정
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon


	//레이아웃 컨트롤들 초기화 및 위치 지정
	layoutInit();

	//실행시 비디오 파일 불러옴
	loadFile(0);

	//Slider Control 범위 지정
	setSliderRange(videoLength, COLS, ROWS, 100);

	//UI 업데이트, control에 default 값 할당
	updateUI(videoLength, COLS, ROWS, fps);

	//Slider Control
	m_sliderSearchStartTime.EnableWindow(FALSE);	//비활성화
	m_sliderSearchEndTime.EnableWindow(FALSE);

	// Play, Pause버튼 상태 초기화
	isPlayBtnClicked = false;
	isPauseBtnClicked = true;
	isAlreadyBGMade = false;

	// 라디오 버튼 초기화
	CheckRadioButton(IDC_RADIO_PLAY1, IDC_RADIO_PLAY3, IDC_RADIO_PLAY1);
	radioChoice = 0; preRadioChoice = 0; //라디오 버튼의 default는 맨 처음 버튼임
	mButtonSynSave.EnableWindow(false);

	SetTimer(LOGO_TIMER, 1, NULL);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

//mode 0 : 취소시 프로그램 종료. 1 : 취소시 다이얼로그만 종료
int CMFC_SyntheticDlg::loadFile(int mode) {
	//파일 다이얼로그 호출해서 segmentation 할 영상 선택	
	char szFilter[] = "Video (*.avi, *.MP4) | *.avi;*.mp4; | All Files(*.*)|*.*||";	//검색 옵션
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter, AfxGetMainWnd());	//파일 다이얼로그 생성
	if (dlg.DoModal() == 1) {	//다이얼로그 띄움
		//FRAMES_FOR_MAKE_BACKGROUND, FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND 입력 받기
		CInitBGCounts InitBGCount(this);                // this 를 사용하여 부모를 지정.
		InitBGCount.CenterWindow();
		if (InitBGCount.DoModal() == 1){//OK 눌렀을 경우
			FRAMES_FOR_MAKE_BACKGROUND = InitBGCount.BGMAKINGCOUNTS;
			FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND = InitBGCount.BGUPDATECOUNTS;
		}
		else OnCancel();

		//load한 영상의 이름을 text control에 표시
		CString cstrImgPath = dlg.GetPathName();	//path
		videoFilePath = (string)cstrImgPath;
		String stringFileName = "File Name : ";	//출력할 문자열
		fileNameExtension.clear();
		fileNameExtension = getFileName(cstrImgPath, '\\', true);

		CWnd *pStringFileName = GetDlgItem(IDC_MENU_STRING_FILE_NAME);
		char *video_filename_cstr = new char[stringFileName.length() + 1];
		strcpy(video_filename_cstr, stringFileName.c_str());
		strcat(video_filename_cstr, fileNameExtension.c_str());
		pStringFileName->SetWindowTextA(video_filename_cstr);

		video_filename_cstr = nullptr;
		delete[] video_filename_cstr;

		//segmentation 결과를 저장할 텍스트 파일 이름 설정
		txt_filename = RESULT_TEXT_FILENAME;
		fileNameNoExtension = getFileName(cstrImgPath, '\\', false);
		txt_filename = txt_filename.append(fileNameNoExtension).append(".txt");

		// 세그먼테이션 데이터(txt, jpg들)를 저장할 디렉토리 유무확인, 없으면 만들어줌

		// root 디렉토리 생성(폴더명 data)
		if (!isDirectory(SEGMENTATION_DATA_DIRECTORY_NAME.c_str()))
			int rootDirectory_check = makeDataRootDirectory();

		// video 이름 별 디렉토리 생성(폴더명 확장자 없는 파일 이름)
		if (!isDirectory(getDirectoryPath(fileNameNoExtension.c_str())))
			int subDirectory_check = makeDataSubDirectory(getDirectoryPath(fileNameNoExtension));

		// obj 디렉토리 생성
		if (!isDirectory(getObjDirectoryPath(fileNameNoExtension.c_str())))
			int subObjDirectory_check = makeDataSubDirectory(getObjDirectoryPath(fileNameNoExtension));

		// color 검출을 위한 obj 디렉토리 생성
		if (!isDirectory(getObj_for_colorDirectoryPath(fileNameNoExtension.c_str())))
			int subObjDirectory_check = makeDataSubDirectory(getObj_for_colorDirectoryPath(fileNameNoExtension));

		capture.open((string)cstrImgPath);

		if (!capture.isOpened()) { //예외처리. 해당이름의 파일이 없는 경우
			perror("No Such File!\n");
			::SendMessage(GetSafeHwnd(), WM_CLOSE, NULL, NULL);	//다이얼 로그 종료
		}

		//영상 정보 - COLS,ROWS,FPS,비디오 길이(초)
		COLS = (int)capture.get(CV_CAP_PROP_FRAME_WIDTH); //가로 길이
		ROWS = (int)capture.get(CV_CAP_PROP_FRAME_HEIGHT); //세로 길이
		fps = capture.get(CV_CAP_PROP_FPS);
		totalFrameCount = (int)capture.get(CV_CAP_PROP_FRAME_COUNT);
		videoLength = (int)((totalFrameCount / (float)fps));	//비디오의 길이를 초단위로 계산


		background_binaryVideo_gray = Mat(ROWS, COLS, CV_8UC1);
		//평균 배경에 사용할 배열 생성 및 초기화
		bg_array = new unsigned int[ROWS*COLS];
		for (int i = 0; i < ROWS*COLS; i++){
			bg_array[i] = 0;
		}

		// 배경생성 및 파일로 저장(초반 n프레임)
		backgroundInit(videoFilePath);

		SetTimer(LOGO_TIMER, 1, NULL);

		//라디오버튼 - 합성영상 활성화 / 비활성화
		if (checkSegmentation()) {
			GetDlgItem(IDC_RADIO_PLAY2)->EnableWindow(TRUE);
		}
		else {
			GetDlgItem(IDC_RADIO_PLAY2)->EnableWindow(FALSE);
		}

		return 0;
	}
	else if (mode == 0){
		OnCancel();
	}
	else if (mode == 1){
		return 1;
	}
	else
		perror("Unkown mode error\n");
	return 1;
}

void CMFC_SyntheticDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}

	// 프로그램을 중단(x버튼)했을 때
	else if (nID == SC_CLOSE) {
		if (MessageBox("프로그램을 종료하시겠습니까?", "Exit", MB_YESNO) == IDYES) {
			// 종료시 이벤트
			AfxGetMainWnd()->PostMessage(WM_CLOSE);
		}
		else {
			// 취소시 이벤트
		}
	}

	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// MFC에서 종료(x)버튼을 누를 시, OnClose()->OnCancel()->OnDestroy()순으로 호출되어 끝남
// OnClose(), OnDestroy()는 이용할 필요가 없어서 생략함

// 공통 변수 메모리 해제 및 종료연산
void CMFC_SyntheticDlg::OnCancel() {
	m_segmentArray = NULL;
	capture = NULL;
	background_loadedFromFile = NULL;
	background_binaryVideo_gray = NULL;

	delete[] bg_array;
	// cpp파일 내 전역변수들 메모리 해제
	fileNameExtension.clear();
	fileNameNoExtension.clear();
	txt_filename.clear();

	background_loadedFromFile.release();
	if (m_segmentArray != nullptr)
		delete[] m_segmentArray;

	// CMFC_SyntheticDlg 클래스의 멤버변수들 메모리 해제
	capture.release();
	background_binaryVideo_gray.release();

	KillTimer(LOGO_TIMER);
	KillTimer(VIDEO_TIMER);
	KillTimer(BIN_VIDEO_TIMER);
	KillTimer(SYN_RESULT_TIMER);

	// To Do :: 열려있는 텍스트 파일 모두 닫음

	PostQuitMessage(0);
}
void CMFC_SyntheticDlg::OnDestroy() {
}

//다이얼로그를 그릴 때 혹은 다시 그릴 때 호출되는 함수
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

//최소화된 CWND가 사용자에 의해 드래그 될 때
// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMFC_SyntheticDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// Picture Contorl Display 함수
void CMFC_SyntheticDlg::DisplayImage(int IDC_PICTURE_TARGET, Mat targetMat, int TIMER_ID) {
	if (targetMat.empty()) {	//예외처리. 프레임이 없음
		perror("Empty Frame");
		KillTimer(TIMER_ID);
		return;
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
	else {
		cvReleaseImage(&tempImage);
		return;
	}
	bitmapInfo.bmiHeader.biBitCount = tempImage->depth * tempImage->nChannels;

	CDC* pDC;
	pDC = GetDlgItem(IDC_RESULT_IMAGE)->GetDC();
	CRect rect;
	GetDlgItem(IDC_RESULT_IMAGE)->GetClientRect(&rect);

	//http://blog.naver.com/PostView.nhn?blogId=hayandoud&logNo=220851430885&categoryNo=0&parentCategoryNo=0&viewDate=&currentPage=1&postListTopCurrentPage=1&from=postView
	pDC->SetStretchBltMode(COLORONCOLOR);

	// 영상 비유을을 계산하여 Picture control에 출력
	double fImageRatio = double(tempImage->width) / double(tempImage->height);
	double fRectRatio = double(rect.right) / double(rect.bottom);
	double fScaleFactor;
	if (fImageRatio < fRectRatio) {
		fScaleFactor = double(rect.bottom) / double(tempImage->height);	//TRACE("%f",fScaleFactor);
		int rightWithRatio = tempImage->width * fScaleFactor;
		double halfOfDif = ((double)rect.right - (double)rightWithRatio) / (double)2;
		rect.left = halfOfDif;
		rect.right = rightWithRatio;
	}
	else {
		fScaleFactor = double(rect.right) / double(tempImage->width);	//TRACE("%f",fScaleFactor);
		int bottomWithRatio = tempImage->height * fScaleFactor;
		double halfOfDif = ((double)rect.bottom - (double)bottomWithRatio) / (double)2;
		rect.top = halfOfDif;
		rect.bottom = bottomWithRatio;
	}
	//이미지 출력 (https://thebook.io/006796/ch03/04/01_01/)
	::StretchDIBits(pDC->GetSafeHdc(),
		rect.left + 5, rect.top + 5, rect.right - 10, rect.bottom - 10,
		0, 0, tempImage->width, tempImage->height,
		tempImage->imageData, &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);

	cvReleaseImage(&tempImage);
}

//타이머
void CMFC_SyntheticDlg::OnTimer(UINT_PTR nIDEvent)
{
	CDialogEx::OnTimer(nIDEvent);
	Mat temp_frame;
	int curFrameCount = (int)capture.get(CV_CAP_PROP_POS_FRAMES);//현재 프레임 카운트
	m_SliderPlayer.SetPos(curFrameCount);	//슬라이더 위치를 조정
	SetDlgItemText(IDC_STRING_CUR_TIME, timeConvertor((int)(videoLength * curFrameCount / (float)totalFrameCount)).str().c_str());	//현재 씬의 시간 출력

	switch (nIDEvent) {
	case LOGO_TIMER:	//로고 출력
		if (true) {
			string logoFileName = "logo.jpg";
			Mat logo = imread(logoFileName);
			DisplayImage(IDC_RESULT_IMAGE, logo, LOGO_TIMER);
			logo.release();
			KillTimer(LOGO_TIMER);
			KillTimer(VIDEO_TIMER);
			KillTimer(BIN_VIDEO_TIMER);
			KillTimer(SYN_RESULT_TIMER);
		}
		break;

		// 원본 영상 출력
	case VIDEO_TIMER:
		printf("$");
		capture.read(temp_frame);
		DisplayImage(IDC_RESULT_IMAGE, temp_frame, VIDEO_TIMER);
		break;

		// 이진 영상 출력
	case BIN_VIDEO_TIMER:
		if (true) {
			Mat img_labels, stats, centroids;
			Mat loadBackground = Mat(ROWS, COLS, CV_8UC1);
			capture.read(temp_frame);
			int curFrameCount = (int)capture.get(CV_CAP_PROP_POS_FRAMES);
			int curFrameCount_nomalized = curFrameCount%FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND;
			if (temp_frame.empty()) {	//예외처리. 프레임이 없음
				perror("Empty Frame");
				KillTimer(BIN_VIDEO_TIMER);
				break;
			}

			//그레이스케일 변환
			cvtColor(temp_frame, temp_frame, CV_RGB2GRAY);

			//다음에 쓸 배경을 만들어야 할 경우
			if (curFrameCount_nomalized >= (FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND - FRAMES_FOR_MAKE_BACKGROUND)){
				if (curFrameCount_nomalized == (FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND - FRAMES_FOR_MAKE_BACKGROUND)){	//새로 만드는 첫 배경 Init
					printf("Background Making Start : %d frame\n", curFrameCount);
<<<<<<< HEAD
					//temp_frame.copyTo(background_binaryVideo_gray);
					setArrayToZero(bg_array, ROWS, COLS);
					isAlreadyBGMade = false;
=======
					background_binaryVideo_gray = temp_frame.clone();
>>>>>>> set_extra_module
				}
				else{	//배경 생성
					//temporalMedianBG(temp_frame, background_binaryVideo_gray);
					averageBG(temp_frame, bg_array);
				}
			}

			//만든 배경을 저장해야 할 경우
			if (curFrameCount_nomalized == FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND - 1 && !isAlreadyBGMade){
				accIntArrayToMat(background_binaryVideo_gray, bg_array, FRAMES_FOR_MAKE_BACKGROUND);
				imwrite(getTempBackgroundFilePath(fileNameNoExtension), background_binaryVideo_gray);
			}

			if (curFrameCount >= FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND && curFrameCount_nomalized == 0){
				printf("Background Changed, %d frame\n", curFrameCount);
			}

			//새로운 배경이 write되기 전 까지는 base gray배경을 사용
			if (curFrameCount < FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND - 1){
				loadBackground = imread(getBackgroundFilePath(fileNameNoExtension), IMREAD_GRAYSCALE);	//배경 로드
			}
			else{	//새로운 배경이 만들어진 다음부터는 만들어진 배경을 사용
				loadBackground = imread(getTempBackgroundFilePath(fileNameNoExtension), IMREAD_GRAYSCALE);	//배경 로드
			}

			temp_frame = ExtractFg(temp_frame, loadBackground, ROWS, COLS);// 전경 추출
			////TODO 손보기
			// 이진화
			threshold(temp_frame, temp_frame, 5, 255, CV_THRESH_BINARY);

			//// 노이즈 제거
			temp_frame = morphologyOpening(temp_frame);
			temp_frame = morphologyClosing(temp_frame);
			temp_frame = morphologyClosing(temp_frame);
			blur(temp_frame, temp_frame, Size(11, 11));

			threshold(temp_frame, temp_frame, 10, 255, CV_THRESH_BINARY);

			int numOfLables = connectedComponentsWithStats(temp_frame, img_labels, stats, centroids, 8, CV_32S);

			cvtColor(temp_frame, temp_frame, CV_GRAY2BGR);

			//라벨링 된 이미지에 각각 직사각형으로 둘러싸기 
			for (int j = 1; j < numOfLables; j++) {
				int left = stats.at<int>(j, CC_STAT_LEFT);
				int top = stats.at<int>(j, CC_STAT_TOP);
				int width = stats.at<int>(j, CC_STAT_WIDTH);
				int height = stats.at<int>(j, CC_STAT_HEIGHT);
				if (labelSizeFiltering(width, height
					, m_SliderWMIN.GetPos(), m_SliderWMAX.GetPos(), m_SliderHMIN.GetPos(), m_SliderHMAX.GetPos())) {
					rectangle(temp_frame, Point(left, top), Point(left + width, top + height),
						Scalar(0, 0, 255), 1);
				}
			}

			DisplayImage(IDC_RESULT_IMAGE, temp_frame, BIN_VIDEO_TIMER);
			img_labels = NULL;
			stats = NULL;
			centroids = NULL;
			loadBackground = NULL;
			loadBackground.release();
			img_labels.release();
			stats.release();
			centroids.release();
		}
		break;
	case SYN_RESULT_TIMER:
		printf("#");
		Mat bg_copy = background_loadedFromFile.clone();
		// 불러온 배경을 이용하여 합성을 진행
		Mat syntheticResult = getSyntheticFrame(&segment_queue, bg_copy, m_segmentArray);
		DisplayImage(IDC_RESULT_IMAGE, syntheticResult, SYN_RESULT_TIMER);
		syntheticResult = NULL;
		bg_copy = NULL;
		syntheticResult.release();
		bg_copy.release();
		break;
	}
	temp_frame = NULL;
	temp_frame.release();
}

// segmentation Button을 눌렀을 때의 연산
// 시와 분을 입력받아 segmentation을 진행함
void CMFC_SyntheticDlg::OnBnClickedBtnSegmentation()
{
	cout << "세그멘테이션 시작 시간 : " << currentDateTime() << endl;
	KillTimer(LOGO_TIMER);
	KillTimer(VIDEO_TIMER);
	KillTimer(BIN_VIDEO_TIMER);

	CString str_startHour, str_startMinute;
	//Edit Box로부터 시작 시간과 분을 읽어옴
	m_pEditBoxStartHour->GetWindowTextA(str_startHour);
	m_pEditBoxStartMinute->GetWindowTextA(str_startMinute);

	// Edit box에 문자 입력, 또는 범위외 입력 시 예외처리
	if (segmentationTimeInputException(str_startHour, str_startMinute)) {
		segmentationOperator(&capture, atoi(str_startHour), atoi(str_startMinute)
			, m_SliderWMIN.GetPos(), m_SliderWMAX.GetPos(), m_SliderHMIN.GetPos(), m_SliderHMAX.GetPos());	//Object Segmentation

		//라디오버튼 - 합성영상 활성화 / 비활성화
		if (checkSegmentation()) {
			GetDlgItem(IDC_RADIO_PLAY2)->EnableWindow(TRUE);
		}
		else {
			GetDlgItem(IDC_RADIO_PLAY2)->EnableWindow(FALSE);
		}

		cout << "세그멘테이션 종료 시간 : " << currentDateTime() << endl;
	}
	else {	// 범위 외 입력시 예외처리
	}
}

// segmentation 기능 수행, 물체 추적 및 파일로 저장
void CMFC_SyntheticDlg::segmentationOperator(VideoCapture* vc_Source, int videoStartHour, int videoStartMin, int WMIN, int WMAX, int HMIN, int HMAX) {
	videoStartMsec = (videoStartHour * 60 + videoStartMin) * 60 * 1000;
	unsigned int COLS = (int)vc_Source->get(CV_CAP_PROP_FRAME_WIDTH);	//가로 길이
	unsigned int ROWS = (int)vc_Source->get(CV_CAP_PROP_FRAME_HEIGHT);	//세로 길이

	CProgressDlg ProgressDlg(this);                // this 를 사용하여 부모를 지정.
	ProgressDlg.CenterWindow();
	ProgressDlg.mode = 1;
	ProgressDlg.videoFilePath = videoFilePath;
	ProgressDlg.ROWS = ROWS;
	ProgressDlg.COLS = COLS;
	ProgressDlg.WMIN = WMIN;
	ProgressDlg.WMAX = WMAX;
	ProgressDlg.HMIN = HMIN;
	ProgressDlg.HMAX = HMAX;
	ProgressDlg.videoStartMsec = videoStartMsec;
	ProgressDlg.FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND = FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND;
	ProgressDlg.FRAMES_FOR_MAKE_BACKGROUND = FRAMES_FOR_MAKE_BACKGROUND;
	ProgressDlg.fileNameNoExtension = fileNameNoExtension;
	ProgressDlg.DoModal();

	printf("segmentation Operator 끝\n");
}

bool isColorDataOperation(Mat frame, Mat bg, Mat binary, int i_height, int j_width) {
	// 배경 불러오기
	Vec3b colorB = bg.at<Vec3b>(Point(j_width, i_height));

	// hsv 영역에서 객체와 객체 영역의 배경과의 색깔 경계 값을 한정하기 위한 변수 
	const int COLOR_THRESHOLD = FOREGROUND_THRESHOLD;

	return (abs(colorB[0] - frame.at<cv::Vec3b>(Point(j_width, i_height))[0]) > COLOR_THRESHOLD &&
		abs(colorB[1] - frame.at<cv::Vec3b>(Point(j_width, i_height))[1]) > COLOR_THRESHOLD &&
		abs(colorB[2] - frame.at<cv::Vec3b>(Point(j_width, i_height))[2]) > COLOR_THRESHOLD) &&
		binary.data[i_height*binary.cols + j_width] == 255;
}

<<<<<<< HEAD
// colorArray를 구성하면서, component의 색 정보를 확인함
int* getColorData(Mat frame, component *object, Mat binary, Mat bg, int frameCount, int currentMsec){
	Mat temp; frame.copyTo(temp);
	
	// color 배열 초기화, 동적생성, segmentation 저장 이후 메모리 해제함
=======
int* getColorArray(Mat frame, component object, Mat binary){
	Mat temp = frame.clone();
	Mat bg_copy = imread(getBackgroundFilePath(fileNameNoExtension));
>>>>>>> set_extra_module
	int *colorArray = new int[COLORS];
	for (int i = 0; i < COLORS; i++)
		colorArray[i] = 0;

	//원본 프레임 각각 RGB, HSV로 변환하기
	Mat frame_hsv, frame_rgb;
	cvtColor(temp, frame_hsv, CV_BGR2HSV);
	cvtColor(temp, frame_rgb, CV_BGR2RGB);

	int sum_of_color_array[6] = { 0 , };

	// 한 프레임에서 유효한 color을 추출하는 연산을 하는 횟수, component의 color_count에 저장
	int temp_color_count = 0;
	for (int i = object->top; i < object->bottom; i++) {
		Vec3b* ptr_temp = temp.ptr<Vec3b>(i);
		Vec3b* ptr_color_hsv = frame_hsv.ptr<Vec3b>(i);
		Vec3b* ptr_color_rgb = frame_rgb.ptr<Vec3b>(i);

		for (int j = object->left + 1; j < object->right; j++) {
			// 색상 데이터 저장
			if (isColorDataOperation(frame, bg, binary, i, j)) {
				temp_color_count++;
				int color_check = colorPicker(ptr_color_hsv[j], ptr_color_rgb[j], colorArray);

				// 한 component의 color 평균을 구하기 위해 임시 배열에 합을 구함
				for (int c = 0; c < 3; c++) {
					sum_of_color_array[c] += ptr_color_hsv[j][c];
					sum_of_color_array[c + 3] += ptr_color_rgb[j][c];
				}
			}
			else {
				ptr_temp[j] = Vec3b(0, 0, 0);
			}

		}
	}

	// 무채색, 유채색의 밸런스를 맞추기 위한 연산, white와 black의 weight 조절
	colorArray[BLACK] *= 0.8;
	colorArray[WHITE] *= 0.8;

	// object의 색 영역(hsv, rgb) 평균 요소와 색 최종 카운트에 데이터 삽입,
	for (int c = 0; c < 3; c++) {
		object->hsv_avarage[c] = 0;
		object->rgb_avarage[c] = 0;
		if (sum_of_color_array[c] > 0 && sum_of_color_array[c + 3] > 0) {
			object->hsv_avarage[c] = sum_of_color_array[c] / object->area;
			object->rgb_avarage[c] = sum_of_color_array[c + 3] / object->area;
		}
	}
	object->color_count = temp_color_count;

	// color를 위한 obj를 jpg파일로 저장
	// 추후 삭제
	component temp_object = *object;
	temp_object.fileName = allocatingComponentFilename(temp_object.timeTag, currentMsec, frameCount, temp_object.label);
	saveSegmentation_JPG(temp_object, temp, getObj_for_colorDirectoryPath(fileNameNoExtension));

	// 확인 코드
	/*
	printf("timatag = %d) [", object.timeTag);
	for (int i = 0; i < 6; i++) {
		double color_value = (double)temp_color_array[i] / (double)get_color_data_count;
		printf("%.0lf ", color_value);
	}
	printf("] rate = %.2lf \n", rate_of_color_operation);
	*/

	//printf("%10d : ", object.timeTag);
	//for (int i = 0; i < COLORS;i++)
	//	printf("%d ",colorArray[i]);
	//printf("\n");

	temp = NULL;
	temp.release();
	return colorArray;
}

// component vector 큐를 이용한 추가된 함수
vector<component> humanDetectedProcess2(vector<component> humanDetectedVector, vector<component> prevHumanDetectedVector
	, ComponentVectorQueue prevHumanDetectedVector_Queue, Mat frame, int frameCount, unsigned int currentMsec, FILE *fp, Mat binary_frame) {

	// 현재에서 바로 이전 component 저장
	vector<component> prevDetectedVector_i = prevHumanDetectedVector;

	// 최대 레이블 정의
	int max_label = humanDetectedVector.size() - 1;
	for (int humanCount = 0; humanCount < humanDetectedVector.size(); humanCount++) {
		if (max_label < humanDetectedVector[humanCount].label) 
			max_label = humanDetectedVector[humanCount].label;
	}

	// 사람을 검출한 양 많큼 반복
	for (int humanCount = 0; humanCount < humanDetectedVector.size(); humanCount++) {
		bool findFlag = false;

		// 이전객체와 연속한다고 판단되는 component
		component prev_detected_component;

		//이전 프레임의 검출된 객체가 있을 경우
		if (!prevDetectedVector_i.empty()) {
			for (int j = 0; j < prevDetectedVector_i.size(); j++) {
				// 두 프레임이 겹칠 경우에 대한 연산
				if (!IsComparePrevComponent(humanDetectedVector[humanCount], prevDetectedVector_i[j])) {
					humanDetectedVector[humanCount].timeTag = prevDetectedVector_i[j].timeTag;
					humanDetectedVector[humanCount].label = prevDetectedVector_i[j].label;
					prev_detected_component = prevDetectedVector_i[j];
					findFlag = true;
				}
			} // end for

			// 이전 객체를 통해서 발견하지 못했음
			if (findFlag == false) {
				// 이전 뿐 아니라 그 이전에 데이터에 접근하기
				for (int i = MAXSIZE_OF_COMPONENT_VECTOR_QUEUE - 3; i >= 0; i--) {
					prevDetectedVector_i = GetComponentVectorQueue(&prevHumanDetectedVector_Queue,
						(prevHumanDetectedVector_Queue.rear + i) % MAXSIZE_OF_COMPONENT_VECTOR_QUEUE);
					for (int j = 0; j < prevDetectedVector_i.size(); j++) {
						// 두 프레임이 겹칠 경우에 대한 연산
						if (!IsComparePrevComponent(humanDetectedVector[humanCount], prevDetectedVector_i[j])) {
							humanDetectedVector[humanCount].timeTag = prevDetectedVector_i[j].timeTag;
							humanDetectedVector[humanCount].label = prevDetectedVector_i[j].label;
							prev_detected_component = prevDetectedVector_i[j];
							findFlag = true;
							break;

						}
					}
				} // end for (int i = MAXSIZE_OF_COMPONENT_VECTOR_QUEUE ...
			} // end if (findFlag == false) 
		} // end if ((!prevDetectedVector_i.empty())

		// 첫 시행이거나 이전 프레임에 검출된 객체가 없다고 판단될 경우에
		else {
			// 이전 뿐 아니라 그 이전에 데이터에 접근하기
			for (int i = MAXSIZE_OF_COMPONENT_VECTOR_QUEUE - 3; i >= 0; i--) {
				prevDetectedVector_i = GetComponentVectorQueue(&prevHumanDetectedVector_Queue,
					(prevHumanDetectedVector_Queue.rear + i) % MAXSIZE_OF_COMPONENT_VECTOR_QUEUE);
				for (int j = 0; j < prevDetectedVector_i.size(); j++) {
					if (!IsComparePrevComponent(humanDetectedVector[humanCount], prevDetectedVector_i[j])) {
						humanDetectedVector[humanCount].timeTag = prevDetectedVector_i[j].timeTag;
						humanDetectedVector[humanCount].label = prevDetectedVector_i[j].label;
						prev_detected_component = prevDetectedVector_i[j];
						findFlag = true;
						break;
					}
				}
			}
		} // end else
		
		// 새 객체가 출현 되었다고 판정함
		if (findFlag == false) {
			humanDetectedVector[humanCount].timeTag = currentMsec;
			humanDetectedVector[humanCount].label = ++max_label;
			// printf("timetag))%d label))%d\n", currentMsec, humanDetectedVector[humanCount].label);
		}

		// 현재 component의 저장 여부를 결정하는 플래그 생성
		bool save_flag = false;

		// 배경, 업데이트 할 때마다 불러옴
		Mat bg_copy = imread(getBackgroundFilePath(fileNameNoExtension));
		
		// 현재 component의 색상정보를 얻어냄
		int *colorArray = getColorData(frame, &humanDetectedVector[humanCount], binary_frame, bg_copy, frameCount, currentMsec);

		// 배경과의 차이가 거이 없는 것을 걸러내기 위해 색상 연산을 하는 카운트끼리 객체와 배경과 비교함
		double difference_value = (double)humanDetectedVector[humanCount].color_count 
			/ (double)(humanDetectedVector[humanCount].height *humanDetectedVector[humanCount].width);
		
		if (difference_value > 0.10)
			save_flag = true;

		else {
			save_flag = false;
			printf("save fail, rate_of_color_operation = %.2lf\n", difference_value);
		}

		// 연속성이 만족할 경우 파일에 저장할 수 있도록 함
		if ((findFlag == false) || ( (save_flag == true) && isSizeContinue(&humanDetectedVector[humanCount], &prev_detected_component)
			&& isColorContinue(&humanDetectedVector[humanCount], &prev_detected_component) )) {
			saveSegmentationData(fileNameNoExtension, humanDetectedVector[humanCount], frame
				, currentMsec, frameCount, fp, ROWS, COLS, colorArray);
		}

		// getColorData에서 생성한 colorArray 객체 메모리 해제
		delete[] colorArray;
	} // end for (humanCount) 
	vector<component> vclear;
	prevDetectedVector_i.swap(vclear);

	return humanDetectedVector;
}

// 이전과 연속적이어서 저장할 가치가 있는 지를 판별하는 함수
bool isSizeContinue(component *curr_component, component *prev_component) {
	const int diff_component_height = prev_component->height* 0.3; //  ( 480/15 = 32)
	const int diff_component_width = prev_component->width * 0.3; //  ( 640/15 = 42)

	// width와 height 크기를 비교
	// 추후 색상 데이터를 보는 식으로 하여 강화
	if (curr_component->label == prev_component->label) {
		if ((abs(curr_component->width - prev_component->width) > diff_component_width) ||
			(abs(curr_component->height - prev_component->height) > diff_component_height)) {
			printf("save fail, Size unContinue %d %d %d!!\n", prev_component->timeTag
				, (abs(curr_component->width - prev_component->width))
				, (abs(curr_component->height - prev_component->height)));
			return false;
		}
	}
	return true;
}

// 색 정보의 연속성을 따져서 저장을 할껀지 말껀지를 판별하는 함수
bool isColorContinue(component *curr_component, component *prev_component) {
	const int tolerance_of_hsv_value = 30;
	const int tolerance_of_rgb_value = 35;
	for (int c = 0; c < 3; c++) {
		// hsv, rgh 영역에서 확인
		if (abs(curr_component->hsv_avarage[c] - prev_component->hsv_avarage[c]) > tolerance_of_hsv_value
			|| abs(curr_component->rgb_avarage[c] - prev_component->rgb_avarage[c]) > tolerance_of_rgb_value) {
			printf("save fail, Color unContinue %d %d %d!!\n", prev_component->timeTag
				, abs(curr_component->hsv_avarage[c] - prev_component->hsv_avarage[c])
				, abs(curr_component->rgb_avarage[c] - prev_component->rgb_avarage[c]));
			return false;
		}
	}
}

// 현재와 이전에 검출한 결과를 비교, true 면 겹칠 수 없음
bool IsComparePrevComponent(component curr_component, component prev_component) {
	return curr_component.left > prev_component.right
		|| curr_component.right < prev_component.left
		|| curr_component.top > prev_component.bottom
		|| curr_component.bottom < prev_component.top;
}

// 합성된 프레임을 가져오는 연산
Mat CMFC_SyntheticDlg::getSyntheticFrame(Queue* segment_queue, Mat bgFrame, segment* m_segmentArray) {
	int *labelMap = new int[bgFrame.cols * bgFrame.rows];	//겹침을 판단하는 용도
	segment temp_segment; //DeQueue한 결과를 받을 segment
	int countOfObj = segment_queue->count;	//큐 인스턴스의 노드 갯수
	synthesisEndFlag = false;

	//큐가 비었는지 확인한다. 비었으면 더 이상 출력 할 것이 없는 것 이므로 종료
	if (IsEmpty(segment_queue)) {
		// 타이머를 죽인 이후에
		KillTimer(SYN_RESULT_TIMER);

		// 합성이 끝났다고 판정하여 플래그를 true로 변경
		synthesisEndFlag = true;

		// 버튼 초기화
		isPlayBtnClicked = false;

		// 동적 해제
		free(labelMap);
		// pause가 눌리지 않고 끝까지 재생되었을 경우에
		if (isPauseBtnClicked == false)
			delete[] m_segmentArray;

		// 빈 프레임 반환
		Mat nullFrame(ROWS, COLS, CV_8UC1);
		for (int i = 0; i < ROWS; i++) {
			for (int j = 0; j < COLS; j++) 
				nullFrame.data[j + i*COLS] = 0;
		}
	//	nullFrame.Mat::setTo(Scalar(0));
		return nullFrame;
	}

	Point* TimeTag_p = new Point[countOfObj];		// 타임태그 위치
	string* TimeTag_s = new string[countOfObj];		// 타임태그 내용
	int countOfShowObj = 0;	//실질적으로 출력할 객체 수

	// 객체들 인덱스 정보를 저장하기 위한 벡터
	vector<int> vectorPreNodeIndex;

	//큐에 있는 객체들의 인덱스 정보들을 벡터에 저장
	for (int k = 0; k < countOfObj; k++) {
		int curr_index = Getqueue_IndexOfSegmentArray(segment_queue);
		temp_segment = Dequeue(segment_queue);
		vectorPreNodeIndex.push_back(curr_index);
		Enqueue(segment_queue, temp_segment, curr_index);
	}

	bool isEnqueueFlag = true;
	// 큐에 들어있는 객체 갯수 만큼 DeQueue. 
	for (int i = 0; i < countOfObj; i++) {
		//dequeue한 객체를 출력한다.
		int curIndex = Getqueue_IndexOfSegmentArray(segment_queue);
		temp_segment = Dequeue(segment_queue);
		bool isCross = false;

		//객체가 이전 객체와 겹치는지 비교, 처음이 아니고 현재 출력할 객체가 timetag의 첫 프레임 일 때
		if ((i != 0 && m_segmentArray[curIndex].first_timeTagFlag)) {
			for (int j = 0; j < i; j++) {
				// 겹침을 확인하고 확인된 객체를 다시 enqueue 연산, 
				if (!IsObjectOverlapingDetector(m_segmentArray[curIndex], m_segmentArray[vectorPreNodeIndex.at(j)])) {
					isCross = true;
					Enqueue(segment_queue, temp_segment, curIndex);
					break;
				}
			}
		}
		// 출력된 객체가 없거나 이전 객체와 겹치지 않는 경우
		if (m_segmentArray[curIndex].printFlag == false && isCross == false) {
			// 출력여부 플래그 참으로 설정
			m_segmentArray[curIndex].printFlag = true;

			// 배경에 객체를 올리기
			bgFrame = printObjOnBG(bgFrame, m_segmentArray[curIndex], labelMap, fileNameNoExtension);

			// 타임태그를 string으로 변환
			int timetagInSec = (m_segmentArray[curIndex].timeTag + videoStartMsec) / 1000;	//영상의 시작시간을 더해준다.
			string timetag = timeConvertor(timetagInSec).str();

			// 타임태그 위치 및 텍스트 정보 저장
			TimeTag_p[countOfShowObj] = Point(m_segmentArray[curIndex].left + 5, m_segmentArray[curIndex].top + 50);
			TimeTag_s[countOfShowObj] = timetag;
			countOfShowObj++;

			// 다음 객체가 검출된 프레임이 이전 프레임과 1 차이가 날 때
			if (m_segmentArray[curIndex].frameCount + 1 == m_segmentArray[curIndex + 1].frameCount) {
				//타임태그가 같고 인덱스가 같은 경우에만 큐에 넣어서 다음에 이어 출력될 수 있도록 한다.(일반적인 경우)
				Enqueue(segment_queue, temp_segment, curIndex + 1);
			}

			//다음 객체가 있는 프레임이 객체가 있는 프레임과 2 이상, 버퍼 이하 만큼 차이 날 때
			else {
				for (int i = 2; i <= MAX_SEGMENT_TEMP_BUFFER + 1; i++) {
					// 세그먼트 카운트의 차이를 비교함
					if (m_segmentArray[curIndex].frameCount + i == m_segmentArray[curIndex + 1].frameCount) {
						// 이전과 타임태그와 인덱스가 모두 같을 때에 다음 인덱스 enqueue시키기
						//	printf(" 2 이상, 버퍼 이하 만큼 차이 %d %d\n", temp_segment.timeTag, temp_segment.frameCount);
						Enqueue(segment_queue, temp_segment, curIndex + 1);
						break;
					}
				} // end for  (int i = 2; i <= MAX_SEGMENT_TEMP_BUFFER + 1 ...
				// 임시 세그먼트 buffer 크기 이상 차이나는 객체들은 출력되지 않도록 설정
			} // end else
		} // end if (isCross == false)
	} // end for (int i = 0; i < countOfObj; i++)

	for (int i = 0; i < countOfShowObj; i++)
		putText(bgFrame, TimeTag_s[i], TimeTag_p[i], FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255), 1.5);

	// 합성 중 메모리 해제
	delete[] TimeTag_p; delete[] TimeTag_s;
	delete[] labelMap;
	vector<int>().swap(vectorPreNodeIndex);
	return bgFrame;
}
// 객체들 끼리 겹침을 판별하는 함수, 하나라도 성립하면 겹치지 않음
bool IsObjectOverlapingDetector(segment m_segment, segment pre_segment) {
	return m_segment.left > pre_segment.right
		|| m_segment.right < pre_segment.left
		|| m_segment.top > pre_segment.bottom
		|| m_segment.bottom < pre_segment.top;
}
// 큐에 넣을 객체를 판별하는 함수
bool IsEnqueueFiltering(segment *segment_array, int cur_index) {
	// 양 끝 filter_object_num 갯수만큼의 객체를 비교하고자 함
	// 1일 경우 앞쪽 뒷쪽 한 개씩만
	const unsigned int filter_object_num = 1;
	const unsigned int filter_object_height = ROWS / 15; //  ( 480/15 = 32)
	const unsigned int filter_object_width = COLS / 15; //  ( 640/15 = 42)

	// 앞의 객체가 없으면 그냥 출력 가능하게 함
	if ((cur_index < filter_object_num && cur_index >= 0)
		|| segment_array[cur_index].first_timeTagFlag == true) {
		return true;
	}
	if ((segment_array[cur_index].timeTag == segment_array[cur_index + 1].timeTag)
		&& (segment_array[cur_index].index == segment_array[cur_index + 1].index)) {
		// 앞 뒤로 일정 범위 이상의 height, width보다 작으면 필터 통과, 출력 가능하게끔 만듦
		for (int i = 1; i <= filter_object_num; i++) {
			bool height_filter = (abs(segment_array[cur_index - i].height - segment_array[cur_index].height) < filter_object_height)
				&& (abs(segment_array[cur_index + i].height - segment_array[cur_index].height) < filter_object_height);
			bool width_filter = (abs(segment_array[cur_index - i].width - segment_array[cur_index].width) < filter_object_width)
				&& (abs(segment_array[cur_index + i].width - segment_array[cur_index].width) < filter_object_width);

			// 필터가 모두 true가 될 경우
			if ((height_filter && width_filter) == true)
				return true;
		}
		// 위의 height, width 필터도 통과 못할 경우는 
		printf("필터 통과 못함 %d %d\n", segment_array[cur_index].timeTag, segment_array[cur_index].frameCount);
		return false;
	}

	else
		return false;
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
	else if (pScrollBar == (CScrollBar*)&m_SliderWMIN)
		SetDlgItemText(IDC_SEG_STRING_VAL_MIN_W, to_string(m_SliderWMIN.GetPos()).c_str());
	else if (pScrollBar == (CScrollBar*)&m_SliderWMAX)
		SetDlgItemText(IDC_SEG_STRING_VAL_MAX_W, to_string(m_SliderWMAX.GetPos()).c_str());
	else if (pScrollBar == (CScrollBar*)&m_SliderHMIN)
		SetDlgItemText(IDC_SEG_STRING_VAL_MIN_H, to_string(m_SliderHMIN.GetPos()).c_str());
	else if (pScrollBar == (CScrollBar*)&m_SliderHMAX)
		SetDlgItemText(IDC_SEG_STRING_VAL_MAX_H, to_string(m_SliderHMAX.GetPos()).c_str());
	else if (pScrollBar == (CScrollBar*)&m_SliderPlayer) {	//플레이어 타임 슬라이더
		//슬라이딩 시 멈춤
		KillTimer(LOGO_TIMER);
		KillTimer(VIDEO_TIMER);
		KillTimer(BIN_VIDEO_TIMER);
		KillTimer(SYN_RESULT_TIMER);

		//한 프레임 송출, 이진 영상 구별 안함.
		capture.set(CV_CAP_PROP_POS_FRAMES, m_SliderPlayer.GetPos());
		Mat temp_frame;
		capture.read(temp_frame);
		DisplayImage(IDC_RESULT_IMAGE, temp_frame, NULL);
		temp_frame = NULL;
		temp_frame.release();


		SetDlgItemText(IDC_STRING_CUR_TIME, timeConvertor((int)(videoLength * m_SliderPlayer.GetPos() / (float)totalFrameCount)).str().c_str());
	}
	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

// Play Button을 눌렀을 경우에 실행되는 콜백
void CMFC_SyntheticDlg::OnClickedBtnPlay()
{
	if (radioChoice == 0 && isPlayBtnClicked == false) {//라디오버튼이 원본영상일 경우 - 그냥 재생
		printf("원본영상 재생 버튼 눌림\n");
		KillTimer(BIN_VIDEO_TIMER);
		KillTimer(SYN_RESULT_TIMER);
		SetTimer(VIDEO_TIMER, 1000 / m_sliderFps.GetPos(), NULL);
		isPlayBtnClicked = true;
		isPauseBtnClicked = false;
	}
	else if (radioChoice == 2 && isPlayBtnClicked == false) {//라디오버튼이 이진영상일 경우 - 이진 및 객체 검출 바운더리가 그려진 영상 재생
		printf("이진영상 재생 버튼 눌림\n");
		KillTimer(SYN_RESULT_TIMER);
		KillTimer(VIDEO_TIMER);
		SetTimer(BIN_VIDEO_TIMER, 1000 / m_sliderFps.GetPos(), NULL);
		isPlayBtnClicked = true;
		isPauseBtnClicked = false;
	}

	else if (radioChoice == 1 && isPlayBtnClicked == false) { //라디오버튼이 합성영상일 경우 - 설정에 따라 합성된 영상 재생
		printf("합성영상 재생 버튼 눌림\n");
		isPlayBtnClicked = true;
		isPauseBtnClicked = false;

		boolean isSynPlayable = checkSegmentation();

		if (isSynPlayable) {
			// BUFFER크기만큼 segment 정보들을 가져오기 위해 메모리 할당
			segment *segmentArray = new segment[BUFFER];

			// segment Text File을 읽어옴
			int segmentCount = readSegmentTxtFile(segmentArray);

			//TimeTag를 Edit box로부터 입력받음
			unsigned int obj1_TimeTag = m_sliderSearchStartTime.GetPos() * 1000;	//검색할 TimeTag1
			unsigned int obj2_TimeTag = m_sliderSearchEndTime.GetPos() * 1000;	//검색할 TimeTag2

			if (obj1_TimeTag >= obj2_TimeTag) {
				AfxMessageBox("Check search time again");
				return;
			}

			if (inputSegmentQueue(obj1_TimeTag, obj2_TimeTag, segmentCount, segmentArray)) {
				// free(m_segmentArray);
			}
			m_segmentArray = segmentArray;

			//실행중인 타이머 종료
			KillTimer(BIN_VIDEO_TIMER);
			KillTimer(VIDEO_TIMER);

			//타이머 시작	params = timerID, ms, callback함수 명(NULL이면 OnTimer)
			SetTimer(SYN_RESULT_TIMER, 1000 / m_sliderFps.GetPos(), NULL);
		}
		else { //실행 못하는 경우 segmentation을 진행하라고 출력
			AfxMessageBox("You can't play without segmentation results");
		}
	}
	else {}
}

int readSegmentTxtFile(segment* segmentArray) {
	// 텍스트파일 읽을 때 사용할 buffer 정의
	char *txtBuffer = new char[100];

	// 읽어들일 텍스트 파일의 경로를 불러옴
	string path = "./" + getTextFilePath(fileNameNoExtension);
	int segmentCount = 0;
	FILE *fp;
	if (fp = fopen(path.c_str(), "r")){

		fseek(fp, 0, SEEK_SET);	//포인터 처음으로 이동
		fgets(txtBuffer, 99, fp);
		sscanf(txtBuffer, "%d", &videoStartMsec);	//텍스트 파일 첫줄에 명시된 실제 영상 시작 시간 받아옴
		fflush(stdin);

		// frameInfo.txt 파일에서 데이터를 추출 하여 segment array 초기화
		while (!feof(fp)) {
			fgets(txtBuffer, 99, fp);

			// txt파일에 있는 프레임 데이터들 segmentArray 버퍼로 복사
			sscanf(txtBuffer, "%d_%d_%d_%d %d %d %d %d %d %d",
				&segmentArray[segmentCount].timeTag, &segmentArray[segmentCount].msec,
				&segmentArray[segmentCount].frameCount, &segmentArray[segmentCount].index,
				&segmentArray[segmentCount].left, &segmentArray[segmentCount].top,
				&segmentArray[segmentCount].right, &segmentArray[segmentCount].bottom,
				&segmentArray[segmentCount].width, &segmentArray[segmentCount].height);
			fflush(stdin);
			// filename 저장
			segmentArray[segmentCount].fileName
				.append(to_string(segmentArray[segmentCount].timeTag)).append("_")
				.append(to_string(segmentArray[segmentCount].msec)).append("_")
				.append(to_string(segmentArray[segmentCount].frameCount)).append("_")
				.append(to_string(segmentArray[segmentCount].index)).append(".jpg");

			// m_segmentArray의 인덱스 증가
			segmentCount++;
		}

		// 버블 정렬 사용하여 m_segmentArray를 TimeTag순으로 정렬
		segment *tmp_segment = new segment; // 임시 segment 동적생성, 메모리 해제에 용의하게 하기
		for (int i = 0; i < segmentCount; i++) {
			for (int j = 0; j < segmentCount - 1; j++) {
				if (segmentArray[j].timeTag > segmentArray[j + 1].timeTag) {
					// m_segmentArray[segmentCount]와 m_segmentArray[segmentCount + 1]의 교체
					*tmp_segment = segmentArray[j + 1];
					segmentArray[j + 1] = segmentArray[j];
					segmentArray[j] = *tmp_segment;
				}
			}
		}

		// 임시 버퍼들의 메모리 해제
		delete tmp_segment;
		delete[] txtBuffer;

		// 텍스트 파일 닫기
		fclose(fp);
	}
	else{
		perror("텍스트 파일 읽기 오류");
	}

	return segmentCount;
}

bool CMFC_SyntheticDlg::inputSegmentQueue(int obj1_TimeTag, int obj2_TimeTag, int segmentCount, segment* segmentArray) {
	//큐 초기화
	InitQueue(&segment_queue);

	//출력할 객체를 큐에 삽입하는 부분
	for (int i = 0; i < segmentCount; i++) {
		// start timetag와 end timetag 사이면 enqueue
		// 아직 찾지 못했고 일치하는 타임태그를 찾았을 경우
		if (segmentArray[i].timeTag >= obj1_TimeTag && segmentArray[i].timeTag <= obj2_TimeTag) {
			if (segmentArray[i].timeTag == segmentArray[i].msec && isDirectionAndColorMatch(segmentArray[i])) {
				//출력해야할 객체의 첫 프레임의 타임태그와 위치를 큐에 삽입
				segmentArray[i].first_timeTagFlag = true;
				Enqueue(&segment_queue, segmentArray[i], i);
			}
		}
		//탐색 중, obj2_TimeTag을 넘으면 진행 완료
		else if (segmentArray[i].timeTag > obj2_TimeTag) {
			return true;
		}
	}
	return false;
}
// segmentation을 할 떄에 입력받는 수의 범위를 한정해주는 함수
bool segmentationTimeInputException(CString str_h, CString str_m) {
	// 시 :: 1~24, 분 :: 1~60
	if ((atoi(str_h) > 0 && atoi(str_h) <= 24)
		&& (atoi(str_m) > 0 && atoi(str_m) <= 60)) {
		return true;
	}

	else if ((str_h == "0" && atoi(str_h) == 0)
		|| (str_m == "0" && atoi(str_m) == 0))
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
		s << "0" << hour << ":";
	else
		s << hour << ":";

	if ((t % 3600) / 60 < 10)
		s << "0" << min << ":";
	else
		s << min << ":";

	if (t % 60 < 10)
		s << "0" << sec;
	else
		s << sec;

	return s;
}


//load 버튼을 누르면 발생하는 콜백
void CMFC_SyntheticDlg::OnBnClickedBtnMenuLoad() {
	SetTimer(LOGO_TIMER, 1, NULL);
	//실행시 비디오 파일 불러옴
	if (loadFile(1) == 0){
		//Slider Control 범위 지정
		setSliderRange(videoLength, COLS, ROWS, 100);

		//UI 업데이트, control에 default 값 할당
		updateUI(videoLength, COLS, ROWS, fps);

		// // Play, Pause버튼 상태 초기화
		isPlayBtnClicked = false;
		isPauseBtnClicked = true;
		isAlreadyBGMade = false;

		// 라디오 버튼 초기화
		CheckRadioButton(IDC_RADIO_PLAY1, IDC_RADIO_PLAY3, IDC_RADIO_PLAY1);
		radioChoice = 0; preRadioChoice = 0; //라디오 버튼의 default는 맨 처음 버튼임
		mButtonSynSave.EnableWindow(false);
	}
	return;
}

void CMFC_SyntheticDlg::SetRadioStatus(UINT value) {
	UpdateData(TRUE);

	if (radioChoice != mRadioPlay) {	//현재 위치와 새로 누른 위치가 같을 경우 무시
		mButtonSynSave.EnableWindow(false);	//저장버튼 막음
		//슬라이더 활성 및 비활성
		m_SliderPlayer.EnableWindow(TRUE);
		m_sliderSearchStartTime.EnableWindow(FALSE);
		m_sliderSearchEndTime.EnableWindow(FALSE);
		//UI 플래그 초기화
		isPlayBtnClicked = false;
		isPauseBtnClicked = true;
		capture.set(CV_CAP_PROP_POS_FRAMES, 0);
		switch (mRadioPlay) {
		case 0:
			radioChoice = 0;
			SetTimer(LOGO_TIMER, 1, NULL);
			printf("원본 영상 라디오 버튼 선택됨\n");
			break;
		case 1:
			radioChoice = 1;
			SetTimer(LOGO_TIMER, 1, NULL);
			printf("합성 영상 라디오 버튼 선택됨\n");
			m_SliderPlayer.EnableWindow(FALSE);	//합성영상일 경우 비활성화
			m_sliderSearchStartTime.EnableWindow(TRUE);	//활성화
			m_sliderSearchEndTime.EnableWindow(TRUE);
			mButtonSynSave.EnableWindow(true);//저장버튼 활성화
			background_loadedFromFile = imread(getColorBackgroundFilePath(fileNameNoExtension));//합성 영상을 출력할 때 바탕이 될 프레임. 영상합성 라디오 버튼 클릭 시 자동으로 파일로부터 로드 됨
			printf("합성 기본 배경 로드 완료\n");
			break;
		case 2:
			radioChoice = 2;
			SetTimer(LOGO_TIMER, 1, NULL);
			printf("이진 영상 라디오 버튼 선택됨\n");
			break;
		default:
			radioChoice = 0;
			printf("원본 영상 라디오 버튼 선택됨\n");
			break;
		}
	}
}

//일시 정지 버튼
void CMFC_SyntheticDlg::OnBnClickedBtnPause()
{
	if (isPauseBtnClicked == false) {
		// 합성 영상 재생 중에 해당 버튼이 눌렸을 때에 segment 배열의 메모리 해제를 위한 코드
		if (radioChoice == 1 && synthesisEndFlag == false)
			delete[] m_segmentArray;

		isPlayBtnClicked = false;
		isPauseBtnClicked = true;

		KillTimer(LOGO_TIMER);
		KillTimer(VIDEO_TIMER);
		KillTimer(BIN_VIDEO_TIMER);
		KillTimer(SYN_RESULT_TIMER);
	}
}

//정지 버튼
void CMFC_SyntheticDlg::OnBnClickedBtnStop()
{
	// 합성 영상 재생 중에 해당 버튼이 눌렸을 때에 segment 배열의 메모리 해제를 위한 코드
	if (synthesisEndFlag == false)
	if (m_segmentArray != nullptr)
		delete[] m_segmentArray;

	printf("정지 버튼 눌림\n");

	isPlayBtnClicked = false;

	KillTimer(VIDEO_TIMER);
	KillTimer(BIN_VIDEO_TIMER);
	KillTimer(SYN_RESULT_TIMER);

	SetTimer(LOGO_TIMER, 1, NULL);

	capture.set(CV_CAP_PROP_POS_FRAMES, 0);
}

void CMFC_SyntheticDlg::OnBnClickedBtnRewind()
{
	printf("리와인드 버튼 눌림\n");

	capture.set(CV_CAP_PROP_POS_FRAMES, 0);
}

//Segmentation이 수행 됬는지 판별하는 함수, true = 수행됐음
bool CMFC_SyntheticDlg::checkSegmentation()
{
	string path = "./";
	path.append(getTextFilePath(fileNameNoExtension));

	FILE *txtFile = fopen(path.c_str(), "r");

	if (txtFile) {	//파일을 제대로 불러왔을 경우
		//포인터 끝으로 이동하여 파일 크기 측정
		fseek(txtFile, 0, SEEK_END);
		if (ftell(txtFile) != 0) {	//텍스트 파일이 크기가 0 이 아닐 경우 true 반환
			fclose(txtFile);
			return true;
		}
		else { //파일 크기가 0 일 경우 false 반환
			printf("Empty txt file\n");
			fclose(txtFile);
			return false;
		}
	}
	else {	//파일을 불러오지 못 할 경우 false 반환
		printf("Can't find txt file\n");
		return false;
	}
}

void CMFC_SyntheticDlg::backgroundInit(string videoFilePath) {
	if (!isGrayBackgroundExists(getBackgroundFilePath(fileNameNoExtension))){
		CProgressDlg ProgressDlg(this);                // this 를 사용하여 부모를 지정.
		ProgressDlg.CenterWindow();
		ProgressDlg.mode = 0;
		ProgressDlg.videoFilePath = videoFilePath;
		ProgressDlg.ROWS = ROWS;
		ProgressDlg.COLS = COLS;
		ProgressDlg.FRAMES_FOR_MAKE_BACKGROUND = FRAMES_FOR_MAKE_BACKGROUND;
		ProgressDlg.fileNameNoExtension = fileNameNoExtension;
		ProgressDlg.DoModal();
	}
	else{
		printf("Init skip : 배경이 존재함\n");
	}
	return;
}

void CMFC_SyntheticDlg::layoutInit() {
	//(http://gandus.tistory.com/530)
	//현재 dialog 크기 얻어옴
	int dialogWidth = m_rectCurHist.right;
	int dialogHeight = m_rectCurHist.bottom - 50;	//작업표시줄 크기 빼줌
	int padding = 10;
	SetWindowPos(&wndTop, 0, 0, dialogWidth, dialogHeight, SWP_NOREDRAW);	//다이얼로그 크기 조정

	//group box - MENU
	CWnd *pGroupMenu = GetDlgItem(IDC_GROUP_MENU);
	CWnd *pStringFileName = GetDlgItem(IDC_MENU_STRING_FILE_NAME);
	CButton *pButtonLoad = (CButton *)GetDlgItem(IDC_BTN_MENU_LOAD);
	CWnd *pRadioBtn1 = GetDlgItem(IDC_RADIO_PLAY1);
	CWnd *pRadioBtn2 = GetDlgItem(IDC_RADIO_PLAY2);
	CWnd *pRadioBtn3 = GetDlgItem(IDC_RADIO_PLAY3);
	int box_MenuX = padding;
	int box_MenuY = padding;
	int box_MenuWidth = (dialogWidth - 3 * padding)*0.2;
	int box_MenuHeight = ((dialogHeight - 3 * padding)*0.8 - padding)*0.3;

	pGroupMenu->MoveWindow(box_MenuX, box_MenuY, box_MenuWidth, box_MenuHeight, TRUE);
	pStringFileName->MoveWindow(box_MenuX + padding, box_MenuY + 2 * padding, 230, 50, TRUE);
	pButtonLoad->MoveWindow(box_MenuX + box_MenuWidth - padding - 100, box_MenuY + 3 * padding + 20, 100, 20, TRUE);
	pRadioBtn1->MoveWindow(box_MenuX + padding, box_MenuY + 4 * padding + 40, 100, 20, TRUE);
	pRadioBtn3->MoveWindow(box_MenuX + padding + 150, box_MenuY + 4 * padding + 40, 100, 20, TRUE);
	pRadioBtn2->MoveWindow(box_MenuX + padding, box_MenuY + 5 * padding + 60, 100, 20, TRUE);

	//Picture Control
	CWnd *pResultImage = GetDlgItem(IDC_RESULT_IMAGE);
	CButton *pButtonPlay = (CButton  *)GetDlgItem(IDC_BTN_PLAY);
	cImage.Load("res\\play.bmp");
	pButtonPlay->SetBitmap(cImage);
	CButton  *pButtonPause = (CButton  *)GetDlgItem(IDC_BTN_PAUSE);
	cImage.Load("res\\pause.bmp");
	pButtonPause->SetBitmap(cImage);
	CButton  *pButtonStop = (CButton  *)GetDlgItem(IDC_BTN_STOP);
	cImage.Load("res\\stop.bmp");
	pButtonStop->SetBitmap(cImage);
	CButton  *pButtonRewind = (CButton  *)GetDlgItem(IDC_BTN_REWIND);
	cImage.Load("res\\rewind.bmp");
	pButtonRewind->SetBitmap(cImage);
	CWnd *pStringCurTimeSlider = GetDlgItem(IDC_STRING_CUR_TIME);
	CWnd *pStringTotalTimeSlider = GetDlgItem(IDC_STRING_TOTAL_TIME);
	int pictureContorlX = 2 * padding + box_MenuWidth;
	int pictureContorlY = padding;
	int pictureContorlWidth = (dialogWidth - 3 * padding) - box_MenuWidth - 15;
	int playerSliderWidth = pictureContorlWidth;
	int playerSliderHeight = 40;
	int pictureContorlHeight = (dialogHeight - 3 * padding)*0.8 - 40 - playerSliderHeight - padding;
	pResultImage->MoveWindow(pictureContorlX, pictureContorlY, pictureContorlWidth, pictureContorlHeight, TRUE);
	m_SliderPlayer.MoveWindow(pictureContorlX, pictureContorlY + pictureContorlHeight + 10, playerSliderWidth, playerSliderHeight, TRUE);
	pStringCurTimeSlider->MoveWindow(pictureContorlX + padding, pictureContorlY + pictureContorlHeight + 10 + playerSliderHeight, 230, 20, TRUE);
	pStringTotalTimeSlider->MoveWindow(pictureContorlX + pictureContorlWidth - 80, pictureContorlY + pictureContorlHeight + 10 + playerSliderHeight, 230, 20, TRUE);
	pButtonRewind->MoveWindow(pictureContorlX + pictureContorlWidth*0.5 - 95, pictureContorlY + pictureContorlHeight + 10 + playerSliderHeight, 40, 40, TRUE);
	pButtonPlay->MoveWindow(pictureContorlX + pictureContorlWidth*0.5 - 45, pictureContorlY + pictureContorlHeight + 10 + playerSliderHeight, 40, 40, TRUE);
	pButtonPause->MoveWindow(pictureContorlX + pictureContorlWidth*0.5 + 5, pictureContorlY + pictureContorlHeight + 10 + playerSliderHeight, 40, 40, TRUE);
	pButtonStop->MoveWindow(pictureContorlX + pictureContorlWidth*0.5 + 55, pictureContorlY + pictureContorlHeight + 10 + playerSliderHeight, 40, 40, TRUE);

	//group box - segmetation
	CWnd *pGroupSegmentation = GetDlgItem(IDC_GROUP_SEG);
	CWnd *pStringStartTime = GetDlgItem(IDC_SEG_STRING_VIDEO_START_TIME);
	CWnd *pStringColon = GetDlgItem(IDC_SEG_STRING_COLON);
	m_pEditBoxStartHour = (CEdit *)GetDlgItem(IDC_SEG_EDITBOX_START_HOUR);
	m_pEditBoxStartMinute = (CEdit *)GetDlgItem(IDC_SEG_EDITBOX_START_MINUTE);
	CWnd *pGroupSegWidth = GetDlgItem(IDC_GROUP_SEG_WIDTH);
	CWnd *pStringWMIN = GetDlgItem(IDC_SEG_STRING_MIN_W);
	CWnd *pStringWMAX = GetDlgItem(IDC_SEG_STRING_MAX_W);
	CWnd *pStringValWMIN = GetDlgItem(IDC_SEG_STRING_VAL_MIN_W);
	CWnd *pStringValWMAX = GetDlgItem(IDC_SEG_STRING_VAL_MAX_W);
	CWnd *pSegSliderWMIN = GetDlgItem(IDC_SEG_SLIDER_WMIN);
	CWnd *pSegSliderWMAX = GetDlgItem(IDC_SEG_SLIDER_WMAX);
	CWnd *pGroupSegHeight = GetDlgItem(IDC_GROUP_SEG_HEIGHT);
	CWnd *pStringHMIN = GetDlgItem(IDC_SEG_STRING_MIN_H);
	CWnd *pStringHMAX = GetDlgItem(IDC_SEG_STRING_MAX_H);
	CWnd *pStringValHMIN = GetDlgItem(IDC_SEG_STRING_VAL_MIN_H);
	CWnd *pStringValHMAX = GetDlgItem(IDC_SEG_STRING_VAL_MAX_H);
	CWnd *pSegSliderHMIN = GetDlgItem(IDC_SEG_SLIDER_HMIN);
	CWnd *pSegSliderHMAX = GetDlgItem(IDC_SEG_SLIDER_HMAX);
	CButton *pButtonSegmentation = (CButton *)GetDlgItem(IDC_BTN_SEG_SEGMENTATION);
	int box_segmentationX = padding;
	int box_segmentationY = 2 * padding + box_MenuHeight;
	int box_segmentationWidth = box_MenuWidth;
	int box_segmentationHeight = ((dialogHeight - 3 * padding)*0.8 - padding) - box_MenuHeight;
	pGroupSegmentation->MoveWindow(box_segmentationX, box_segmentationY, box_segmentationWidth, box_segmentationHeight, TRUE);
	pStringStartTime->MoveWindow(box_segmentationX + padding, box_segmentationY + 2 * padding, 230, 20, TRUE);
	m_pEditBoxStartHour->MoveWindow(box_segmentationX + padding + box_segmentationWidth*0.5, box_segmentationY + 3 * padding + 20, 20, 20, TRUE);
	pStringColon->MoveWindow(box_segmentationX + padding + 25 + box_segmentationWidth*0.5, box_segmentationY + 3 * padding + 20, 20, 20, TRUE);
	m_pEditBoxStartMinute->MoveWindow(box_segmentationX + padding + 35 + box_segmentationWidth*0.5, box_segmentationY + 3 * padding + 20, 20, 20, TRUE);
	pGroupSegWidth->MoveWindow(box_segmentationX + padding, box_segmentationY + 4 * padding + 40, box_segmentationWidth - 2 * padding, 80, TRUE);
	pStringWMIN->MoveWindow(box_segmentationX + 2 * padding, box_segmentationY + 6 * padding + 40, 40, 20, TRUE);
	m_SliderWMIN.MoveWindow(box_segmentationX + 3 * padding + 40, box_segmentationY + 6 * padding + 40, box_segmentationWidth - 6 * padding - 60, 20, TRUE);
	pStringValWMIN->MoveWindow(box_segmentationX + box_segmentationWidth - 2 * padding - 20, box_segmentationY + 6 * padding + 40, 20, 20, TRUE);
	pStringWMAX->MoveWindow(box_segmentationX + 2 * padding, box_segmentationY + 7 * padding + 60, 40, 20, TRUE);
	m_SliderWMAX.MoveWindow(box_segmentationX + 3 * padding + 40, box_segmentationY + 7 * padding + 60, box_segmentationWidth - 6 * padding - 60, 20, TRUE);
	pStringValWMAX->MoveWindow(box_segmentationX + box_segmentationWidth - 2 * padding - 20, box_segmentationY + 7 * padding + 60, 20, 20, TRUE);
	pGroupSegHeight->MoveWindow(box_segmentationX + padding, box_segmentationY + 5 * padding + 120, box_segmentationWidth - 2 * padding, 80, TRUE);
	pStringHMIN->MoveWindow(box_segmentationX + 2 * padding, box_segmentationY + 7 * padding + 120, 40, 20, TRUE);
	m_SliderHMIN.MoveWindow(box_segmentationX + 3 * padding + 40, box_segmentationY + 7 * padding + 120, box_segmentationWidth - 6 * padding - 60, 20, TRUE);
	pStringValHMIN->MoveWindow(box_segmentationX + box_segmentationWidth - 2 * padding - 20, box_segmentationY + 7 * padding + 120, 20, 20, TRUE);
	pStringHMAX->MoveWindow(box_segmentationX + 2 * padding, box_segmentationY + 8 * padding + 140, 40, 20, TRUE);
	m_SliderHMAX.MoveWindow(box_segmentationX + 3 * padding + 40, box_segmentationY + 8 * padding + 140, box_segmentationWidth - 6 * padding - 60, 20, TRUE);
	pStringValHMAX->MoveWindow(box_segmentationX + box_segmentationWidth - 2 * padding - 20, box_segmentationY + 8 * padding + 140, 20, 20, TRUE);
	pButtonSegmentation->MoveWindow(box_segmentationX + box_segmentationWidth - padding - 100, box_segmentationY + box_segmentationHeight - 30, 100, 20, TRUE);


	//group box - Play Settings
	CWnd *pGroupSynthetic = GetDlgItem(IDC_GROUP_PLAY_SETTINGS);
	CWnd *pStringSearchStartTime = GetDlgItem(IDC_STRING_SEARCH_START_TIME);
	CWnd *pStringSearchEndTime = GetDlgItem(IDC_STRING_SEARCH_END_TIME);
	CWnd *pStringFps = GetDlgItem(IDC_STRING_FPS);
	CWnd *pStringSearchStartTimeSlider = GetDlgItem(IDC_STRING_SEARCH_START_TIME_SLIDER);
	CWnd *pStringSearchEndTimeSlider = GetDlgItem(IDC_STRING_SEARCH_END_TIME_SLIDER);
	CWnd *pStringFpsSlider = GetDlgItem(IDC_STRING_FPS_SLIDER);
	CWnd *pStringDirection = GetDlgItem(IDC_STRING_DIRECTION);
	CWnd *pStringDirectionStart = GetDlgItem(IDC_STRING_DIRECTION_START);
	CWnd *pStringDirectionEnd = GetDlgItem(IDC_STRING_DIRECTION_END);
	CWnd *pStringColor = GetDlgItem(IDC_STRING_COLOR);
	CButton *pButtonColorR = (CButton *)GetDlgItem(IDC_COLOR_RED);
	CButton *pButtonColorG = (CButton *)GetDlgItem(IDC_COLOR_GREEN);
	CButton *pButtonColorB = (CButton *)GetDlgItem(IDC_COLOR_BLUE);
	CButton *pButtonColorO = (CButton *)GetDlgItem(IDC_COLOR_ORANGE);
	CButton *pButtonColorY = (CButton *)GetDlgItem(IDC_COLOR_YELLOW);
	CButton *pButtonColorM = (CButton *)GetDlgItem(IDC_COLOR_MAGENTA);
	CButton *pButtonColorBLACK = (CButton *)GetDlgItem(IDC_COLOR_BLACK);
	CButton *pButtonColorWHITE = (CButton *)GetDlgItem(IDC_COLOR_WHITE);
	CButton *pButtonColorGRAY = (CButton *)GetDlgItem(IDC_COLOR_GRAY);
	CWnd *pCheckBoxAll = GetDlgItem(IDC_CHECK_ALL);
	CWnd *pCheckBoxR = GetDlgItem(IDC_CHECK_RED);
	CWnd *pCheckBoxG = GetDlgItem(IDC_CHECK_GREEN);
	CWnd *pCheckBoxB = GetDlgItem(IDC_CHECK_BLUE);
	CWnd *pCheckBoxO = GetDlgItem(IDC_CHECK_ORANGE);
	CWnd *pCheckBoxY = GetDlgItem(IDC_CHECK_YELLOW);
	CWnd *pCheckBoxM = GetDlgItem(IDC_CHECK_MAGENTA);
	CWnd *pCheckBoxBLACK = GetDlgItem(IDC_CHECK_BLACK);
	CWnd *pCheckBoxWHITE = GetDlgItem(IDC_CHECK_WHITE);
	CWnd *pCheckBoxGRAY = GetDlgItem(IDC_CHECK_GRAY);

	int box_syntheticX = padding;
	int box_syntheticY = box_segmentationY + box_segmentationHeight + padding;
	int box_syntheticWidth = dialogWidth - 3 * padding;
	int box_syntheticHeight = (dialogHeight - 3 * padding)*0.2 - 40;
	pGroupSynthetic->MoveWindow(box_syntheticX, box_syntheticY, box_syntheticWidth, box_syntheticHeight, TRUE);
	pStringSearchStartTime->MoveWindow(box_syntheticX + padding, box_syntheticY + box_syntheticHeight*0.3, 100, 20, TRUE);
	m_sliderSearchStartTime.MoveWindow(box_syntheticX + padding, box_syntheticY + box_syntheticHeight*0.3 + 20 + padding, 140, 20, TRUE);
	pStringSearchStartTimeSlider->MoveWindow(box_syntheticX + padding + 40, box_syntheticY + box_syntheticHeight*0.3 + 40 + padding * 2, 140, 20, TRUE);

	pStringSearchEndTime->MoveWindow(box_syntheticX + padding + 150, box_syntheticY + box_syntheticHeight*0.3, 100, 20, TRUE);
	m_sliderSearchEndTime.MoveWindow(box_syntheticX + padding + 150, box_syntheticY + box_syntheticHeight*0.3 + 20 + padding, 140, 20, TRUE);
	pStringSearchEndTimeSlider->MoveWindow(box_syntheticX + padding + 40 + 150, box_syntheticY + box_syntheticHeight*0.3 + 40 + padding * 2, 140, 20, TRUE);

	pStringFps->MoveWindow(box_syntheticX + padding + 300, box_syntheticY + box_syntheticHeight*0.3, 100, 20, TRUE);
	m_sliderFps.MoveWindow(box_syntheticX + padding + 300, box_syntheticY + box_syntheticHeight*0.3 + 20 + padding, 140, 20, TRUE);
	pStringFpsSlider->MoveWindow(box_syntheticX + padding + 60 + 300, box_syntheticY + box_syntheticHeight*0.3 + 40 + padding * 2, 30, 20, TRUE);


	pStringDirection->MoveWindow(box_syntheticX + padding + 450, box_syntheticY + box_syntheticHeight*0.2, 100, 20, TRUE);
	pStringDirectionStart->MoveWindow(box_syntheticX + padding + 470, box_syntheticY + box_syntheticHeight*0.4, 30, 20, TRUE);
	mComboStart.MoveWindow(box_syntheticX + padding + 520, box_syntheticY + box_syntheticHeight*0.4, 100, 20, TRUE);
	pStringDirectionEnd->MoveWindow(box_syntheticX + padding + 470, box_syntheticY + box_syntheticHeight*0.6, 30, 20, TRUE);
	mComboEnd.MoveWindow(box_syntheticX + padding + 520, box_syntheticY + box_syntheticHeight*0.6, 100, 20, TRUE);

	pStringColor->MoveWindow(box_syntheticX + padding + 680, box_syntheticY + box_syntheticHeight*0.2, 100, 20, TRUE);
	pCheckBoxAll->MoveWindow(box_syntheticX + padding + 710, box_syntheticY + box_syntheticHeight*0.2, 30, 20, TRUE);
	pButtonColorR->MoveWindow(box_syntheticX + padding + 680, box_syntheticY + box_syntheticHeight*0.35, 30, 20, TRUE);
	pCheckBoxR->MoveWindow(box_syntheticX + padding + 730, box_syntheticY + box_syntheticHeight*0.35, 30, 20, TRUE);
	pButtonColorO->MoveWindow(box_syntheticX + padding + 680, box_syntheticY + box_syntheticHeight*0.5, 30, 20, TRUE);
	pCheckBoxO->MoveWindow(box_syntheticX + padding + 730, box_syntheticY + box_syntheticHeight*0.5, 30, 20, TRUE);
	pButtonColorWHITE->MoveWindow(box_syntheticX + padding + 680, box_syntheticY + box_syntheticHeight*0.65, 30, 20, TRUE);
	pCheckBoxWHITE->MoveWindow(box_syntheticX + padding + 730, box_syntheticY + box_syntheticHeight*0.65, 30, 20, TRUE);
	pButtonColorG->MoveWindow(box_syntheticX + padding + 780, box_syntheticY + box_syntheticHeight*0.35, 30, 20, TRUE);
	pCheckBoxG->MoveWindow(box_syntheticX + padding + 830, box_syntheticY + box_syntheticHeight*0.35, 30, 20, TRUE);
	pButtonColorY->MoveWindow(box_syntheticX + padding + 780, box_syntheticY + box_syntheticHeight*0.5, 30, 20, TRUE);
	pCheckBoxY->MoveWindow(box_syntheticX + padding + 830, box_syntheticY + box_syntheticHeight*0.5, 30, 20, TRUE);
	pButtonColorGRAY->MoveWindow(box_syntheticX + padding + 780, box_syntheticY + box_syntheticHeight*0.65, 30, 20, TRUE);
	pCheckBoxGRAY->MoveWindow(box_syntheticX + padding + 830, box_syntheticY + box_syntheticHeight*0.65, 30, 20, TRUE);
	pButtonColorB->MoveWindow(box_syntheticX + padding + 880, box_syntheticY + box_syntheticHeight*0.35, 30, 20, TRUE);
	pCheckBoxB->MoveWindow(box_syntheticX + padding + 920, box_syntheticY + box_syntheticHeight*0.35, 30, 20, TRUE);
	pButtonColorM->MoveWindow(box_syntheticX + padding + 880, box_syntheticY + box_syntheticHeight*0.5, 30, 20, TRUE);
	pCheckBoxM->MoveWindow(box_syntheticX + padding + 920, box_syntheticY + box_syntheticHeight*0.5, 30, 20, TRUE);
	pButtonColorBLACK->MoveWindow(box_syntheticX + padding + 880, box_syntheticY + box_syntheticHeight*0.65, 30, 20, TRUE);
	pCheckBoxBLACK->MoveWindow(box_syntheticX + padding + 920, box_syntheticY + box_syntheticHeight*0.65, 30, 20, TRUE);

	mButtonSynSave.MoveWindow(box_syntheticX + box_syntheticWidth - 110, box_syntheticY + box_syntheticHeight*0.8, 100, 20, TRUE);
}

//Slider Control의 범위 지정
void CMFC_SyntheticDlg::setSliderRange(int video_length, int video_cols, int video_rows, int MAX_Fps) {
	//segmentation slider
	m_SliderWMIN.SetRange(0, video_cols);
	m_SliderWMAX.SetRange(0, video_cols);
	m_SliderHMIN.SetRange(0, video_rows);
	m_SliderHMAX.SetRange(0, video_rows);
	m_SliderWMIN.SetPageSize(1);	//슬라이더 몸통 클릭 시 이동하는 눈금 수
	m_SliderWMAX.SetPageSize(1);
	m_SliderHMIN.SetPageSize(1);
	m_SliderHMAX.SetPageSize(1);

	//play settings slider
	m_sliderSearchStartTime.SetRange(0, video_length);
	m_sliderSearchEndTime.SetRange(0, video_length);
	m_sliderFps.SetRange(1, MAX_Fps);
	m_sliderSearchStartTime.SetPageSize(1);
	m_sliderSearchEndTime.SetPageSize(1);
	m_sliderFps.SetPageSize(1);

	//비디오 플레이어 슬라이더
	m_SliderPlayer.SetRange(0, totalFrameCount);
	m_SliderPlayer.SetPageSize(1);
}

void CMFC_SyntheticDlg::updateUI(int video_length, int video_cols, int video_rows, int MAX_Fps) {
	//edit box default
	m_pEditBoxStartHour->SetWindowTextA("0");
	m_pEditBoxStartMinute->SetWindowTextA("0");

	// time slider default
	SetDlgItemText(IDC_STRING_SEARCH_START_TIME_SLIDER, _T("00 : 00 : 00"));
	SetDlgItemText(IDC_STRING_SEARCH_END_TIME_SLIDER, _T("00 : 00 : 00"));
	SetDlgItemText(IDC_STRING_FPS_SLIDER, to_string(MAX_Fps).c_str());
	m_sliderSearchStartTime.SetPos(0);
	m_sliderSearchEndTime.SetPos(0);
	m_sliderFps.SetPos(MAX_Fps);

	//player slider default
	SetDlgItemText(IDC_STRING_CUR_TIME, _T("00 : 00 : 00"));
	SetDlgItemText(IDC_STRING_TOTAL_TIME, _T(timeConvertor(video_length).str().c_str()));

	// detection slider text
	SetDlgItemText(IDC_SEG_STRING_VAL_MIN_W, _T(to_string(video_cols / 5).c_str()));
	SetDlgItemText(IDC_SEG_STRING_VAL_MAX_W, _T(to_string(video_cols / 2).c_str()));
	SetDlgItemText(IDC_SEG_STRING_VAL_MIN_H, _T(to_string(video_rows / 5).c_str()));
	SetDlgItemText(IDC_SEG_STRING_VAL_MAX_H, _T(to_string(video_rows / 2).c_str()));

	// detection slider default position
	m_SliderWMIN.SetPos(video_cols / 5);
	m_SliderWMAX.SetPos(video_cols / 2);
	m_SliderHMIN.SetPos(video_rows / 5);
	m_SliderHMAX.SetPos(video_rows / 2);

	//combo box
	mComboStart.AddString(_T(ALL));
	mComboStart.AddString(_T(LEFT));
	mComboStart.AddString(_T(RIGHT));
	mComboStart.AddString(_T(ABOVE));
	mComboStart.AddString(_T(BELOW));
	mComboStart.AddString(_T(CENTER));
	mComboStart.AddString(_T(LEFTABOVE));
	mComboStart.AddString(_T(RIGHTABOVE));
	mComboStart.AddString(_T(LEFTBELOW));
	mComboStart.AddString(_T(RIGHTBELOW));
	mComboStart.SetCurSel(0);	//첫 인덱스를 가리킴

	mComboEnd.AddString(_T(ALL));
	mComboEnd.AddString(_T(LEFT));
	mComboEnd.AddString(_T(RIGHT));
	mComboEnd.AddString(_T(ABOVE));
	mComboEnd.AddString(_T(BELOW));
	mComboEnd.AddString(_T(CENTER));
	mComboEnd.AddString(_T(LEFTABOVE));
	mComboEnd.AddString(_T(RIGHTABOVE));
	mComboEnd.AddString(_T(LEFTBELOW));
	mComboEnd.AddString(_T(RIGHTBELOW));
	mComboEnd.SetCurSel(0);	//첫 인덱스를 가리킴

	//컬러 체크박스 초기화
	CheckDlgButton(IDC_CHECK_ALL, TRUE);
	CheckDlgButton(IDC_CHECK_RED, TRUE);
	CheckDlgButton(IDC_CHECK_GREEN, TRUE);
	CheckDlgButton(IDC_CHECK_BLUE, TRUE);
	CheckDlgButton(IDC_CHECK_ORANGE, TRUE);
	CheckDlgButton(IDC_CHECK_YELLOW, TRUE);
	CheckDlgButton(IDC_CHECK_MAGENTA, TRUE);
	CheckDlgButton(IDC_CHECK_BLACK, TRUE);
	CheckDlgButton(IDC_CHECK_GRAY, TRUE);
	CheckDlgButton(IDC_CHECK_WHITE, TRUE);
}

//동영상 플레이어 슬라이더를 마우스로 잡은 뒤, 놓았을 때 발생하는 콜백
void CMFC_SyntheticDlg::OnReleasedcaptureSliderPlayer(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	int releasedPoint = m_SliderPlayer.GetPos();
	capture.set(CV_CAP_PROP_POS_FRAMES, releasedPoint);

	if (isPauseBtnClicked == true) {	//일시정지된 상황이라면 한 프레임만 출력해서 화면을 바꿔줌
		Mat temp_frame;
		if (radioChoice == 2) {	//radio btn이 이진영상이면, 이진 영상을 출력
			Mat img_labels, stats, centroids;

			//새로운 배경이 필요하기 전에는 만들어진 base gray배경을 사용
			if (releasedPoint < FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND - 1){
				printf("있는거 사용\n");
				Mat loadedBG = imread(getBackgroundFilePath(fileNameNoExtension), IMREAD_GRAYSCALE);
				capture.read(temp_frame);
				//그레이스케일 변환
				cvtColor(temp_frame, temp_frame, CV_RGB2GRAY);
				//temp_frame = ExtractFg(temp_frame, loadedBG, ROWS, COLS);// 전경 추출
				imwrite(getTempBackgroundFilePath(fileNameNoExtension), loadedBG);
				loadedBG = NULL;
				loadedBG.release();
			}
			else{	//새로운 배경이 필요할때 다시 만들어서 저장함
				printf("새로 생성\n");
				capture.set(CV_CAP_PROP_POS_FRAMES, releasedPoint - FRAMES_FOR_MAKE_BACKGROUND);

				setArrayToZero(bg_array, ROWS, COLS);
				for (int j = FRAMES_FOR_MAKE_BACKGROUND; j > 0; j--){
					capture.read(temp_frame);
					cvtColor(temp_frame, temp_frame, CV_RGB2GRAY);
					averageBG(temp_frame, bg_array);
				}

				accIntArrayToMat(background_binaryVideo_gray, bg_array, FRAMES_FOR_MAKE_BACKGROUND);
				imwrite(getTempBackgroundFilePath(fileNameNoExtension), background_binaryVideo_gray);
				isAlreadyBGMade = true;
				capture.set(CV_CAP_PROP_POS_FRAMES, releasedPoint);
				capture.read(temp_frame);
				cvtColor(temp_frame, temp_frame, CV_RGB2GRAY);
				//temp_frame = ExtractFg(temp_frame, bg_gray, ROWS, COLS);// 전경 추출				//TODO
			}

			//TODO 후에 전처리 연산과 같게 하기
			// 전경 추출
			//temp_frame = ExtractFg(temp_frame, background_gray, ROWS, COLS);

			//// 이진화
			//threshold(temp_frame, temp_frame, 5, 255, CV_THRESH_BINARY);

			//// 노이즈 제거
			//temp_frame = morphologicalOperation(temp_frame);
			//blur(temp_frame, temp_frame, Size(9, 9));
			//temp_frame = morphologicalOperation(temp_frame);

			//threshold(temp_frame, temp_frame, 5, 255, CV_THRESH_BINARY);

			/*int numOfLables = connectedComponentsWithStats(temp_frame, img_labels,
				stats, centroids, 8, CV_32S);*/

			//cvtColor(temp_frame, temp_frame, CV_GRAY2BGR);

			////라벨링 된 이미지에 각각 직사각형으로 둘러싸기 
			//for (int j = 1; j < numOfLables; j++) {
			//	//int area = stats.at<int>(j, CC_STAT_AREA);
			//	int left = stats.at<int>(j, CC_STAT_LEFT);
			//	int top = stats.at<int>(j, CC_STAT_TOP);
			//	int width = stats.at<int>(j, CC_STAT_WIDTH);
			//	int height = stats.at<int>(j, CC_STAT_HEIGHT);
			//	if (labelSizeFiltering(width, height
			//		, m_SliderWMIN.GetPos(), m_SliderWMAX.GetPos(), m_SliderHMIN.GetPos(), m_SliderHMAX.GetPos())) {
			//		rectangle(temp_frame, Point(left, top), Point(left + width, top + height),
			//			Scalar(0, 0, 255), 1);
			//	}
			//}

			DisplayImage(IDC_RESULT_IMAGE, temp_frame, BIN_VIDEO_TIMER);
			img_labels = NULL;
			stats = NULL;
			centroids = NULL;
			img_labels.release();
			stats.release();
			centroids.release();
		}
		else {
			capture.read(temp_frame);
			DisplayImage(IDC_RESULT_IMAGE, temp_frame, NULL);
		}

		temp_frame = NULL;
		temp_frame.release();
	}
	else if (isPlayBtnClicked) {	//실행 중이었던 경우 마저 실행한다.
		switch (radioChoice) {
		case 0:	//원본영상 마저 출력
			printf("a");
			SetTimer(VIDEO_TIMER, 1000 / m_sliderFps.GetPos(), NULL);
			break;
		case 1:	//합성 영상
			//미구현
			break;
		case 2:	//이진 영상 마저 출력
			if (true){
				Mat temp_frame;

				//새로운 배경이 필요하기 전에는 만들어진 base gray배경을 사용
				if (releasedPoint < FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND - 1){
					printf("있는거 사용\n");
					Mat loadedBG = imread(getBackgroundFilePath(fileNameNoExtension), IMREAD_GRAYSCALE);
					imwrite(getTempBackgroundFilePath(fileNameNoExtension), loadedBG);
					loadedBG = NULL;
					loadedBG.release();
				}
				else{	//새로운 배경이 필요할때 다시 만들어서 저장함
					printf("새로 생성\n");
					capture.set(CV_CAP_PROP_POS_FRAMES, releasedPoint - FRAMES_FOR_MAKE_BACKGROUND);
					setArrayToZero(bg_array, ROWS, COLS);

					for (int j = FRAMES_FOR_MAKE_BACKGROUND; j > 0; j--){
						capture.read(temp_frame);
						cvtColor(temp_frame, temp_frame, CV_RGB2GRAY);
						//						temporalMedianBG(temp_frame, bg_gray);
						averageBG(temp_frame, bg_array);
					}
					accIntArrayToMat(background_binaryVideo_gray, bg_array, FRAMES_FOR_MAKE_BACKGROUND);

					//					imwrite(getTempBackgroundFilePath(fileNameNoExtension), bg_gray);
					imwrite(getTempBackgroundFilePath(fileNameNoExtension), background_binaryVideo_gray);
					isAlreadyBGMade = true;
					capture.set(CV_CAP_PROP_POS_FRAMES, releasedPoint);
				}

				SetTimer(BIN_VIDEO_TIMER, 1000 / m_sliderFps.GetPos(), NULL);
				temp_frame = NULL;
				temp_frame.release();
			}
			break;
		default:
			break;
		}
	}
	return;
}

void CMFC_SyntheticDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	//CDialogEx::OnOK();
}

void CMFC_SyntheticDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	CDialogEx::OnCancel();
}

bool isDierectionAvailable(int val, int val_cur) {
	bool result = false;
	switch (val) {
	case 0:
		result = true;
		break;
	case 1:
		if (val_cur == 20 || val_cur == 23 || val_cur == 29)
			result = true;
		break;
	case 2:
		if (val_cur == 15 || val_cur == 18 || val_cur == 24)
			result = true;
		break;
	case 3:
		if (val_cur == 13 || val_cur == 23 || val_cur == 18)
			result = true;
		break;
	case 4:
		if (val_cur == 19 || val_cur == 29 || val_cur == 24)
			result = true;
		break;
	case 5:
		if (val_cur == 10)
			result = true;
		break;
	case 6:
		if (val_cur == 23)
			result = true;
		break;
	case 7:
		if (val_cur == 18)
			result = true;
		break;
	case 8:
		if (val_cur == 29)
			result = true;
		break;
	case 9:
		if (val_cur == 24)
			result = true;
		break;
	default:
		break;
	}

	return result;
}

bool isColorAvailable(boolean colorCheckArray[], unsigned int colorArray[]){
	// 첫번쨰가 0번쨰 요소, 두번쨰가 1번째 ...
	unsigned int sorted_value[3] = { 0, }; 
	int sorted_index[3] = { 0, };
	int total_color_value = 0;
	int check_count = 0;

	// 많이 발견된 색상별로 순위를 매김(0~2순위)
	for (int i = 0; i < COLORS; i++) {
		// 전체 색상값의 합을 구함
		total_color_value += colorArray[i];

		// 전체 체크한 갯수를 구함
		if (colorCheckArray[i] == true) check_count++;

		if (colorArray[i] >= sorted_value[0]){
			sorted_value[2] = sorted_value[1];
			sorted_value[1] = sorted_value[0];
			sorted_value[0] = colorArray[i];
			
			sorted_index[2] = sorted_index[1];
			sorted_index[1] = sorted_index[0];
			sorted_index[0] = i;
		}
		else if (colorArray[i] >= sorted_value[1]){
			sorted_value[2] = sorted_value[1];
			sorted_value[1] = colorArray[i];

			sorted_index[2] = sorted_index[1];
			sorted_index[1] = i;
		}
		else if (colorArray[i] >= sorted_value[2]) {
			sorted_value[2] = colorArray[i];
			sorted_index[2] = i;
		}
	}
	// 확인코드
	/*
	printf("first = %d second = %d third = %d\n"
		, sorted_value[0], sorted_value[1], sorted_value[2]);
	*/

	// 희소색에 가중치 부여 (현재 red, orange, yellow에 대해서)
	// 또한 두번쨰로 검정색 또는 하양이 많이 나오는 경우는 인덱스 하나 밀어주기
	/*
	if (sorted_index[2] == RED || sorted_index[2] == ORANGE || sorted_index[2] == YELLOW) {
		sorted_value[2] *= 1.4;
		if (sorted_value[2] > sorted_value[1]) {
			int temp_value = sorted_value[2];
			sorted_value[2] = sorted_value[1];
			sorted_value[1] = temp_value;
		}
	}
	*/

	// 또한 두번쨰로 검정색 또는 하양이 많이 나오는 경우는 인덱스 하나 밀어주기
	if (sorted_index[1] == BLACK || sorted_index[1] == WHITE) {
		int temp_value = sorted_value[2];
		sorted_value[2] = sorted_value[1];
		sorted_value[1] = temp_value;
	}
	
	// 전체 나온 색깔의 비율을 따져서 세번째, 두번째로 나온 색상도 검출할 것인지 판별함
	if (((double)sorted_value[2] / (double)total_color_value) > 0.2) 
		return isColorChecker(colorCheckArray, sorted_index, 3);

	if (((double)sorted_value[1] / (double)total_color_value) > 0.2) 
		return isColorChecker(colorCheckArray, sorted_index, 2);

	/*
	// 두번째로 많이 나온 색깔과 첫 번쨰 나온 색과 차이가 클 경우에 (일반 색상에서)
	if ((double)sorted_value[1] < (double)sorted_value[0] * 0.4) {
		if (colorCheckArray[sorted_index[0]])
			return true;
		else
			return false;
	}

	// 두번째로 많이 나온 색깔과 첫 번쨰 나온 색과 차이가 클 경우에 (블랙/화이트일 경우에는 값의 폭을 좁게)
	if (((double)sorted_value[1] < (double)sorted_value[0] * 0.7)
		&& (sorted_index[1] == WHITE) && (sorted_index[1] == BLACK)) {
		if (colorCheckArray[sorted_index[0]])
			return true;
		else
			return false;
	}
	*/
	// 일반적인 경우
	return isColorChecker(colorCheckArray, sorted_index, 1);
}

bool isColorChecker(boolean color_array[], int sorted_index[], int num_of_index) {
	bool colorcheck_flag = false;
	for (int i = 0; i < num_of_index; i++) 
		colorcheck_flag = colorcheck_flag || color_array[sorted_index[i]];
	return colorcheck_flag;
}

// 방향과 색깔이 매치하는 지를 확인하는 함수
bool CMFC_SyntheticDlg::isDirectionAndColorMatch(segment object) {
	// checkBox의 체크 여부를 가져와서 배열에 저장함
	boolean isColorCheckedArray[COLORS] = {
		IsDlgButtonChecked(IDC_CHECK_RED),
		IsDlgButtonChecked(IDC_CHECK_ORANGE),
		IsDlgButtonChecked(IDC_CHECK_YELLOW),
		IsDlgButtonChecked(IDC_CHECK_GREEN),
		IsDlgButtonChecked(IDC_CHECK_BLUE),
		IsDlgButtonChecked(IDC_CHECK_MAGENTA),
		IsDlgButtonChecked(IDC_CHECK_BLACK),
		IsDlgButtonChecked(IDC_CHECK_WHITE),
		IsDlgButtonChecked(IDC_CHECK_GRAY)
	};

	// comboBox의 문자열의 데이터를 가져옴
	int indexFirst = mComboStart.GetCurSel();
	int indexLast = mComboEnd.GetCurSel();

	int tempTimetag, stamp, label;
	int tempFirst, tempLast;

	unsigned int colors[COLORS] = { 0, };

	// fp_detail 파일 열기
	fp_detail = fopen(getDetailTextFilePath(fileNameNoExtension).c_str(), "r");

	// fp_detail의 데이터 읽어오기
	string txt = readTxt(getDetailTextFilePath(fileNameNoExtension).c_str());
	size_t posOfTimetag = txt.find(to_string(object.timeTag).append(" ").append(to_string(object.index)));
	if (posOfTimetag != string::npos) {
		int posOfNL = txt.find("\n", posOfTimetag);

		string capture = txt.substr(posOfTimetag, posOfNL - posOfTimetag);
		char *line = new char[capture.length() + 1];
		strcpy(line, capture.c_str());
		char *ptr = strtok(line, " ");

		if (ptr != NULL) {
			stamp = atoi(ptr);
			ptr = strtok(NULL, " ");
			if (ptr != NULL) {
				label = atoi(ptr);
			}
			// 객체의 first, end flag 가져오기 
			ptr = strtok(NULL, " ");
			if (ptr != NULL) {
				tempFirst = atoi(ptr);
			}
			ptr = strtok(NULL, " ");
			if (ptr != NULL) {
				tempLast = atoi(ptr);
			}
			// 색 영역 데이터 가져오기
			for (int i = 0; i < COLORS; i++) {
				ptr = strtok(NULL, " ");
				if (ptr != NULL) {
					colors[i] = atoi(ptr);
				}
			}
		}
		delete[] line;
	}

	fclose(fp_detail);

	bool isDirectionOk = isDierectionAvailable(indexFirst, tempFirst) && isDierectionAvailable(indexLast, tempLast);
	bool isColorOk = isColorAvailable(isColorCheckedArray, colors);

	if (isDirectionOk && isColorOk)
		return true;
	else
		return false;
}

void CMFC_SyntheticDlg::OnBnClickedBtnSynSave()
{
	//실행중인 타이머 종료
	KillTimer(BIN_VIDEO_TIMER);
	KillTimer(VIDEO_TIMER);
	KillTimer(SYN_RESULT_TIMER);

	boolean isSynPlayable = checkSegmentation();

	if (isSynPlayable) {

		unsigned int obj1_TimeTag = m_sliderSearchStartTime.GetPos() * 1000;	//검색할 TimeTag1
		unsigned int obj2_TimeTag = m_sliderSearchEndTime.GetPos() * 1000;	//검색할 TimeTag2

		if (obj1_TimeTag >= obj2_TimeTag) {
			AfxMessageBox("Search start time can't larger than end time");
			return;
		}
		else{
			CProgressDlg ProgressDlg(this);                // this 를 사용하여 부모를 지정.
			ProgressDlg.CenterWindow();
			ProgressDlg.mode = 2;
			ProgressDlg.obj1_TimeTag = obj1_TimeTag;	//검색할 TimeTag1
			ProgressDlg.obj2_TimeTag = obj2_TimeTag;	//검색할 TimeTag2
			ProgressDlg.videoFilePath = videoFilePath;
			ProgressDlg.ROWS = ROWS;
			ProgressDlg.COLS = COLS;
			ProgressDlg.videoStartMsec = videoStartMsec;
			ProgressDlg.FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND = FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND;
			ProgressDlg.FRAMES_FOR_MAKE_BACKGROUND = FRAMES_FOR_MAKE_BACKGROUND;
			ProgressDlg.fileNameNoExtension = fileNameNoExtension;
			ProgressDlg.DoModal();
		}

	}
	else {
		AfxMessageBox("You can't save without segmentation results");
	}
}

//FPS 슬라이더 놓았을 경우 바로 재생
void CMFC_SyntheticDlg::OnReleasedcaptureSynSliderFps(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	if (isPlayBtnClicked){
		int releasedPoint = m_sliderFps.GetPos();
		switch (radioChoice){
		case 0:	//원본
			KillTimer(VIDEO_TIMER);
			SetTimer(VIDEO_TIMER, 1000 / releasedPoint, NULL);
			break;
		case 1:	//합성
			KillTimer(SYN_RESULT_TIMER);
			SetTimer(SYN_RESULT_TIMER, 1000 / releasedPoint, NULL);
			break;
		case 2: //이진
			KillTimer(BIN_VIDEO_TIMER);
			SetTimer(BIN_VIDEO_TIMER, 1000 / releasedPoint, NULL);
			break;
		default:
			break;
		}
	}
}

//버튼 색상
void CMFC_SyntheticDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC dc;
	RECT rect;
	UINT state;
	switch (nIDCtl){
	case IDC_COLOR_RED:
		dc.Attach(lpDrawItemStruct->hDC);   // Get the Button DC to CDC

		rect = lpDrawItemStruct->rcItem;     //Store the Button rect to our local rect.
		dc.Draw3dRect(&rect, RGB(255, 255, 255), RGB(0, 0, 0));
		dc.FillSolidRect(&rect, RGB(255, 0, 0));//Here you can define the required color to appear on the Button.

		state = lpDrawItemStruct->itemState;  //This defines the state of the Push button either pressed or not. 

		dc.DrawEdge(&rect, EDGE_SUNKEN, BF_RECT);
		break;
	case IDC_COLOR_GREEN:
		dc.Attach(lpDrawItemStruct->hDC);   // Get the Button DC to CDC

		rect = lpDrawItemStruct->rcItem;     //Store the Button rect to our local rect.
		dc.Draw3dRect(&rect, RGB(255, 255, 255), RGB(0, 0, 0));
		dc.FillSolidRect(&rect, RGB(0, 255, 0));//Here you can define the required color to appear on the Button.

		state = lpDrawItemStruct->itemState;  //This defines the state of the Push button either pressed or not. 

		dc.DrawEdge(&rect, EDGE_SUNKEN, BF_RECT);
		break;
	case IDC_COLOR_BLUE:
		dc.Attach(lpDrawItemStruct->hDC);   // Get the Button DC to CDC

		rect = lpDrawItemStruct->rcItem;     //Store the Button rect to our local rect.
		dc.Draw3dRect(&rect, RGB(255, 255, 255), RGB(0, 0, 0));
		dc.FillSolidRect(&rect, RGB(0, 0, 255));//Here you can define the required color to appear on the Button.

		state = lpDrawItemStruct->itemState;  //This defines the state of the Push button either pressed or not. 

		dc.DrawEdge(&rect, EDGE_SUNKEN, BF_RECT);
		break;
	case IDC_COLOR_ORANGE:
		dc.Attach(lpDrawItemStruct->hDC);   // Get the Button DC to CDC

		rect = lpDrawItemStruct->rcItem;     //Store the Button rect to our local rect.
		dc.Draw3dRect(&rect, RGB(255, 255, 255), RGB(0, 0, 0));
		dc.FillSolidRect(&rect, RGB(255, 102, 0));//Here you can define the required color to appear on the Button.

		state = lpDrawItemStruct->itemState;  //This defines the state of the Push button either pressed or not. 

		dc.DrawEdge(&rect, EDGE_SUNKEN, BF_RECT);
		break;
	case IDC_COLOR_YELLOW:
		dc.Attach(lpDrawItemStruct->hDC);   // Get the Button DC to CDC

		rect = lpDrawItemStruct->rcItem;     //Store the Button rect to our local rect.
		dc.Draw3dRect(&rect, RGB(255, 255, 255), RGB(0, 0, 0));
		dc.FillSolidRect(&rect, RGB(255, 255, 0));//Here you can define the required color to appear on the Button.

		state = lpDrawItemStruct->itemState;  //This defines the state of the Push button either pressed or not. 

		dc.DrawEdge(&rect, EDGE_SUNKEN, BF_RECT);
		break;
	case IDC_COLOR_MAGENTA:
		dc.Attach(lpDrawItemStruct->hDC);   // Get the Button DC to CDC

		rect = lpDrawItemStruct->rcItem;     //Store the Button rect to our local rect.
		dc.Draw3dRect(&rect, RGB(255, 255, 255), RGB(0, 0, 0));
		dc.FillSolidRect(&rect, RGB(255, 0, 255));//Here you can define the required color to appear on the Button.

		state = lpDrawItemStruct->itemState;  //This defines the state of the Push button either pressed or not. 

		dc.DrawEdge(&rect, EDGE_SUNKEN, BF_RECT);
		break;
	case IDC_COLOR_BLACK:
		dc.Attach(lpDrawItemStruct->hDC);   // Get the Button DC to CDC

		rect = lpDrawItemStruct->rcItem;     //Store the Button rect to our local rect.
		dc.Draw3dRect(&rect, RGB(255, 255, 255), RGB(0, 0, 0));
		dc.FillSolidRect(&rect, RGB(0, 0, 0));//Here you can define the required color to appear on the Button.

		state = lpDrawItemStruct->itemState;  //This defines the state of the Push button either pressed or not. 

		dc.DrawEdge(&rect, EDGE_SUNKEN, BF_RECT);
		break;
	case IDC_COLOR_GRAY:
		dc.Attach(lpDrawItemStruct->hDC);   // Get the Button DC to CDC

		rect = lpDrawItemStruct->rcItem;     //Store the Button rect to our local rect.
		dc.Draw3dRect(&rect, RGB(255, 255, 255), RGB(0, 0, 0));
		dc.FillSolidRect(&rect, RGB(153, 153, 153));//Here you can define the required color to appear on the Button.

		state = lpDrawItemStruct->itemState;  //This defines the state of the Push button either pressed or not. 

		dc.DrawEdge(&rect, EDGE_SUNKEN, BF_RECT);
		break;
	case IDC_COLOR_WHITE:
		dc.Attach(lpDrawItemStruct->hDC);   // Get the Button DC to CDC

		rect = lpDrawItemStruct->rcItem;     //Store the Button rect to our local rect.
		dc.Draw3dRect(&rect, RGB(255, 255, 255), RGB(0, 0, 0));
		dc.FillSolidRect(&rect, RGB(255, 255, 255));//Here you can define the required color to appear on the Button.

		state = lpDrawItemStruct->itemState;  //This defines the state of the Push button either pressed or not. 

		dc.DrawEdge(&rect, EDGE_SUNKEN, BF_RECT);
		break;
	}

	dc.Detach();  // Detach the Button DC
	CDialogEx::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CMFC_SyntheticDlg::OnBnClickedCheckAll()
{
	if (IsDlgButtonChecked(IDC_CHECK_ALL)){
		CheckDlgButton(IDC_CHECK_RED, TRUE);
		CheckDlgButton(IDC_CHECK_GREEN, TRUE);
		CheckDlgButton(IDC_CHECK_BLUE, TRUE);
		CheckDlgButton(IDC_CHECK_ORANGE, TRUE);
		CheckDlgButton(IDC_CHECK_YELLOW, TRUE);
		CheckDlgButton(IDC_CHECK_MAGENTA, TRUE);
		CheckDlgButton(IDC_CHECK_BLACK, TRUE);
		CheckDlgButton(IDC_CHECK_GRAY, TRUE);
		CheckDlgButton(IDC_CHECK_WHITE, TRUE);
	}
	else{
		CheckDlgButton(IDC_CHECK_RED, FALSE);
		CheckDlgButton(IDC_CHECK_GREEN, FALSE);
		CheckDlgButton(IDC_CHECK_BLUE, FALSE);
		CheckDlgButton(IDC_CHECK_ORANGE, FALSE);
		CheckDlgButton(IDC_CHECK_YELLOW, FALSE);
		CheckDlgButton(IDC_CHECK_MAGENTA, FALSE);
		CheckDlgButton(IDC_CHECK_BLACK, FALSE);
		CheckDlgButton(IDC_CHECK_GRAY, FALSE);
		CheckDlgButton(IDC_CHECK_WHITE, FALSE);
	}
}
