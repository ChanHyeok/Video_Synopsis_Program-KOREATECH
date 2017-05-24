
// MFC_SyntheticDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "MFC_SyntheticDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define VIDEO_TIMER 1
#define SYN_RESULT_TIMER 2
#define MAX_STR_BUFFER_SIZE  128 // 문자열 출력에 쓸 버퍼 길이

int fps;
Mat m_resultBackground;
segment *m_segmentArray;
int segmentCount;
Queue queue;

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
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFC_SyntheticDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SYN_SLIDER_START_TIME, m_sliderSearchStartTime);
	DDX_Control(pDX, IDC_SYN_SLIDER_END_TIME, m_sliderSearchEndTime);
	DDX_Control(pDX, IDC_SYN_SLIDER_FPS, m_sliderFps);
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
END_MESSAGE_MAP()


// CMFC_SyntheticDlg message handlers

BOOL CMFC_SyntheticDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	ShowWindow(SW_SHOWMAXIMIZED);	//전체화면
	this->GetWindowRect(m_rectCurHist);	//다이얼로그 크기를 얻어옴

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
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
	int controlBoxHeight = dialogHeight*0.25;
	SetWindowPos(&wndTop, 0, 0, dialogWidth, dialogHeight, SWP_NOMOVE);//다이얼로그 크기 조정

	//Picture Control
	int pictureContorlWidth = dialogWidth - padding * 4;
	int pictureContorlHeight = dialogHeight - controlBoxHeight - padding * 3;
	pResultImage->MoveWindow(padding, padding, pictureContorlWidth, pictureContorlHeight, TRUE);

	//group box - segmetation
	CWnd *pGroupSegmentation = GetDlgItem(IDC_GROUP_SEG);
	CWnd *pStringStartTime = GetDlgItem(IDC_SEG_STRING_VIDEO_START_TIME);
	CWnd *pStringColon = GetDlgItem(IDC_SEG_STRING_COLON);
	m_pEditBoxStartHour = (CEdit *)GetDlgItem(IDC_SEG_EDITBOX_START_HOUR);	
	m_pEditBoxStartMinute = (CEdit *)GetDlgItem(IDC_SEG_EDITBOX_START_MINUTE); 
	CButton *pButtonSegmentation = (CButton *)GetDlgItem(IDC_BTN_SEG_SEGMENTATION);
	int box_segmentationX = padding;
	int box_segmentationY = padding*2 + pictureContorlHeight;
	int box_segmentationWidth = pictureContorlWidth*0.5;
	int box_segmentationHeight = controlBoxHeight - padding * 3;
	pGroupSegmentation->MoveWindow(box_segmentationX, box_segmentationY, box_segmentationWidth, box_segmentationHeight, TRUE);
	pStringStartTime->MoveWindow(box_segmentationX + padding, box_segmentationY + box_segmentationHeight*0.3, 200, 20, TRUE);
	m_pEditBoxStartHour->MoveWindow(box_segmentationX + padding, box_segmentationY + box_segmentationHeight*0.3+30, 20, 20, TRUE);
	pStringColon->MoveWindow(box_segmentationX + padding+25, box_segmentationY + box_segmentationHeight*0.3 + 30, 20, 20, TRUE);
	m_pEditBoxStartMinute->MoveWindow(box_segmentationX + padding + 35, box_segmentationY + box_segmentationHeight*0.3 + 30, 20, 20, TRUE);
	pButtonSegmentation->MoveWindow(box_segmentationX + box_segmentationWidth - padding - 100, box_segmentationY + box_segmentationHeight - 30, 100, 20, TRUE);
	

	//group box - synthetic
	CWnd *pGroupSynthetic = GetDlgItem(IDC_GROUP_SYN);
	CWnd *pStringSearchStartTime = GetDlgItem(IDC_STRING_SEARCH_START_TIME);
	CWnd *pStringSearchEndTime = GetDlgItem(IDC_STRING_SEARCH_END_TIME);
	CWnd *pStringFps = GetDlgItem(IDC_STRING_FPS);
	CWnd *pStringSearchStartTimeSlider = GetDlgItem(IDC_STRING_SEARCH_START_TIME_SLIDER);
	CWnd *pStringSearchEndTimeSlider = GetDlgItem(IDC_STRING_SEARCH_END_TIME_SLIDER);
	CWnd *pStringFpsSlider = GetDlgItem(IDC_STRING_FPS_SLIDER);
	CButton *pButtonPlay = (CButton *)GetDlgItem(IDC_BTN_SYN_PLAY);
	int box_syntheticX = padding + box_segmentationWidth;
	int box_syntheticY = padding * 2 + pictureContorlHeight;
	int box_syntheticWidth = pictureContorlWidth*0.5;
	int box_syntheticHeight = controlBoxHeight - padding * 3;
	pGroupSynthetic->MoveWindow(box_syntheticX, box_syntheticY, box_syntheticWidth, box_syntheticHeight, TRUE);
	pStringSearchStartTime->MoveWindow(box_syntheticX + padding, box_syntheticY + box_syntheticHeight*0.3, 100, 20, TRUE);
	m_sliderSearchStartTime.MoveWindow(box_syntheticX + padding, box_syntheticY + box_syntheticHeight*0.3+20+padding, 140, 20, TRUE);
	pStringSearchStartTimeSlider->MoveWindow(box_syntheticX + padding+40, box_syntheticY + box_syntheticHeight*0.3 + 40 + padding*2, 140, 20, TRUE);
	
	pStringSearchEndTime->MoveWindow(box_syntheticX + padding + 150, box_syntheticY + box_syntheticHeight*0.3, 100, 20, TRUE);
	m_sliderSearchEndTime.MoveWindow(box_syntheticX + padding + 150, box_syntheticY + box_syntheticHeight*0.3 + 20 + padding, 140, 20, TRUE);
	pStringSearchEndTimeSlider->MoveWindow(box_syntheticX + padding + 40 + 150, box_syntheticY + box_syntheticHeight*0.3 + 40 + padding * 2, 140, 20, TRUE);

	pStringFps->MoveWindow(box_syntheticX + padding + 300, box_syntheticY + box_syntheticHeight*0.3, 100, 20, TRUE);
	m_sliderFps.MoveWindow(box_syntheticX + padding+300, box_syntheticY + box_syntheticHeight*0.3 + 20 + padding, 140, 20, TRUE);
	pStringFpsSlider->MoveWindow(box_syntheticX + padding + 70 + 300, box_syntheticY + box_syntheticHeight*0.3 + 40 + padding * 2, 30, 20, TRUE);

	pButtonPlay->MoveWindow(box_syntheticX + box_syntheticWidth - padding - 100, box_segmentationY + box_syntheticHeight - 30, 100, 20, TRUE);


	
	/*
	slider m_sliderSearchStartTime, m_sliderSearchEndTime, m_sliderFps 설정
	*/
	m_sliderSearchStartTime.SetRange(0, 500);
	m_sliderSearchEndTime.SetRange(0, 500);
	m_sliderFps.SetRange(0, 100);

	//***************************************************************************************************************
	
	cimage_mfc = NULL;
	isPlayBtnClicked = false;


	//실행시 비디오 읽어옴
	//파일 다이얼로그 호출해서 segmentation 할 영상 선택	
	//TODO : 동영상 확장자로 제한하기
	char szFilter[] = "All Files(*.*)|*.*||";	//검색 옵션
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter, AfxGetMainWnd());	//파일 다이얼로그 생성
	dlg.DoModal();	//다이얼로그 띄움

	CString cstrImgPath = dlg.GetPathName();
	capture.open((string)cstrImgPath);
	if (!capture.isOpened()) { //예외처리. 해당이름의 파일이 없는 경우
		perror("No Such File!\n");
		::SendMessage(GetSafeHwnd(), WM_CLOSE, NULL, NULL);	//다이얼 로그 종료
	}

	fps = capture.get(CV_CAP_PROP_FPS);

	//edit box default
	m_pEditBoxStartHour->SetWindowTextA("0");
	m_pEditBoxStartMinute->SetWindowTextA("0");
	//slider default
	SetDlgItemText(IDC_STRING_SEARCH_START_TIME_SLIDER, _T("00:00:00"));
	SetDlgItemText(IDC_STRING_SEARCH_END_TIME_SLIDER, _T("00:00:00"));
	SetDlgItemText(IDC_STRING_FPS_SLIDER, to_string(fps).c_str());
	m_sliderFps.SetPos(fps);

	return TRUE;  // return TRUE  unless you set the focus to a control
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
	// TODO: Add your control notification handler code here
	AfxMessageBox("OK 버튼 눌림");

	//char szFilter[] = "Image (*.BMP, *.GIF, *.JPG, *.PNG) | *.BMP;*.GIF;*.JPG;*.PNG;*.bmp;*.gif;*.jpg;*.png | All Files(*.*)|*.*||";
	//CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter, AfxGetMainWnd());
	//if (dlg.DoModal() == IDOK)
	//{
	//	CString cstrImgPath = dlg.GetPathName();

	//AfxMessageBox(cstrImgPath); 
	////
	//Mat src = imread(string(cstrImgPath));
	//DisplayImage(IDC_RESULT_IMAGE, src);
	//}

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

	IplImage *tempImage;

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
	// TODO: Add your message handler code here and/or call default

	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CDialogEx::OnTimer(nIDEvent);

	switch (nIDEvent){
	case VIDEO_TIMER:
		printf(".");
		if (isPlayBtnClicked == true){
			//capture->read(mat_frame);
			//DisplayImage(IDC_RESULT_IMAGE, mat_frame);
		}
		break;

	case SYN_RESULT_TIMER:
		printf("+");
		if (isPlayBtnClicked == true){
			//TODO mat에 합성된 결과를 넣어준다.
			Mat syntheticResult;
			syntheticResult = getSyntheticFrame(syntheticResult);
			//capture->read(mat_frame);
			DisplayImage(IDC_RESULT_IMAGE, syntheticResult, SYN_RESULT_TIMER);
			printf("ASD");
			syntheticResult.release();
		}
		break;
	}

}

void CMFC_SyntheticDlg::OnBnClickedBtnSegmentation()
{
	//TODO : Edit box에 문자 입력시 예외처리 (atoi()로는 문자와 null과 숫자 0 이 둘다 0 으로 출력됨)
	//Edit Box로부터 시작 시간과 분을 읽어옴
	CString startHour;
	m_pEditBoxStartHour->GetWindowTextA(startHour);
	CString startMinute;
	m_pEditBoxStartMinute->GetWindowTextA(startMinute);

	humonDetector(&capture, atoi(startHour), atoi(startMinute));	//Object Segmentation
	}


void humonDetector(VideoCapture* vc_Source, int videoStartHour, int videoStartMin)
{
	int videoStartMsec = (videoStartHour * 60 + videoStartMin) * 60 * 1000;

	unsigned int COLS = (int)vc_Source->get(CV_CAP_PROP_FRAME_WIDTH);	//가로 길이
	unsigned int ROWS = (int)vc_Source->get(CV_CAP_PROP_FRAME_HEIGHT);	//세로 길이

	unsigned char* result = (unsigned char*)malloc(sizeof(unsigned char)* ROWS * COLS);

	/* 영상에 텍스트 출력 */
	char strBuffer[MAX_STR_BUFFER_SIZE] = { 0, }; // fps출력
	char timeBuffer[MAX_STR_BUFFER_SIZE] = { 0, }; // time 출력
	Scalar color(0, 0, 255); // B/G/R
	int thickness = 3;	// line thickness

	vector<component> humanDetectedVector;
	vector<component> prevHumanDetectedVector;
	unsigned int currentMsec;



	/* Mat */
	Mat frame(ROWS, COLS, CV_8UC3); // Mat(height, width, channel)
	Mat frame_g(ROWS, COLS, CV_8UC1);
	Mat background(ROWS, COLS, CV_8UC1); // 배경 프레임과 원본 프레임

	//frame 카운터
	int frameCount = 0;

	// 얻어낸 객체 프레임의 정보를 써 낼 텍스트 파일 정의
	FILE *fp; // frameInfo를 작성할 File Pointer
	fp = fopen(RESULT_TEXT_FILENAME, "w");	// 쓰기모드

	// 고정 background 사용
	//TODO 배경 자동 생성하기
	background = imread("background.jpg");
	cvtColor(background, background, CV_RGB2GRAY);

	//MessageBox(0, "Just on second!\nSegmentation in progress...", "Dude, Wait", NULL);

	while (1) {
		vc_Source->read(frame); //get single frame
		if (frame.empty()) {	//예외처리. 프레임이 없음
			perror("Empty Frame");
			break;
		}
		//그레이스케일 변환
		cvtColor(frame, frame_g, CV_RGB2GRAY);

		// 배경 생성
		// background = TemporalMedianBg(frame_g, background, ROWS, COLS);

		// 전경 추출
		frame_g = ExtractFg(frame_g, background, ROWS, COLS);

		// 이진화
		threshold(frame_g, frame_g, 5, 255, CV_THRESH_BINARY);

		// 노이즈 제거
		frame_g = morphologicalOperation(frame_g);
		blur(frame_g, frame_g, Size(9, 9));

		// MAT형으로 라벨링
		humanDetectedVector.clear();
		// humanDetectedVector = connectedComponentLabelling_sequencial(frame_g, ROWS, COLS);
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

Mat morphologicalOperation(Mat img_binary) {
	//morphological opening 작은 점들을 제거
	erode(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	dilate(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

	//morphological closing 영역의 구멍 메우기
	dilate(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	erode(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	return img_binary;
}

string allocatingComponentFilename(vector<component> humanDetectedVector, int timeTag, int currentMsec, int frameCount, int indexOfhumanDetectedVector) {
	string name;
	name.append(to_string(timeTag)).append("_")
		.append(to_string(currentMsec)).append("_")
		.append(to_string(frameCount)).append("_")
		.append(to_string(indexOfhumanDetectedVector));
	return name;
}


vector<component> humanDetectedProcess(vector<component> humanDetectedVector, vector<component> prevHumanDetectedVector
	, Mat frame, int frameCount, int videoStartMsec, unsigned int currentMsec, FILE *fp)
{
	int prevTimeTag;
	for (int i = 0; i < humanDetectedVector.size(); i++) {
		// TODO : 현재 프레임에서 이전프레임과 겹치는 obj가 있는지 판단한다. 
		// 이전 오브젝트에 다음오브젝트가 두개가 걸칠 경우 어떻게 처리할 것인가?
		if (prevHumanDetectedVector.empty() == 0) {	//이전 프레임의 검출된 객체가 있을 경우
			bool findFlag = false;
			for (int j = 0; j < prevHumanDetectedVector.size(); j++) {
				if (humanDetectedVector[i].left < prevHumanDetectedVector[j].right
					&& humanDetectedVector[i].right > prevHumanDetectedVector[j].left
					&& humanDetectedVector[i].top < prevHumanDetectedVector[j].bottom
					&& humanDetectedVector[i].bottom > prevHumanDetectedVector[j].top) {	// 두 ROI가 겹칠 경우

					prevTimeTag = prevHumanDetectedVector[j].timeTag;
					humanDetectedVector[i].fileName = allocatingComponentFilename(humanDetectedVector, prevTimeTag, currentMsec, frameCount, i);

					humanDetectedVector[i].timeTag = prevTimeTag;
					//printf("\n@@@@@@@@@@@@@@@@@@@@@\n %s와 현재 %s 가 겹침%d, %d\n@@@@@@@@@@@@@@@@\n", prevHumanDetectedVector[j].fileName.c_str(), humanDetectedVector[i].fileName.c_str());
					saveSegmentation_JPG(humanDetectedVector[i], frame, frameCount, currentMsec, i, videoStartMsec);
					saveSegmentation_TXT(humanDetectedVector[i], frameCount, currentMsec, fp, i);
					findFlag = true;
					//break;
				}
			}

			if (findFlag == false) {
				humanDetectedVector[i].timeTag = currentMsec;
				humanDetectedVector[i].fileName = allocatingComponentFilename(humanDetectedVector, currentMsec, currentMsec, frameCount, i);
				//printf("\n*********************************\n 새객채 %s 출현\n*********************************\n", humanDetectedVector[i].fileName.c_str());

				saveSegmentation_JPG(humanDetectedVector[i], frame, frameCount, currentMsec, i, videoStartMsec);
				saveSegmentation_TXT(humanDetectedVector[i], frameCount, currentMsec, fp, i);
			}
		}
		else {	// 첫 시행이거나 이전 프레임에 검출된 객체가 없을 경우
			// 새로운 이름 할당
			humanDetectedVector[i].timeTag = currentMsec;
			humanDetectedVector[i].fileName = allocatingComponentFilename(humanDetectedVector, currentMsec, currentMsec, frameCount, i);
			//printf("\n*********************************\n 새객채 %s 출현\n*********************************\n", humanDetectedVector[i].fileName.c_str());
			saveSegmentation_JPG(humanDetectedVector[i], frame, frameCount, currentMsec, i, videoStartMsec);
			saveSegmentation_TXT(humanDetectedVector[i], frameCount, currentMsec, fp, i);
		}
	}
	return humanDetectedVector;
}



Mat getSyntheticFrame(Mat tempBackGround) {
	//출력
		int *labelMap = (int*)calloc(m_resultBackground.cols * m_resultBackground.rows, sizeof(int));	//겹침을 판단하는 용도
		node tempnode;	//DeQueue한 결과를 받을 node
		int countOfObj = queue.count;	//큐 인스턴스의 노드 갯수

		//큐가 비었는지 확인한다
		if (IsEmpty(&queue))
			return tempBackGround;

		m_resultBackground.copyTo(tempBackGround);	//임시로 쓸 배경 복사



		//DeQueue를 큐에 들어있는 객체 갯수 만큼 한다. 
		for (int i = 0; i < countOfObj; i++) {
			//dequeue한 객체를 출력한다.
			tempnode = Dequeue(&queue);

			//if (tempnode.timeTag == 66920)
			//printf("\n@ %d / %s", tempnode.indexOfSegmentArray, m_segmentArray[tempnode.indexOfSegmentArray].fileName);
			//배경에 객체를 올리는 함수
			tempBackGround = printObjOnBG(tempBackGround, m_segmentArray[tempnode.indexOfSegmentArray], labelMap);

			//다음목록에 같은 타임태그를 가진 객체가 있는지 확인한다. 있으면 EnQueue
			if (m_segmentArray[tempnode.indexOfSegmentArray + 1].timeTag == m_segmentArray[tempnode.indexOfSegmentArray].timeTag) {
				Enqueue(&queue, tempnode.timeTag, tempnode.indexOfSegmentArray + 1);
				//printf("@ %d", tempnode.indexOfSegmentArray + 1);
			}

		}

	
		free(labelMap);
	return tempBackGround;
}

//slider control이 움직이면 발생하는 콜백
void CMFC_SyntheticDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default
	if (pScrollBar == (CScrollBar*)&m_sliderSearchStartTime)
		SetDlgItemText(IDC_STRING_SEARCH_START_TIME_SLIDER, timeConvertor(m_sliderSearchStartTime.GetPos()).str().c_str());
	else if (pScrollBar == (CScrollBar*)&m_sliderSearchEndTime)
		SetDlgItemText(IDC_STRING_SEARCH_END_TIME_SLIDER, timeConvertor(m_sliderSearchEndTime.GetPos()).str().c_str());
	else if (pScrollBar == (CScrollBar*)&m_sliderFps)
		SetDlgItemText(IDC_STRING_FPS_SLIDER, to_string(m_sliderFps.GetPos()).c_str());
	

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CMFC_SyntheticDlg::OnClickedBtnSynPlay()
{
	// TODO: Add your control notification handler code here
	isPlayBtnClicked = true;


	//frameInfo.txt 파일이 있고, 내용이 비어있지 않으면 실행가능하다고 표시
	FILE *f;
	string path = "./";
	path.append(RESULT_TEXT_FILENAME);
	f = fopen(path.c_str(), "r");
	boolean isPlayable = false;
	if (f){
		fseek(f, 0, SEEK_END);
		if (ftell(f) != 0)
			isPlayable = true;
	}

	if (isPlayable){	//segment 폴더가 있을 경우에만 실행
		//*******************************************텍스트파일을 읽어서 정렬****************************************************************
		m_segmentArray = new segment[BUFFER];  //(segment*)calloc(BUFFER, sizeof(segment));	//텍스트 파일에서 읽은 segment 정보를 저장할 배열 초기화

		segmentCount = 0;
		char txtBuffer[100] = { 0, };	//텍스트파일 읽을 때 사용할 buffer
		// frameInfo.txt 파일에서 데이터를 추출 하여 segment array 초기화
		FILE *fp = NULL;
		fp = fopen(RESULT_TEXT_FILENAME, "r");
		if (fp == NULL) {	//예외처리. 텍스트 파일을 찾을 수 없음
			perror("No File!");
			exit(1);
		}
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
		InitQueue(&queue);

		// 고정 배경 프레임 불러오기
		m_resultBackground = imread("background.jpg");



		/************************************/
		//TODO timetag 입력받기
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
				Enqueue(&queue, m_segmentArray[i].timeTag, i);	//출력해야할 객체의 첫 프레임의 타임태그와 위치를 큐에 삽입
				prevTimetag = m_segmentArray[i].timeTag;
			}
		}
		/***********/


		//타이머 시작	params = timerID, ms, callback함수 명(NULL이면 OnTimer)
		SetTimer(SYN_RESULT_TIMER, 1000 / m_sliderFps.GetPos(), NULL);
	}
	else{ //segment 폴더가 없을 경우 segmentation을 진행하라고 출력
		AfxMessageBox("You can't play without segmentation results");
	}


}
