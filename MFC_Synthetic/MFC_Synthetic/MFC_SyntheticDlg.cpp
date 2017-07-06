// MFC_SyntheticDlg.cpp : implementation file
#include <crtdbg.h>

#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "MFC_SyntheticDlg.h"
#include "afxdialogex.h"
#include "SplashScreenEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// 메모리 누수를 점검하는 키워드 (http://codes.xenotech.net/38)
// 점검하기 위해 디버깅 모드로 실행 후, 디버그 로그를 보면 됨
// #include <crtdbg.h> 선언 이후 _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); 

/*** 각종 상수들 ***/
#define LOGO_TIMER 0
#define VIDEO_TIMER 1
#define SYN_RESULT_TIMER 2
#define BIN_VIDEO_TIMER 3
#define MAX_STR_BUFFER_SIZE  128 // 문자열 출력에 쓸 버퍼 길이

// 배경을 만드는 데 필요한 프레임 카운트 상수들 정의
const int FRAMES_FOR_MAKE_BACKGROUND = 350; // 첫 번째 배경을 만들기 까지 필요한 프레임카운트
const int FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND = 1500; // 다음 배경을 만들기 위한 시간간격(동적)
// fps가 약 23-25 가량 나오는 영상에서 약 1분이 흐른 framecount 값은 1500

/***  전역변수  ***/
segment *m_segmentArray;
Queue segment_queue; // C++ STL의 queue 키워드와 겹치기 때문에 변수를 조정함
int videoStartMsec, segmentCount, fps; // 시작 millisecond, 세그먼트 카운팅변수, 초당 프레임수
int radioChoice, preRadioChoice;	//라디오 버튼 선택 결과 저장 변수. 0 - 원본영상, 1 - 합성영상, 2 - 이진영상
boolean isPlayBtnClicked, isPauseBtnClicked;
Mat background, background_gray; // 배경 프레임과 원본 프레임

unsigned int COLS, ROWS;

// background 전역변수 삭제
// synthesis 중에 배경관련 정보들이 사라지는 버그가 발생하여 수정하면서 m_background를 삭제함

// File 관련
FILE *fp; // frameInfo를 작성할 File Pointer
std::string video_filename(""); // 입력받은 비디오파일 이름
std::string background_filename, txt_filename = RESULT_TEXT_FILENAME; // 배경 파일 이름과 txt 파일 이름

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
CMFC_SyntheticDlg::~CMFC_SyntheticDlg()
{
	background.release();
	background_gray.release();
	// MFC에서 소멸자에서의 메모리 해제는 의미가 없음
}


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
}

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
	int dialogHeight = m_rectCurHist.bottom - 50;//작업표시줄 크기 빼줌
	int padding = 10;
	SetWindowPos(&wndTop, 0, 0, dialogWidth, dialogHeight, SWP_NOMOVE);//다이얼로그 크기 조정

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
	int box_MenuHeight = ((dialogHeight - 3 * padding)*0.7 - padding)*0.3;

	pGroupMenu->MoveWindow(box_MenuX, box_MenuY, box_MenuWidth, box_MenuHeight, TRUE);
	pStringFileName->MoveWindow(box_MenuX + padding, box_MenuY + 2 * padding, 230, 20, TRUE);
	pButtonLoad->MoveWindow(box_MenuX + box_MenuWidth - padding - 100, box_MenuY + 3 * padding + 20, 100, 20, TRUE);
	pRadioBtn1->MoveWindow(box_MenuX + padding, box_MenuY + 4 * padding + 40, 100, 20, TRUE);
	pRadioBtn3->MoveWindow(box_MenuX + padding + 150, box_MenuY + 4 * padding + 40, 100, 20, TRUE);
	pRadioBtn2->MoveWindow(box_MenuX + padding, box_MenuY + 5 * padding + 60, 100, 20, TRUE);

	//Picture Control
	CButton *pButtonPlay = (CButton *)GetDlgItem(IDC_BTN_PLAY);
	CButton *pButtonPause = (CButton *)GetDlgItem(IDC_BTN_PAUSE);
	CButton *pButtonStop = (CButton *)GetDlgItem(IDC_BTN_STOP);
	int pictureContorlX = 2 * padding + box_MenuWidth;
	int pictureContorlY = padding;
	int pictureContorlWidth = (dialogWidth - 3 * padding) - box_MenuWidth - 15;
	int pictureContorlHeight = (dialogHeight - 3 * padding)*0.7 - 40;
	pResultImage->MoveWindow(pictureContorlX, pictureContorlY, pictureContorlWidth, pictureContorlHeight, TRUE);
	pButtonPlay->MoveWindow(pictureContorlX + pictureContorlWidth*0.5 - 120 - padding, pictureContorlY + pictureContorlHeight + 10, 80, 20, TRUE);
	pButtonPause->MoveWindow(pictureContorlX + pictureContorlWidth*0.5 - 40, pictureContorlY + pictureContorlHeight + 10, 80, 20, TRUE);
	pButtonStop->MoveWindow(pictureContorlX + pictureContorlWidth*0.5 + 40 + padding, pictureContorlY + pictureContorlHeight + 10, 80, 20, TRUE);

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
	//CWnd *pSegSliderWMIN = GetDlgItem(IDC_SEG_SLIDER_WMIN);
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
	int box_segmentationHeight = ((dialogHeight - 3 * padding)*0.7 - padding) - box_MenuHeight;
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

	int box_syntheticX = padding;
	int box_syntheticY = box_segmentationY + box_segmentationHeight + padding;
	int box_syntheticWidth = dialogWidth - 3 * padding;
	int box_syntheticHeight = (dialogHeight - 3 * padding)*0.3 - 40;
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



	// slider m_sliderSearchStartTime, m_sliderSearchEndTime, m_sliderFps, segment 트랙바 설정

	m_sliderSearchStartTime.SetRange(0, 500);
	m_sliderSearchEndTime.SetRange(0, 500);
	m_sliderFps.SetRange(0, 100);

	//실행시 비디오 파일 불러옴
	loadFile();

	// // Play, Pause버튼 상태 초기화
	isPlayBtnClicked = false;
	isPauseBtnClicked = true;

	// 라디오 버튼 초기화
	CheckRadioButton(IDC_RADIO_PLAY1, IDC_RADIO_PLAY3, IDC_RADIO_PLAY1);
	radioChoice = 0; preRadioChoice = 0; //라디오 버튼의 default는 맨 처음 버튼임

	//edit box default
	m_pEditBoxStartHour->SetWindowTextA("0");
	m_pEditBoxStartMinute->SetWindowTextA("0");

	//slider default
	SetTimer(LOGO_TIMER, 1, NULL);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMFC_SyntheticDlg::loadFile(){
	//파일 다이얼로그 호출해서 segmentation 할 영상 선택	
	char szFilter[] = "Video (*.avi, *.MP4) | *.avi;*.mp4; | All Files(*.*)|*.*||";	//검색 옵션
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter, AfxGetMainWnd());	//파일 다이얼로그 생성
	dlg.DoModal();	//다이얼로그 띄움

	//다이얼로그 종료 시 로딩 창 띄움
	//로딩바 hide하는 순간 메모리해제됨. 그때 그때 사용할 것
	CSplashScreenEx *pSplash; //로딩창
	pSplash = new CSplashScreenEx();
	pSplash->Create(this, "Loading", 0, CSS_FADE | CSS_CENTERAPP | CSS_SHADOW);
	pSplash->SetBitmap(IDB_LOADING, 0, 0, 0);
	pSplash->SetTextFont("Arial", 140, CSS_TEXT_BOLD);
	pSplash->SetTextRect(CRect(148, 38, 228, 70));
	pSplash->SetTextColor(RGB(0, 0, 0));
	pSplash->Show();
	pSplash->SetText("Loading");

	// Path를 받아와서 filename만을 떼어서 저장함(배경파일 이름을 결정할 때 사용)
	CString cstrImgPath = dlg.GetPathName();
	String temp = "File Name : ";
	video_filename = "";
	video_filename = getFileName(cstrImgPath, '\\');
	txt_filename = txt_filename.append(video_filename).append(".txt");

	CWnd *pStringFileName = GetDlgItem(IDC_MENU_STRING_FILE_NAME);

	// 세그먼테이션 데이터(txt, jpg들)를 저장할 디렉토리 유무확인, 없으면 만들어줌
	int tmp = makeDataRootDirectory();
	int tmp2 = makeDataSubDirectory(video_filename);

	// 다시 영상 파일을 불러올 때 cstr 포인터가 메모리 해제가 
	// 재대로 안된 상태이기 때문에 에러가 발생하는 경우가 있음
	// To Do :: cstr 메모리 누수

	char *cstr = new char[temp.length() + 1];
	strcpy(cstr, temp.c_str());
	strcat(cstr, txt_filename.c_str());
	pStringFileName->SetWindowTextA(cstr);
	capture.open((string)cstrImgPath);
	capture_for_background.open((string)cstrImgPath);

	if (!capture.isOpened()) { //예외처리. 해당이름의 파일이 없는 경우
		perror("No Such File!\n");
		::SendMessage(GetSafeHwnd(), WM_CLOSE, NULL, NULL);	//다이얼 로그 종료
	}

	// delete[]cstr;

	// 받아올 영상의 정보들 :: 가로, 세로길이 받아오기
	COLS = (int)capture.get(CV_CAP_PROP_FRAME_WIDTH); //가로 길이
	ROWS = (int)capture.get(CV_CAP_PROP_FRAME_HEIGHT); //세로 길이
	fps = capture.get(CV_CAP_PROP_FPS);

	//비디오 파일 다시 불러올 때 마다 라디오버튼 초기화
	isPlayBtnClicked = false;
	isPauseBtnClicked = true;
	CheckRadioButton(IDC_RADIO_PLAY1, IDC_RADIO_PLAY3, IDC_RADIO_PLAY1);//라디오 버튼 초기화
	radioChoice = 0; preRadioChoice = 0;	//라디오 버튼의 default는 맨 처음 버튼임


	// edit box와 slider 기본 값 불러오기
	loadValueOfSlider(COLS, ROWS, 0, 0); // 일단 startTime과 endTime은 0으로 해놓음

	// 배경 변수 초기화 및 초기 배경 생성
	background = Mat(ROWS, COLS, CV_8UC3);
	background_gray = Mat(ROWS, COLS, CV_8UC1);


	// 배경생성부분
	background_gray = backgroundInit(&capture_for_background, background);

	SetTimer(LOGO_TIMER, 1, NULL);

	//라디오버튼 - 합성영상 활성화 / 비활성화
	if (checkSegmentation()){
		GetDlgItem(IDC_RADIO_PLAY2)->EnableWindow(TRUE);
	}
	else{
		GetDlgItem(IDC_RADIO_PLAY2)->EnableWindow(FALSE);
	}


	//로딩창 제거. 메모리 해제 자동
	pSplash->Hide();
}


// 파일을 불러오면서 각종 slider와 box를 초기화하는 함수
void CMFC_SyntheticDlg::loadValueOfSlider(int captureCols, int captureRows, int startTime, int endTime) {
	//edit box default
	m_pEditBoxStartHour->SetWindowTextA("0");
	m_pEditBoxStartMinute->SetWindowTextA("0");

	// To Do :: startTime, endTime도 매개변수로 받아와서 초기화시켜주기
	// stringstream time_string;
	// time_string = timeConvertor(startTime);

	// time slider default
	SetDlgItemText(IDC_STRING_SEARCH_START_TIME_SLIDER, _T("00 : 00 : 00"));
	SetDlgItemText(IDC_STRING_SEARCH_END_TIME_SLIDER, _T("00 : 00 : 00"));
	SetDlgItemText(IDC_STRING_FPS_SLIDER, to_string(fps).c_str());
	m_sliderSearchStartTime.SetPos(0);
	m_sliderSearchEndTime.SetPos(0);
	m_sliderFps.SetPos(fps);

	SetDlgItemText(IDC_SEG_STRING_VAL_MIN_W, _T("0"));
	SetDlgItemText(IDC_SEG_STRING_VAL_MAX_W, _T("0"));
	SetDlgItemText(IDC_SEG_STRING_VAL_MIN_H, _T("0"));
	SetDlgItemText(IDC_SEG_STRING_VAL_MAX_H, _T("0"));
	m_SliderWMIN.SetPos(0);
	m_SliderWMAX.SetPos(0);
	m_SliderHMIN.SetPos(0);
	m_SliderHMAX.SetPos(0);

	// detection slider range
	m_SliderWMIN.SetRange(0, captureCols);
	m_SliderWMAX.SetRange(0, captureCols);
	m_SliderHMIN.SetRange(0, captureRows);
	m_SliderHMAX.SetRange(0, captureRows);

	// detection slider text
	SetDlgItemText(IDC_SEG_STRING_VAL_MIN_W, _T(to_string(captureCols / 5).c_str()));
	SetDlgItemText(IDC_SEG_STRING_VAL_MAX_W, _T(to_string(captureCols / 2).c_str()));
	SetDlgItemText(IDC_SEG_STRING_VAL_MIN_H, _T(to_string(captureRows / 5).c_str()));
	SetDlgItemText(IDC_SEG_STRING_VAL_MAX_H, _T(to_string(captureRows / 2).c_str()));

	// detection slider default position
	m_SliderWMIN.SetPos(captureCols / 5);
	m_SliderWMAX.SetPos(captureCols / 2);
	m_SliderHMIN.SetPos(captureRows / 5);
	m_SliderHMAX.SetPos(captureRows / 2);
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
		if (MessageBox("프로그램을 종료하시겠습니까??", "S/W Exit", MB_YESNO) == IDYES) {
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
	// cpp파일 내 전역변수들 메모리 해제
	background.release();
	background_gray.release();
	video_filename.clear(); background_filename.clear(); txt_filename.clear();

	delete[] m_segmentArray;

	// CMFC_SyntheticDlg 클래스의 멤버변수들 메모리 해제
	capture.release();
	capture_for_background.release();

	//  layout에서 제공되는 부분을 해제할 경우 오류가 남
	//	free(&m_rectCurHist);
	//	free(m_pEditBoxStartHour);  free(m_pEditBoxStartMinute);

	// To Do :: 열려있는 텍스트 파일 모두 닫음

	PostQuitMessage(0);
}
void CMFC_SyntheticDlg::OnDestroy() {
	printf("OnDestroy\n");
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

// 디스플레이 함수
void CMFC_SyntheticDlg::DisplayImage(int IDC_PICTURE_TARGET, Mat targetMat, int TIMER_ID){
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
	switch (nIDEvent){
	case LOGO_TIMER:	//로고 출력
		if (true){
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
			capture.read(temp_frame);
			//그레이스케일 변환
			cvtColor(temp_frame, temp_frame, CV_RGB2GRAY);
			// 전경 추출
			temp_frame = ExtractFg(temp_frame, background_gray, ROWS, COLS);
			// frame_g = ExtractForegroundToMOG2(frame_g);

			// 이진화
			threshold(temp_frame, temp_frame, 5, 255, CV_THRESH_BINARY);

			// 노이즈 제거
			temp_frame = morphologicalOperation(temp_frame);
			blur(temp_frame, temp_frame, Size(9, 9));
			temp_frame = morphologicalOperation(temp_frame);

			threshold(temp_frame, temp_frame, 5, 255, CV_THRESH_BINARY);

			int numOfLables = connectedComponentsWithStats(temp_frame, img_labels,
				stats, centroids, 8, CV_32S);

			cvtColor(temp_frame, temp_frame, CV_GRAY2BGR);

			//라벨링 된 이미지에 각각 직사각형으로 둘러싸기 
			for (int j = 1; j < numOfLables; j++) {
				int area = stats.at<int>(j, CC_STAT_AREA);
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
		}
		break;

	case SYN_RESULT_TIMER:
		printf("#");
		background_filename = getBackgroundFilePath(video_filename);
		Mat background_loadedFromFile = imread(getBackgroundFilePath(video_filename));//합성 영상을 출력할 때 바탕이 될 프레임. 영상합성 라디오 버튼 클릭 시 자동으로 파일로부터 로드 됨

		// 불러온 배경을 이용하여 합성을 진행
		Mat syntheticResult = getSyntheticFrame(background_loadedFromFile);
		DisplayImage(IDC_RESULT_IMAGE, syntheticResult, SYN_RESULT_TIMER);
		syntheticResult.release();
		background_loadedFromFile.release();
		break;
	}
	temp_frame.release();
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
	if (segmentationTimeInputException(str_startHour, str_startMinute)){
		//로딩 창 띄움
		CSplashScreenEx *pSplash; //로딩창
		pSplash = new CSplashScreenEx();
		pSplash->Create(this, "Loading", 0, CSS_FADE | CSS_CENTERAPP | CSS_SHADOW);
		pSplash->SetBitmap(IDB_LOADING, 0, 0, 0);
		pSplash->SetTextFont("Arial", 110, CSS_TEXT_BOLD);
		pSplash->SetTextRect(CRect(148, 38, 228, 70));
		pSplash->SetTextColor(RGB(0, 0, 0));
		pSplash->Show();
		pSplash->SetText("Pls wait...");
		segmentationOperator(&capture, atoi(str_startHour), atoi(str_startMinute)
			, m_SliderWMIN.GetPos(), m_SliderWMAX.GetPos(), m_SliderHMIN.GetPos(), m_SliderHMAX.GetPos());	//Object Segmentation

		//라디오버튼 - 합성영상 활성화 / 비활성화
		if (checkSegmentation()){
			GetDlgItem(IDC_RADIO_PLAY2)->EnableWindow(TRUE);
		}
		else{
			GetDlgItem(IDC_RADIO_PLAY2)->EnableWindow(FALSE);
		}

		pSplash->Hide();
	}
	else {	// 범위 외 입력시 예외처리
	}
}

// segmentation 기능 수행, 물체 추적 및 파일로 저장
void segmentationOperator(VideoCapture* vc_Source, int videoStartHour, int videoStartMin, int WMIN, int WMAX, int HMIN, int HMAX){
	videoStartMsec = (videoStartHour * 60 + videoStartMin) * 60 * 1000;

	unsigned int COLS = (int)vc_Source->get(CV_CAP_PROP_FRAME_WIDTH);	//가로 길이
	unsigned int ROWS = (int)vc_Source->get(CV_CAP_PROP_FRAME_HEIGHT);	//세로 길이

	// humanDetector Vector
	vector<component> humanDetectedVector, prevHumanDetectedVector;

	/* Mat */
	Mat frame(ROWS, COLS, CV_8UC3); // Mat(height, width, channel)
	Mat frame_g(ROWS, COLS, CV_8UC1);
	Mat tmp_background(ROWS, COLS, CV_8UC3);
	//frame 카운터와 현재 millisecond
	int frameCount = 0;
	unsigned int currentMsec;

	// 배경 초기화
	background_gray = backgroundInit(vc_Source, background);

	// To Do :: 세그먼테이션이 도중에 중단되었을 때 
	// 그 시점에서부터 다시 세그먼테이션을 진행하고 싶을 때 vc_source의 처리

	// 파일 및 디렉터리 정의 부분
	// 세그먼테이션 데이터(txt, jpg들)를 저장할 디렉토리 유무확인, 없으면 만들어줌
	int tmp = makeDataRootDirectory();
	int tmp2 = makeDataSubDirectory(video_filename);

	// 얻어낸 객체 프레임의 정보를 써 낼 텍스트 파일 정의s
	fp = fopen(getTextFilePath(video_filename).c_str(), "w");	// 쓰기모드
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

			// FRAMES_FOR_MAKE_BACKGROUND 갯수 만큼의 프레임을 이용하여 배경 만들기
			if (frameCount % FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND >= FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND - FRAMES_FOR_MAKE_BACKGROUND
				&& frameCount % FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND < FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND - 1) {
				// 배경을 다시 만들 때 첫번쨰 임시배경을 프레임 중 하나로 선택함(연산을 시작하는 첫번쨰 프레임)
				if (frameCount % FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND == FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND - FRAMES_FOR_MAKE_BACKGROUND)
					tmp_background = frame;
				BackgroundMaker(frame, tmp_background, ROWS, COLS);

				if (frameCount % FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND == FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND - 2) {
					// 만든 background 적용
					background = tmp_background;
					//	int check = imwrite(getBackgroundFilePath(video_filename), background);
					cvtColor(background, background_gray, CV_RGB2GRAY);
					printf("Background Changed, %d frame\n", frameCount);
					tmp_background.release();
				}
			}

			//printf("=====%5d 프레임=====\n", frameCount);
			//그레이스케일 변환
			cvtColor(frame, frame_g, CV_RGB2GRAY);

			// 전경 추출
			frame_g = ExtractFg(frame_g, background_gray, ROWS, COLS);

			// 이진화
			threshold(frame_g, frame_g, 5, 255, CV_THRESH_BINARY);

			// 노이즈 제거 및 블러 처리
			frame_g = morphologicalOperation(frame_g);
			blur(frame_g, frame_g, Size(9, 9));
			frame_g = morphologicalOperation(frame_g);

			threshold(frame_g, frame_g, 5, 255, CV_THRESH_BINARY);

			// MAT형으로 라벨링
			humanDetectedVector = connectedComponentsLabelling(frame_g, ROWS, COLS, WMIN, WMAX, HMIN, HMAX);

			// 현재 프레임의 영상 시간 가져오기
			currentMsec = vc_Source->get(CV_CAP_PROP_POS_MSEC);

			// 영상을 처리하여 파일로 저장하기
			humanDetectedVector = humanDetectedProcess(humanDetectedVector, prevHumanDetectedVector,
				frame, frameCount, videoStartMsec, currentMsec, fp);

			if (frameCount % 100 == 10) {
				//	printf(" size = %d %d\n", sizeof(prevHumanDetectedVector), sizeof(component) );

			}

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

			frameCount++;	//increase frame count
		}
	}

	printf("segmentation Operator 끝\n");

	//HWND hWnd = ::FindWindow(NULL, "Dude, Wait");
	//if (hWnd){ ::PostMessage(hWnd, WM_CLOSE, 0, 0); }

	// To Do :: 세그먼테이션 완료하면서 에러가 남
	MessageBox(0, "Done!!", "ding-dong", MB_OK);
	Sleep(2500);

	//메모리 해제
	frame.release(); frame_g.release();

	vector<component>().swap(humanDetectedVector);
	vector<component>().swap(prevHumanDetectedVector);

	fclose(fp);	// 텍스트 파일 닫기
}

// 현재와 이전에 검출한 결과를 비교, true 면 겹칠 수 없음
bool IsComparePrevDetection(vector<component> curr_detected, vector<component> prev_detected, int curr_index, int prev_index) {
	return curr_detected[curr_index].left > prev_detected[prev_index].right
		|| curr_detected[curr_index].right < prev_detected[prev_index].left
		|| curr_detected[curr_index].top > prev_detected[prev_index].bottom
		|| curr_detected[curr_index].bottom < prev_detected[prev_index].top;
}

vector<component> humanDetectedProcess(vector<component> humanDetectedVector, vector<component> prevHumanDetectedVector
	, Mat frame, int frameCount, int videoStartMsec, unsigned int currentMsec, FILE *fp) {

	//printf("cur msec : %d\n", currentMsec);
	int prevTimeTag;
	for (int index = 0; index < humanDetectedVector.size(); index++) {
		// TODO : 현재 프레임에서 이전프레임과 겹치는 obj가 있는지 판단한다. 
		// 이전 오브젝트에 다음오브젝트가 두개가 걸칠 경우 어떻게 처리할 것인가?
		if (!prevHumanDetectedVector.empty()) {	//이전 프레임의 검출된 객체가 있을 경우
			bool findFlag = false;
			for (int j = 0; j < prevHumanDetectedVector.size(); j++) {
				if (!IsComparePrevDetection(humanDetectedVector, prevHumanDetectedVector, index, j)) {	// 두 ROI가 겹칠 경우
					// 이전 TimeTag를 할당
					prevTimeTag = prevHumanDetectedVector[j].timeTag;
					//printf("%d가 겹침\n", prevTimeTag);
					humanDetectedVector[index].timeTag = prevTimeTag;
					saveSegmentationData(video_filename, humanDetectedVector[index], frame
						, prevTimeTag, currentMsec, frameCount, index, fp);

					findFlag = true;
					//break;
				}
			}

			if (findFlag == false) { // 새 객체의 출현
				humanDetectedVector[index].timeTag = currentMsec;
				saveSegmentationData(video_filename, humanDetectedVector[index], frame
					, currentMsec, currentMsec, frameCount, index, fp);

				//printf("새로운 객체 : %s\n", humanDetectedVector[i].fileName);
			}
		}
		else {	// 첫 시행이거나 이전 프레임에 검출된 객체가 없을 경우
			// 새로운 이름 할당
			humanDetectedVector[index].timeTag = currentMsec;
			saveSegmentationData(video_filename, humanDetectedVector[index], frame
				, currentMsec, currentMsec, frameCount, index, fp);
			//printf("***이전프레임 검출 객체 없음\n새로운 객체 : %s\n", humanDetectedVector[i].fileName);
		}
	}
	return humanDetectedVector;
}

// 합성된 프레임을 가져오는 연산
Mat getSyntheticFrame(Mat bgFrame) {
	int *labelMap = (int*)calloc(bgFrame.cols * bgFrame.rows, sizeof(int));	//겹침을 판단하는 용도

	node tempnode;	//DeQueue한 결과를 받을 node
	int countOfObj = segment_queue.count;	//큐 인스턴스의 노드 갯수
	stringstream ss;

	//큐가 비었는지 확인한다. 비었으면 더 이상 출력 할 것이 없는 것 이므로 종료
	if (IsEmpty(&segment_queue)){
		free(labelMap);
		return bgFrame;
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
		BOOL isCross = false;
		int curIndex = tempnode.indexOfSegmentArray;

		//printf("\n%d : %s", i + 1, m_segmentArray[curIndex].fileName);
		//객체가 이전 객체와 겹치는지 비교
		if (i != 0 && m_segmentArray[curIndex].timeTag == m_segmentArray[curIndex].msec){	//처음이 아니고 현재 출력할 객체가 timetag의 첫 프레임 일 때
			for (int j = 0; j < i; j++){	//이전에 그린 객체 모두와 겹치는지 판별
				if (IsObjectOverlapingDetector(m_segmentArray, vectorPreNodeIndex, curIndex, j)){
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
			// 아래 삭제 이후 synthetic으로 옮기기
			//배경에 객체를 올리는 함수
			bgFrame = printObjOnBG(bgFrame, m_segmentArray[tempnode.indexOfSegmentArray], labelMap, video_filename);

			//타임태그를 string으로 변환
			string timetag = "";
			int timetagInSec = (m_segmentArray[tempnode.indexOfSegmentArray].timeTag + videoStartMsec) / 1000;	//영상의 시작시간을 더해준다.
			ss = timeConvertor(timetagInSec);
			timetag = ss.str();

			//커팅된 이미지에 타임태그를 달아준다
			//params : (Mat, String to show, 출력할 위치, 폰트 타입, 폰트 크기, 색상, 굵기) 
			putText(bgFrame, timetag, Point(m_segmentArray[tempnode.indexOfSegmentArray].left + 5, m_segmentArray[tempnode.indexOfSegmentArray].top - 10), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 150, 150), 2);

			//if (m_segmentArray[tempnode.indexOfSegmentArray + 1].timeTag == m_segmentArray[tempnode.indexOfSegmentArray].timeTag) {
			//	Enqueue(&segment_queue, tempnode.timeTag, tempnode.indexOfSegmentArray + 1);
			//}

			//다음 프레임에 같은 타임태그를 가진 객체가 있는지 확인한다. 있으면 EnQueue
			int index = 1;
			while (1){
				//다음 프레임에 도달할 때 까지 아무것도 하지 않는다.
				if (m_segmentArray[tempnode.indexOfSegmentArray].frameCount == m_segmentArray[tempnode.indexOfSegmentArray + index].frameCount){
					index++;
				}
				else{//프레임 번호가 넘어 갔을 경우
					if ((m_segmentArray[tempnode.indexOfSegmentArray].frameCount + 1) == m_segmentArray[tempnode.indexOfSegmentArray + index].frameCount){//다음 객체가 검출된 프레임이 이전 프레임과 1 차이가 날 때
						if (m_segmentArray[tempnode.indexOfSegmentArray].timeTag == m_segmentArray[tempnode.indexOfSegmentArray + index].timeTag && m_segmentArray[tempnode.indexOfSegmentArray].index == m_segmentArray[tempnode.indexOfSegmentArray + index].index) {
							Enqueue(&segment_queue, tempnode.timeTag, tempnode.indexOfSegmentArray + 1);
							break;
						}
						else
							index++;
					}
					else //다음 객체가 있는 프레임이 객체가 있는 프레임과 1차이 이상 날 때
						break;
				}
			}
		}

	}

	free(labelMap);
	vector<int>().swap(vectorPreNodeIndex);
	return bgFrame;
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
	else if (pScrollBar == (CScrollBar*)&m_SliderWMIN)
		SetDlgItemText(IDC_SEG_STRING_VAL_MIN_W, to_string(m_SliderWMIN.GetPos()).c_str());
	else if (pScrollBar == (CScrollBar*)&m_SliderWMAX)
		SetDlgItemText(IDC_SEG_STRING_VAL_MAX_W, to_string(m_SliderWMAX.GetPos()).c_str());
	else if (pScrollBar == (CScrollBar*)&m_SliderHMIN)
		SetDlgItemText(IDC_SEG_STRING_VAL_MIN_H, to_string(m_SliderHMIN.GetPos()).c_str());
	else if (pScrollBar == (CScrollBar*)&m_SliderHMAX)
		SetDlgItemText(IDC_SEG_STRING_VAL_MAX_H, to_string(m_SliderHMAX.GetPos()).c_str());


	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

// Play Button을 눌렀을 경우에 실행되는 콜백
void CMFC_SyntheticDlg::OnClickedBtnPlay()
{
	if (radioChoice == 0 && isPlayBtnClicked == false){//라디오버튼이 원본영상일 경우 - 그냥 재생
		KillTimer(BIN_VIDEO_TIMER);
		KillTimer(SYN_RESULT_TIMER);
		SetTimer(VIDEO_TIMER, 1000 / m_sliderFps.GetPos(), NULL);
		isPlayBtnClicked = true;
		isPauseBtnClicked = false;
	}
	else if (radioChoice == 2 && isPlayBtnClicked == false){//라디오버튼이 이진영상일 경우 - 이진 및 객체 검출 바운더리가 그려진 영상 재생
		KillTimer(SYN_RESULT_TIMER);
		KillTimer(VIDEO_TIMER);
		SetTimer(BIN_VIDEO_TIMER, 1000 / m_sliderFps.GetPos(), NULL);
		isPlayBtnClicked = true;
		isPauseBtnClicked = false;

	}

	else if (radioChoice == 1 && isPlayBtnClicked == false){ //라디오버튼이 합성영상일 경우 - 설정에 따라 합성된 영상 재생
		isPlayBtnClicked = true;
		isPauseBtnClicked = false;

		boolean isSynPlayable = checkSegmentation();

		if (isSynPlayable){
			char *txtBuffer = new char[100];	//텍스트파일 읽을 때 사용할 buffer
			//=======
			//		string path = "./";
			//		path.append(getTextFilePath(video_filename));
			//>>>>>>> merge_error_wooyo

			string path = "./";
			path.append(getTextFilePath(video_filename));

			fp = fopen(path.c_str(), "r");
			//=======
			//		if (fp){	//파일을 제대로 불러왔을 경우
			//			//포인터 끝으로 이동하여 파일 크기 측정
			//			fseek(fp, 0, SEEK_END);
			//			//  디렉토리 체크하는 조건 수정(경로 재 부여) 
			//			if ((isDirectory(getDirectoryPath(video_filename)) && ftell(fp) != 0))	//파일 크기가 0 이 아닐 경우 실행
			//				isPlayable = true;
			//		}
			//>>>>>>> merge_error_wooyo

			//*******************************************텍스트파일을 읽어서 정렬****************************************************************
			m_segmentArray = new segment[BUFFER];  //(segment*)calloc(BUFFER, sizeof(segment));	//텍스트 파일에서 읽은 segment 정보를 저장할 배열 초기화

			segmentCount = 0;
			fseek(fp, 0, SEEK_SET);	//포인터 처음으로 이동
			fgets(txtBuffer, 99, fp);
			sscanf(txtBuffer, "%d", &videoStartMsec);	//텍스트 파일 첫줄에 명시된 실제 영상 시작 시간 받아옴

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
			segment *tmp_segment = new segment; // 임시 segment 동적생성, 메모리 해제에 용의하게 하기
			for (int i = 0; i < segmentCount; i++) {
				for (int j = 0; j < segmentCount - 1; j++) {
					if (m_segmentArray[j].timeTag > m_segmentArray[j + 1].timeTag) {
						// m_segmentArray[segmentCount]와 m_segmentArray[segmentCount + 1]의 교체
						*tmp_segment = m_segmentArray[j + 1];
						m_segmentArray[j + 1] = m_segmentArray[j];
						m_segmentArray[j] = *tmp_segment;
					}
				}
			}

			//정렬 확인 코드
			//{
			//for (int i = 0; i < segmentCount; i++)
			//cout << m_segmentArray[i].fileName << endl;
			//}

			// 임시 버퍼 메모리 해제
			delete tmp_segment;
			delete[] txtBuffer;

			// 텍스트 파일 닫기
			fclose(fp);
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
			int prevIndex = -1;

			//출력할 객체를 큐에 삽입하는 부분
			for (int i = 0; i < segmentCount; i++) {
				//start timetag와 end timetag 사이면 enqueue
				if (m_segmentArray[i].timeTag >= obj1_TimeTag && m_segmentArray[i].timeTag <= obj2_TimeTag) {	//아직 찾지 못했고 일치하는 타임태그를 찾았을 경우
					if (m_segmentArray[i].timeTag == m_segmentArray[i].msec){
						printf("%s\n", m_segmentArray[i].fileName);
						Enqueue(&segment_queue, m_segmentArray[i].timeTag, i);	//출력해야할 객체의 첫 프레임의 타임태그와 위치를 큐에 삽입
						prevTimetag = m_segmentArray[i].timeTag;
						prevIndex = m_segmentArray[i].index;
					}
				}
			}
			/***********/


			//실행중인 타이머 종료
			KillTimer(BIN_VIDEO_TIMER);
			KillTimer(VIDEO_TIMER);
			//타이머 시작	params = timerID, ms, callback함수 명(NULL이면 OnTimer)
			SetTimer(SYN_RESULT_TIMER, 1000 / m_sliderFps.GetPos(), NULL);
		}
		else{ //실행 못하는 경우 segmentation을 진행하라고 출력
			AfxMessageBox("You can't play without segmentation results");
		}
	}
	else{}


}

// segmentation을 할 떄에 입력받는 수의 범위를 한정해주는 함수
bool segmentationTimeInputException(CString str_h, CString str_m) {
	// 시 :: 1~24, 분 :: 1~60
	if ((atoi(str_h) > 0 && atoi(str_h) <= 24)
		&& (atoi(str_m) > 0 && atoi(str_m) <= 60)){
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

void CMFC_SyntheticDlg::SetRadioStatus(UINT value) {
	UpdateData(TRUE);
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

void CMFC_SyntheticDlg::OnBnClickedBtnPause()
{
	// TODO: Add your control notification handler code here
	if (isPauseBtnClicked == false){
		isPlayBtnClicked = false;
		isPauseBtnClicked = true;

		KillTimer(LOGO_TIMER);
		KillTimer(VIDEO_TIMER);
		KillTimer(BIN_VIDEO_TIMER);
		KillTimer(SYN_RESULT_TIMER);
	}

}

bool CMFC_SyntheticDlg::checkSegmentation()
{
	string path = "./";
	path.append(getTextFilePath(video_filename));

	FILE *txtFile = fopen(path.c_str(), "r");

	if (txtFile){	//파일을 제대로 불러왔을 경우
		//포인터 끝으로 이동하여 파일 크기 측정
		fseek(txtFile, 0, SEEK_END);
		if (ftell(txtFile) != 0){	//파일 크기가 0 이 아닐 경우 실행
			fclose(txtFile);
			return true;
		}
		else{ //파일 크기가 0 일 경우
			fclose(txtFile);
			return false;
		}
	}
	else{	//파일을 불러오지 못 할 경우
		printf("\nCan't find txt file");
		return false;
	}
}

Mat backgroundInit(VideoCapture *vc_Source, Mat bg) {
	Mat frame(ROWS, COLS, CV_8UC3); // Mat(height, width, channel)
	Mat bg_gray(ROWS, COLS, CV_8UC1);
	// 영상 시작점으로 초기화
	vc_Source->set(CV_CAP_PROP_POS_MSEC, 0);
	for (int i = 0; i < FRAMES_FOR_MAKE_BACKGROUND; i++) {
		vc_Source->read(frame); //get single frame
		BackgroundMaker(frame, bg, ROWS * 3, COLS);
	}
	// 비디오 파일 이름을 통해서 bg 파일의 이름 만들어서 jpg 파일로 저장
	int background_write_check = imwrite(getBackgroundFilePath(video_filename), bg);
	if (background_write_check)	 printf("First background Making Complete!!\n");

	// 만든 배경을 그레이 변환 후 반환
	cvtColor(bg, bg_gray, CV_RGB2GRAY);
	return bg_gray;
}

void CMFC_SyntheticDlg::OnBnClickedBtnStop()
{
	// TODO: Add your control notification handler code here
	printf("정지 버튼 눌림\n");

	isPlayBtnClicked = false;

	KillTimer(VIDEO_TIMER);
	KillTimer(BIN_VIDEO_TIMER);
	KillTimer(SYN_RESULT_TIMER);

	SetTimer(LOGO_TIMER, 1, NULL);

	capture.set(CV_CAP_PROP_POS_FRAMES, 0);
}

void book(){
	int index[3] = { 1, 2, 3 };
	char* chapter1 = "우용";
	char* chapter2 = "찬혁";
	char* chapter3 = "은혜";

}