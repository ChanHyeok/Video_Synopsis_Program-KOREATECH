// MFC_SyntheticDlg.cpp : implementation file
#include <crtdbg.h>
#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "MFC_SyntheticDlg.h"
#include "afxdialogex.h"
#include "ProgressDlg.h"

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
#define PROGRESS_BAR_TIMER 4	//로딩바에 사용하는 타이머
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
const int FRAMES_FOR_MAKE_BACKGROUND = 2000;	//영상 Load시 처음에 배경을 만들기 위한 프레임 수
const int FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND = 4000;	//다음 배경을 만들기 위한 시간간격(동적)
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


bool synthesisEndFlag; // 합성이 끝남을 알려주는 플래그
// File 관련
FILE *fp; // frameInfo를 작성할 File Pointer
FILE *fp_detail; // obj_detail_<>를 작성할 File Pointer. 위치 및 색상 정보 저장
std::string fileNameExtension(""); // 입력받은 비디오파일 이름
std::string fileNameNoExtension("");// 확장자가 없는 파일 이름
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
const std::string currentDateTime() {
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
	DDX_Control(pDX, IDC_PROGRESS, m_LoadingProgressCtrl);
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

	//프로그레스바 숨김
	m_LoadingProgressCtrl.ShowWindow(false);

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

		video_filename_cstr = NULL;
		free(video_filename_cstr);

		//segmentation 결과를 저장할 텍스트 파일 이름 설정
		txt_filename = RESULT_TEXT_FILENAME;
		fileNameNoExtension = getFileName(cstrImgPath, '\\', false);
		txt_filename = txt_filename.append(fileNameNoExtension).append(".txt");

		// 세그먼테이션 데이터(txt, jpg들)를 저장할 디렉토리 유무확인, 없으면 만들어줌

		// root 디렉토리 생성(폴더명 data)
		if (!isDirectory(SEGMENTATION_DATA_DIRECTORY_NAME.c_str())) {
			int rootDirectory_check = makeDataRootDirectory();
			printf("root 생성");
		}

		// video 이름 별 디렉토리 생성(폴더명 확장자 없는 파일 이름)
		if (!isDirectory(getDirectoryPath(fileNameNoExtension.c_str()))) {
			int subDirectory_check = makeDataSubDirectory(getDirectoryPath(fileNameNoExtension));
			printf("sub 생성");
		}

		// obj 디렉토리 생성
		if (!isDirectory(getObjDirectoryPath(fileNameNoExtension.c_str()))) {
			int subObjDirectory_check = makeDataSubDirectory(getObjDirectoryPath(fileNameNoExtension));
			printf("sub-obj 생성");
		}
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
		if (MessageBox("프로그램을 종료하시겠습니까??", "Exit", MB_YESNO) == IDYES) {
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
	printf("OnCancel\n");

	m_segmentArray = NULL;
	capture = NULL;
	background_loadedFromFile = NULL;
	background_binaryVideo_gray = NULL;
	// cpp파일 내 전역변수들 메모리 해제
	fileNameExtension.clear();
	fileNameNoExtension.clear();
	txt_filename.clear();

	background_loadedFromFile.release();

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
	printf("OnDestroy\n");
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
	float fImageRatio = float(tempImage->width) / float(tempImage->height);
	float fRectRatio = float(rect.right) / float(rect.bottom);
	float fScaleFactor;
	if (fImageRatio < fRectRatio) {
		fScaleFactor = float(rect.bottom) / float(tempImage->height);	//TRACE("%f",fScaleFactor);
		int rightWithRatio = tempImage->width * fScaleFactor;
		float halfOfDif = ((float)rect.right - (float)rightWithRatio) / (float)2;
		rect.left = halfOfDif;
		rect.right = rightWithRatio;
	}
	else {
		fScaleFactor = float(rect.right) / float(tempImage->width);	//TRACE("%f",fScaleFactor);
		int bottomWithRatio = tempImage->height * fScaleFactor;
		float halfOfDif = ((float)rect.bottom - (float)bottomWithRatio) / (float)2;
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
					temp_frame.copyTo(background_binaryVideo_gray);
				}
				else{	//배경 생성
					temporalMedianBG(temp_frame, background_binaryVideo_gray);
				}
			}

			//만든 배경을 저장해야 할 경우
			if (curFrameCount_nomalized == FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND - 1){
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

			threshold(temp_frame, temp_frame, 5, 255, CV_THRESH_BINARY);

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
	case PROGRESS_BAR_TIMER:
		if (m_LoadingProgressCtrl.GetPos() == 100) {
			KillTimer(PROGRESS_BAR_TIMER);
		}
		else
			m_LoadingProgressCtrl.OffsetPos(1);
		break;
	case SYN_RESULT_TIMER:
		printf("#");
		Mat bg_copy; background_loadedFromFile.copyTo(bg_copy);
		// 불러온 배경을 이용하여 합성을 진행
		Mat syntheticResult = getSyntheticFrame(bg_copy);
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

	cout << currentDateTime() << endl;
	KillTimer(LOGO_TIMER);
	KillTimer(VIDEO_TIMER);
	KillTimer(BIN_VIDEO_TIMER);

	CString str_startHour, str_startMinute;
	//Edit Box로부터 시작 시간과 분을 읽어옴
	m_pEditBoxStartHour->GetWindowTextA(str_startHour);
	m_pEditBoxStartMinute->GetWindowTextA(str_startMinute);

	// Edit box에 문자 입력, 또는 범위외 입력 시 예외처리
	if (segmentationTimeInputException(str_startHour, str_startMinute)) {
		//로딩 창 띄움
		m_LoadingProgressCtrl.ShowWindow(true);
		m_LoadingProgressCtrl.SetRange(0, totalFrameCount);
		m_LoadingProgressCtrl.SetPos(0);

		segmentationOperator(&capture, atoi(str_startHour), atoi(str_startMinute)
			, m_SliderWMIN.GetPos(), m_SliderWMAX.GetPos(), m_SliderHMIN.GetPos(), m_SliderHMAX.GetPos());	//Object Segmentation

		//라디오버튼 - 합성영상 활성화 / 비활성화
		if (checkSegmentation()) {
			GetDlgItem(IDC_RADIO_PLAY2)->EnableWindow(TRUE);
		}
		else {
			GetDlgItem(IDC_RADIO_PLAY2)->EnableWindow(FALSE);
		}

		//로딩 숨기기
		m_LoadingProgressCtrl.ShowWindow(false);
		cout << currentDateTime() << endl;
	}
	else {	// 범위 외 입력시 예외처리
	}
}

// segmentation 기능 수행, 물체 추적 및 파일로 저장
void CMFC_SyntheticDlg::segmentationOperator(VideoCapture* vc_Source, int videoStartHour, int videoStartMin, int WMIN, int WMAX, int HMIN, int HMAX) {
	vector<pair<int, int>> vectorDetailTXTIndex;	//detail text 파일의 객체 인덱스를 저장할 벡터 <타임태그 - 텍스트파일의 인덱스 값> 구조
	int detailTXTIndex = 0;	//텍스트 파일의 라인 수. 해쉬맵에 value값으로 저장 할 것임.

	videoStartMsec = (videoStartHour * 60 + videoStartMin) * 60 * 1000;

	unsigned int COLS = (int)vc_Source->get(CV_CAP_PROP_FRAME_WIDTH);	//가로 길이
	unsigned int ROWS = (int)vc_Source->get(CV_CAP_PROP_FRAME_HEIGHT);	//세로 길이

	vector<component> humanDetectedVector, prevHumanDetectedVector;
	ComponentVectorQueue prevHumanDetectedVector_queue;
	InitComponentVectorQueue(&prevHumanDetectedVector_queue);

	/* Mat */
	Mat frame(ROWS, COLS, CV_8UC3); // Mat(height, width, channel)
	Mat frame_g(ROWS, COLS, CV_8UC1);
	Mat tmp_background(ROWS, COLS, CV_8UC3);
	//frame 카운터와 현재 millisecond
	int frameCount = 0, temp_frameCount = 0; // 배경을 생성하기 위한 임시 프레임 카운트 생성
	unsigned int currentMsec;

	// 배경 초기화
	tmp_background = imread(getBackgroundFilePath(fileNameNoExtension));

	// 얻어낸 객체 프레임의 정보를 써 낼 텍스트 파일 정의s
	fp = fopen(getTextFilePath(fileNameNoExtension).c_str(), "w");	// 쓰기모드
	fp_detail = fopen(getDetailTextFilePath(fileNameNoExtension).c_str(), "w+");	// 쓰기모드
	fprintf(fp, to_string(videoStartMsec).append("\n").c_str());	//첫줄에 영상시작시간 적어줌

	// vc_source의 시작시간 0으로 초기화
	vc_Source->set(CV_CAP_PROP_POS_MSEC, 0);
	if (true) {
		while (1) {
			vc_Source->read(frame); //get single frame
			if (frame.empty()) {	//예외처리. 프레임이 없음
				perror("Empty Frame");
				break;
			}

			int curFrameCount = (int)vc_Source->get(CV_CAP_PROP_POS_FRAMES);
			int curFrameCount_nomalized = curFrameCount%FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND;

			//그레이스케일 변환
			cvtColor(frame, frame_g, CV_RGB2GRAY);

			//다음에 쓸 배경을 만들어야 할 경우
			if (curFrameCount_nomalized >= (FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND - FRAMES_FOR_MAKE_BACKGROUND)){
				if (curFrameCount_nomalized == (FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND - FRAMES_FOR_MAKE_BACKGROUND)){	//새로 만드는 첫 배경 Init
					printf("Background Making Start : %d frame\n", curFrameCount);
					frame_g.copyTo(background_binaryVideo_gray);
				}
				else{	//배경 생성
					temporalMedianBG(frame_g, background_binaryVideo_gray);
				}
			}

			//만든 배경을 저장해야 할 경우
			if (curFrameCount_nomalized == FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND - 1){
				imwrite(getTempBackgroundFilePath(fileNameNoExtension), background_binaryVideo_gray);
			}

			if (curFrameCount >= FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND && curFrameCount_nomalized == 0){
				printf("Background Changed, %d frame\n", curFrameCount);
			}

			//새로운 배경이 write되기 전 까지는 base gray배경을 사용
			if (curFrameCount < FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND - 1){
				frame_g = ExtractFg(frame_g, imread(getBackgroundFilePath(fileNameNoExtension), IMREAD_GRAYSCALE), ROWS, COLS);// 전경 추출
			}
			else{	//새로운 배경이 만들어진 다음부터는 만들어진 배경을 사용
				frame_g = ExtractFg(frame_g, imread(getTempBackgroundFilePath(fileNameNoExtension), IMREAD_GRAYSCALE), ROWS, COLS);// 전경 추출
			}
			


			////TODO 손보기
			// 이진화
			threshold(frame_g, frame_g, 5, 255, CV_THRESH_BINARY);

			//// 노이즈 제거
			frame_g = morphologyOpening(frame_g);
			frame_g = morphologyClosing(frame_g);
			frame_g = morphologyClosing(frame_g);
			blur(frame_g, frame_g, Size(11, 11));

			threshold(frame_g, frame_g, 5, 255, CV_THRESH_BINARY);

			// MAT형으로 라벨링
			humanDetectedVector = connectedComponentsLabelling(frame_g, ROWS, COLS, WMIN, WMAX, HMIN, HMAX);

			// 현재 프레임의 영상 시간 가져오기
			currentMsec = vc_Source->get(CV_CAP_PROP_POS_MSEC);

			// 영상을 처리하여 파일로 저장하기
			// humanDetectedVector = humanDetectedProcess(humanDetectedVector, prevHumanDetectedVector,
			//	frame, frameCount, videoStartMsec, currentMsec, fp);
			// humanDetected가 있을 경우에만 연산(함수호출 오버헤드 감소를 위함)
			// 영상을 처리하여 타임태그를 새로 부여하고 파일로 저장하기(2)
			if (humanDetectedVector.size() > 0)
				humanDetectedVector = humanDetectedProcess2(humanDetectedVector, prevHumanDetectedVector
				, prevHumanDetectedVector_queue, frame, frameCount, videoStartMsec, currentMsec, fp, &vectorDetailTXTIndex, &detailTXTIndex);

			// 큐가 full일 경우 한자리 비워주기
			if (IsComponentVectorQueueFull(&prevHumanDetectedVector_queue))
				RemoveComponentVectorQueue(&prevHumanDetectedVector_queue);

			// 큐에 매 수행마다 벡터를 무조건 넣어줘야함
			PutComponentVectorQueue(&prevHumanDetectedVector_queue, humanDetectedVector);

			// 확인 코드
			/*
			for (int i = 3; i < 5; i++) {
			vector<component> prevDetectedVector_i = GetComponentVectorQueue(&prevHumanDetectedVector_queue
			, (prevHumanDetectedVector_queue.rear + i) % 5);
			for (int j = 0; j < prevDetectedVector_i.size(); j++) {
			printf("queue[%d] = %d %d %d %d\n", i, prevDetectedVector_i[j].top, prevDetectedVector_i[j].bottom
			, prevDetectedVector_i[j].left, prevDetectedVector_i[j].right);
			}
			vector<component> vclear;
			prevDetectedVector_i.swap(vclear);
			}
			printf("\n");
			*/

			// 벡터 메모리 해제를 빈 벡터 생성(prevHumanDetectedVector 메모리 해제)
			vector<component> vclear;
			prevHumanDetectedVector.swap(vclear);

			vclear.clear();
			prevHumanDetectedVector.clear();



			// 현재 검출한 데이터를 이전 데이터에 저장하기
			prevHumanDetectedVector = humanDetectedVector;

			// 벡터 메모리 해제를 빈 벡터 생성
			humanDetectedVector.swap(vclear);

			vclear.clear();
			humanDetectedVector.clear();

			frameCount++;	temp_frameCount++; //increase frame , temp_frame count
			m_LoadingProgressCtrl.OffsetPos(1);
		}
	}

	printf("segmentation Operator 끝\n");

	//메모리 해제
	frame.release(); frame_g.release();
	vector<component>().swap(humanDetectedVector);
	vector<component>().swap(prevHumanDetectedVector);

	// 텍스트 파일 닫기
	fclose(fp);
	fclose(fp_detail);
}
// component vector 큐를 이용한 추가된 함수
vector<component> humanDetectedProcess2(vector<component> humanDetectedVector, vector<component> prevHumanDetectedVector
	, ComponentVectorQueue prevHumanDetectedVector_Queue, Mat frame, int frameCount, int videoStartMsec, unsigned int currentMsec, FILE *fp, vector<pair<int, int>>* vectorDetailTXTIndex, int* detailTxtIndex) {
	// 현재에서 바로 이전 component 저장
	// prevDetectedVector를 바로 큐에 있는 이전 vector로 지정할 시 
	// 파일 저장할 시 frameCount를 매기는 데에 오류가 생김(오류 발생 원인은 아직까지도 불명)
	vector<component> prevDetectedVector_i = prevHumanDetectedVector;

	// 현재 label의 마지막 수를 저장함
	int maxLabel = humanDetectedVector.size() - 1;

	// 사람을 검출한 양 많큼 반복 (보통 humanCount 갯수 1, 2개 나옴)
	for (int humanCount = 0; humanCount < humanDetectedVector.size(); humanCount++) {
		bool save_flag = false;
		bool findFlag = false;
		//이전 프레임의 검출된 객체가 있을 경우
		if (!prevDetectedVector_i.empty()) {
			for (int j = 0; j < prevDetectedVector_i.size(); j++) {
				// 두 프레임이 겹칠 경우에 대한 연산
				if (!IsComparePrevComponent(humanDetectedVector[humanCount], prevDetectedVector_i[j])) {
					humanDetectedVector[humanCount].timeTag = prevDetectedVector_i[j].timeTag;
					humanDetectedVector[humanCount].label = prevDetectedVector_i[j].label;

					if (IsSaveComponent(humanDetectedVector[humanCount], prevDetectedVector_i[j]))
						save_flag = true;

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

							if (IsSaveComponent(humanDetectedVector[humanCount], prevDetectedVector_i[j]))
								save_flag = true;

							findFlag = true;
						}
					}
					if (findFlag == true)
						break;
				}
			}
		} // end if ((!prevDetectedVector_i.empty())

		// 첫 시행이거나 이전 프레임에 검출된 객체가 없다고 판단될 경우에
		else {
			// 이전 뿐 아니라 그 이전에 데이터에 접근하기
			for (int i = MAXSIZE_OF_COMPONENT_VECTOR_QUEUE - 3; i >= 0; i--) {
				prevDetectedVector_i = GetComponentVectorQueue(&prevHumanDetectedVector_Queue,
					(prevHumanDetectedVector_Queue.rear + i) % MAXSIZE_OF_COMPONENT_VECTOR_QUEUE);
				for (int j = 0; j < prevDetectedVector_i.size(); j++) {
					// 두 프레임이 겹칠 경우에 대한 연산
					if (!IsComparePrevComponent(humanDetectedVector[humanCount], prevDetectedVector_i[j])) {
						humanDetectedVector[humanCount].timeTag = prevDetectedVector_i[j].timeTag;
						humanDetectedVector[humanCount].label = prevDetectedVector_i[j].label;

						if (IsSaveComponent(humanDetectedVector[humanCount], prevDetectedVector_i[j]))
							save_flag = true;

						findFlag = true;
					}
				}
				// 더이상 그 이전에 객체를 고려할 필요가 없음
				if (findFlag == true)
					break;
			}
		} // end else
		// 새 객체가 출현 되었다고 판정함
		if (findFlag == false) {
			humanDetectedVector[humanCount].timeTag = currentMsec;
			humanDetectedVector[humanCount].label = ++maxLabel;
			save_flag = true;
		}

		// 파일에 저장할 수 있도록 함
		if (save_flag == true)
			saveSegmentationData(fileNameNoExtension, humanDetectedVector[humanCount], frame
			, currentMsec, frameCount, fp, fp_detail, ROWS, COLS, vectorDetailTXTIndex, detailTxtIndex);
	} // end for (humanCount) 

	vector<component> vclear;
	prevDetectedVector_i.swap(vclear);

	return humanDetectedVector;
}

// 이전과 연속적이어서 저장할 가치가 있는 지를 판별하는 함수
bool IsSaveComponent(component curr_component, component prev_component) {
	bool return_flag = true;
	const int diff_component_height = ROWS / 16; //  ( 480/15 = 32)
	const int diff_component_width = COLS / 16; //  ( 640/15 = 42)
	// width와 height 크기를 비교
	// 추후 색상 데이터를 보는 식으로 하여 강화
	if (curr_component.label == prev_component.label) {
		if ((abs(curr_component.width - prev_component.width) > diff_component_width) ||
			(abs(curr_component.height - prev_component.height) > diff_component_height)) {
			printf("%d %d save하지 않음\n", curr_component.timeTag, curr_component.height);
			return_flag = false;
		}
	}
	return return_flag;
}
// 현재와 이전에 검출한 결과를 비교, true 면 겹칠 수 없음
bool IsComparePrevComponent(component curr_component, component prev_component) {
	return curr_component.left > prev_component.right
		|| curr_component.right < prev_component.left
		|| curr_component.top > prev_component.bottom
		|| curr_component.bottom < prev_component.top;
}
// 합성된 프레임을 가져오는 연산
Mat CMFC_SyntheticDlg::getSyntheticFrame(Mat bgFrame) {
	int *labelMap = new int[bgFrame.cols * bgFrame.rows];	//겹침을 판단하는 용도
	segment temp_segment; //DeQueue한 결과를 받을 segment
	int countOfObj = segment_queue.count;	//큐 인스턴스의 노드 갯수
	synthesisEndFlag = false;

	//큐가 비었는지 확인한다. 비었으면 더 이상 출력 할 것이 없는 것 이므로 종료
	if (IsEmpty(&segment_queue)) {
		// 타이머를 죽인 이후에
		KillTimer(SYN_RESULT_TIMER);

		// 합성이 끝났다고 판정하여 플래그를 true로 변경
		synthesisEndFlag = true;

		// 동적 해제
		free(labelMap);
		delete[] m_segmentArray;

		// 빈 프레임 반환
		Mat nullFrame(ROWS, COLS, CV_8UC1);
		nullFrame.setTo(Scalar(0));
		return nullFrame;
	}

	Point* TimeTag_p = new Point[countOfObj];		// 타임태그 위치
	string* TimeTag_s = new string[countOfObj];		// 타임태그 내용
	int countOfShowObj = 0;	//실질적으로 출력할 객체 수

	// 객체들 인덱스 정보를 저장하기 위한 벡터
	vector<int> vectorPreNodeIndex;

	//큐에 있는 객체들의 인덱스 정보들을 벡터에 저장
	for (int k = 0; k < countOfObj; k++) {
		int curr_index = Getqueue_IndexOfSegmentArray(&segment_queue);
		temp_segment = Dequeue(&segment_queue);
		vectorPreNodeIndex.push_back(curr_index);
		Enqueue(&segment_queue, temp_segment, curr_index);
	}

	bool isEnqueueFlag = true;
	// 큐에 들어있는 객체 갯수 만큼 DeQueue. 
	for (int i = 0; i < countOfObj; i++) {
		//dequeue한 객체를 출력한다.
		int curIndex = Getqueue_IndexOfSegmentArray(&segment_queue);
		temp_segment = Dequeue(&segment_queue);
		BOOL isCross = false;

		//객체가 이전 객체와 겹치는지 비교, 처음이 아니고 현재 출력할 객체가 timetag의 첫 프레임 일 때
		if ((i != 0 && m_segmentArray[curIndex].first_timeTagFlag)) {
			for (int j = 0; j < i; j++) {
				// 겹침을 확인하고 확인된 객체를 다시 enqueue 연산, 
				if (!IsObjectOverlapingDetector(m_segmentArray[curIndex], m_segmentArray[vectorPreNodeIndex.at(j)])) {
					isCross = true;
					Enqueue(&segment_queue, temp_segment, curIndex);
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
				Enqueue(&segment_queue, temp_segment, curIndex + 1);
			}

			//다음 객체가 있는 프레임이 객체가 있는 프레임과 2 이상, 버퍼 이하 만큼 차이 날 때
			else {
				for (int i = 2; i <= MAX_SEGMENT_TEMP_BUFFER + 1; i++) {
					// 세그먼트 카운트의 차이를 비교함
					if (m_segmentArray[curIndex].frameCount + i == m_segmentArray[curIndex + 1].frameCount) {
						// 이전과 타임태그와 인덱스가 모두 같을 때에 다음 인덱스 enqueue시키기
						//	printf(" 2 이상, 버퍼 이하 만큼 차이 %d %d\n", temp_segment.timeTag, temp_segment.frameCount);
						Enqueue(&segment_queue, temp_segment, curIndex + 1);
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
	const unsigned int filter_object_height = ROWS / 2; //  ( 480/15 = 32)
	const unsigned int filter_object_width = COLS / 2; //  ( 640/15 = 42)

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

	fp = fopen(path.c_str(), "r");

	int segmentCount = 0;
	fseek(fp, 0, SEEK_SET);	//포인터 처음으로 이동
	fgets(txtBuffer, 99, fp);
	sscanf(txtBuffer, "%d", &videoStartMsec);	//텍스트 파일 첫줄에 명시된 실제 영상 시작 시간 받아옴

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

		// filename 저장
		segmentArray[segmentCount].fileName
			.append(to_string(segmentArray[segmentCount].timeTag)).append("_")
			.append(to_string(segmentArray[segmentCount].msec)).append("_")
			.append(to_string(segmentArray[segmentCount].frameCount)).append("_")
			.append(to_string(segmentArray[segmentCount].index))
			.append(".jpg");

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
	free(tmp_segment);
	free(txtBuffer);

	// 텍스트 파일 닫기
	fclose(fp);

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
			if (segmentArray[i].timeTag == segmentArray[i].msec && isDirectionMatch(segmentArray[i].timeTag)) {
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
		if (synthesisEndFlag == false)
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
	if(!isGrayBackgroundExists(getBackgroundFilePath(fileNameNoExtension))){
		CProgressDlg ProgressDlg(this);                // this 를 사용하여 부모를 지정.
		ProgressDlg.CenterWindow();
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
	return ;
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

	mButtonSynSave.MoveWindow(box_syntheticX + box_syntheticWidth - 110, box_syntheticY + box_syntheticHeight*0.8, 100, 20, TRUE);

	//로딩 바
	m_LoadingProgressCtrl.MoveWindow(dialogWidth / 2 - 300, dialogHeight / 2 - 80, 600, 80, TRUE);
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

	//SetDlgItemText(IDC_SEG_STRING_VAL_MIN_W, _T("0"));
	//SetDlgItemText(IDC_SEG_STRING_VAL_MAX_W, _T("0"));
	//SetDlgItemText(IDC_SEG_STRING_VAL_MIN_H, _T("0"));
	//SetDlgItemText(IDC_SEG_STRING_VAL_MAX_H, _T("0"));
	//m_SliderWMIN.SetPos(0);
	//m_SliderWMAX.SetPos(0);
	//m_SliderHMIN.SetPos(0);
	//m_SliderHMAX.SetPos(0);

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
			Mat bg_gray;

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
				int temporalBGCount = FRAMES_FOR_MAKE_BACKGROUND;
				capture.set(CV_CAP_PROP_POS_FRAMES, releasedPoint - temporalBGCount);
				capture.read(temp_frame);
				cvtColor(temp_frame, bg_gray, CV_RGB2GRAY);
				for (int j = temporalBGCount-1; j >0; j--){
					capture.read(temp_frame);
					cvtColor(temp_frame, temp_frame, CV_RGB2GRAY);
					temporalMedianBG(temp_frame, bg_gray);
				}
				imwrite(getTempBackgroundFilePath(fileNameNoExtension), bg_gray);
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
			bg_gray = NULL;
			bg_gray.release();
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
				Mat temp_frame, bg_gray;

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
					int temporalBGCount = FRAMES_FOR_MAKE_BACKGROUND;
					capture.set(CV_CAP_PROP_POS_FRAMES, releasedPoint - temporalBGCount);
					capture.read(temp_frame);
					cvtColor(temp_frame, bg_gray, CV_RGB2GRAY);
					for (int j = temporalBGCount - 1; j > 0; j--){
						capture.read(temp_frame);
						cvtColor(temp_frame, temp_frame, CV_RGB2GRAY);
						temporalMedianBG(temp_frame, bg_gray);
					}
					imwrite(getTempBackgroundFilePath(fileNameNoExtension), bg_gray);
					capture.set(CV_CAP_PROP_POS_FRAMES, releasedPoint);
				}

				SetTimer(BIN_VIDEO_TIMER, 1000 / m_sliderFps.GetPos(), NULL);
				temp_frame = NULL;
				temp_frame.release();
				bg_gray = NULL;
				bg_gray.release();
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
bool CMFC_SyntheticDlg::isDirectionMatch(int timetag) {
	bool isFirstOk = false, isLastOk = false;
	int indexFirst = mComboStart.GetCurSel();
	int indexLast = mComboEnd.GetCurSel();

	FILE* fp_detail = fopen(getDetailTextFilePath(fileNameNoExtension).c_str(), "r");
	int tempTimetag;
	int tempFirst;
	int tempLast;
	while (fscanf(fp_detail, "%d %d %d", &tempTimetag, &tempFirst, &tempLast) != EOF) {
		if (tempTimetag == timetag)
			break;
	}
	fclose(fp_detail);

	isFirstOk = isDierectionAvailable(indexFirst, tempFirst);
	isLastOk = isDierectionAvailable(indexLast, tempLast);

	if (isFirstOk && isLastOk)
		return true;
	else return false;
}

void CMFC_SyntheticDlg::OnBnClickedBtnSynSave()
{
	//실행중인 타이머 종료
	KillTimer(BIN_VIDEO_TIMER);
	KillTimer(VIDEO_TIMER);
	KillTimer(SYN_RESULT_TIMER);

	boolean isSynPlayable = checkSegmentation();

	if (isSynPlayable) {
		m_LoadingProgressCtrl.ShowWindow(true);
		m_LoadingProgressCtrl.SetRange(0, 100);
		m_LoadingProgressCtrl.SetPos(0);

		segment *segmentArray = new segment[BUFFER];

		int segmentCount = readSegmentTxtFile(segmentArray);

		//TimeTag를 Edit box로부터 입력받음
		unsigned int obj1_TimeTag = m_sliderSearchStartTime.GetPos() * 1000;	//검색할 TimeTag1
		unsigned int obj2_TimeTag = m_sliderSearchEndTime.GetPos() * 1000;	//검색할 TimeTag2

		if (obj1_TimeTag >= obj2_TimeTag) {
			AfxMessageBox("Search start time can't larger than end time");
			return;
		}

		m_segmentArray = segmentArray;

		//파일로 동영상을 저장하기 위한 준비  
		VideoWriter outputVideo;
		string str = "./data/";
		str.append(fileNameNoExtension).append("/").append(fileNameNoExtension).append("_").append(currentDateTime()).append(".avi");

		outputVideo.open(str, VideoWriter::fourcc('X', 'V', 'I', 'D'),
			25, Size((int)background_loadedFromFile.cols, (int)background_loadedFromFile.rows), true);
		if (!outputVideo.isOpened())
		{
			cout << "동영상을 저장하기 위한 초기화 작업 중 에러 발생" << endl;
		}
		else {
			while (1) {
				Mat bg_copy; background_loadedFromFile.copyTo(bg_copy);
				// 불러온 배경을 이용하여 합성을 진행
				Mat syntheticResult = getSyntheticFrame(bg_copy);
				if (segment_queue.count == 0) {
					printf("영상 저장 끝\n");
					delete[] m_segmentArray;

					outputVideo.release();
					syntheticResult = NULL;
					bg_copy = NULL;
					syntheticResult.release();
					bg_copy.release();
					m_LoadingProgressCtrl.SetPos(100);
					break;
				}
				else {
					outputVideo << syntheticResult;
					syntheticResult = NULL;
					bg_copy = NULL;
					syntheticResult.release();
					bg_copy.release();
					m_LoadingProgressCtrl.OffsetPos(1);
				}
			}
		}
		m_LoadingProgressCtrl.ShowWindow(false);
	}
	else { //실행 못하는 경우 segmentation을 진행하라고 출력
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
