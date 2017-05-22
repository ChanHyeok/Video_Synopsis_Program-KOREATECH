
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
#define MAX_STR_BUFFER_SIZE  128 // 문자열 출력에 쓸 버퍼 길이

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
}

BEGIN_MESSAGE_MAP(CMFC_SyntheticDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CMFC_SyntheticDlg::OnBnClickedOk)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_SEGMENTATION, &CMFC_SyntheticDlg::OnBnClickedBtnSegmentation)
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
	CEdit *pButtonSegmentation = (CEdit *)GetDlgItem(IDC_BTN_SEG_SEGMENTATION);
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
	
	//edit box default
	m_pEditBoxStartHour->SetWindowTextA("0");
	m_pEditBoxStartMinute->SetWindowTextA("0");
	//***************************************************************************************************************
	
	
	//동영상 읽기
	capture = new VideoCapture("test.mp4");

	if (!capture->isOpened())
	{
		MessageBox(_T("비디오를 열수 없습니다. \n"));
	}

	cimage_mfc = NULL;

	isPlayBtnClicked = false;

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
	//AfxMessageBox("OK 버튼 눌림");

	
	isPlayBtnClicked = true; 
	SetTimer(VIDEO_TIMER, 25, NULL); //params = timerID, ms, callback함수 명(NULL이면 OnTimer)


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
void CMFC_SyntheticDlg::DisplayImage(int IDC_PICTURE_TARGET, Mat targetMat){

	if (targetMat.empty()) {	//예외처리. 프레임이 없음
		perror("Empty Frame");
		KillTimer(VIDEO_TIMER);
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


void CMFC_SyntheticDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default

	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CDialogEx::OnTimer(nIDEvent);

	switch (nIDEvent){
	case VIDEO_TIMER :
		printf(".");
		if (isPlayBtnClicked == true){
			capture->read(mat_frame);
			DisplayImage(IDC_RESULT_IMAGE, mat_frame);
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


	//파일 다이얼로그 호출해서 segmentation 할 영상 선택	
	//TODO : 동영상 확장자로 제한하기
	char szFilter[] = "All Files(*.*)|*.*||";	//검색 옵션
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter, AfxGetMainWnd());	//파일 다이얼로그 생성
	dlg.DoModal();	//다이얼로그 띄움
	
	CString cstrImgPath = dlg.GetPathName();

	VideoCapture vcSource((string)cstrImgPath);
	if (!vcSource.isOpened()) { //예외처리. 해당이름의 파일이 없는 경우
		perror("No Such File!\n");
		return ;
	}
	else{
		humonDetector(vcSource, atoi(startHour), atoi(startMinute));	//Object Segmentation
	}
}


void humonDetector(VideoCapture vc_Source, int videoStartHour, int videoStartMin)
{
	int videoStartMsec = (videoStartHour * 60 + videoStartMin) * 60 * 1000;

	unsigned int COLS = (int)vc_Source.get(CV_CAP_PROP_FRAME_WIDTH);	//가로 길이
	unsigned int ROWS = (int)vc_Source.get(CV_CAP_PROP_FRAME_HEIGHT);	//세로 길이

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
	fp = fopen("frameInfo.txt", "w");	// 쓰기모드

	// 고정 background 사용
	//TODO 배경 자동 생성하기
	background = imread("background.jpg");
	cvtColor(background, background, CV_RGB2GRAY);

	//MessageBox(0, "Just on second!\nSegmentation in progress...", "Dude, Wait", NULL);

	while (1) {
		vc_Source >> frame; //get single frame
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
		currentMsec = vc_Source.get(CV_CAP_PROP_POS_MSEC);

		// 영상을 처리하여 파일로 저장하기
		humanDetectedVector = humanDetectedProcess(humanDetectedVector, prevHumanDetectedVector,
			frame, frameCount, videoStartMsec, currentMsec, fp);

		// 현재 검출한 데이터를 이전 데이터에 저장하기
		prevHumanDetectedVector = humanDetectedVector;

		frameCount++;	//increase frame count
	}
	fclose(fp);	// 텍스트 파일 닫기
	//HWND hWnd = ::FindWindow(NULL, "Dude, Wait");
	//if (hWnd){ ::PostMessage(hWnd, WM_CLOSE, 0, 0); }
	MessageBox(0, "Done!!", "ding-dong", MB_OK);
}

Mat morphologicalOperation(Mat img_binary) {
	Mat img_result;
	//morphological opening 작은 점들을 제거
	erode(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	dilate(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

	//morphological closing 영역의 구멍 메우기
	dilate(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	erode(img_binary, img_result, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	return img_result;
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