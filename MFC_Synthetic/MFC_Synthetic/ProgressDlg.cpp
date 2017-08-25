// ProgressDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "ProgressDlg.h"
#include "afxdialogex.h"

const int PROGRESS_TIMER = 0;
const int PROGRESS_TIMER_SEG = 1;
const int PROGRESS_TIMER_SAVE = 2;

// CProgressDlg dialog

IMPLEMENT_DYNAMIC(CProgressDlg, CDialogEx)

CProgressDlg::CProgressDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CProgressDlg::IDD, pParent)
{
	m_pParentWnd = pParent; // 생성자 부분에 부모핸들을 저장
}

CProgressDlg::~CProgressDlg()
{
}

void CProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS, m_ProgressCtrl);
	DDX_Control(pDX, IDC_MESSAGE, m_StaticMessage);
	DDX_Control(pDX, IDMYOK, m_ButtonOK);
}


BEGIN_MESSAGE_MAP(CProgressDlg, CDialogEx)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDMYOK, &CProgressDlg::OnBnClickedMyok)
	ON_BN_CLICKED(IDMYCANCEL, &CProgressDlg::OnBnClickedMycancel)
END_MESSAGE_MAP()


// CProgressDlg message handlers

BOOL CProgressDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowText(_T("Loading"));
	isWorkCompleted = false;
	m_ButtonOK.EnableWindow(false);
	vc_Source.open(videoFilePath);
	totalFrame = (int)vc_Source.get(CV_CAP_PROP_FRAME_COUNT);
	vc_Source.set(CV_CAP_PROP_POS_MSEC, 0);	// 영상 시작점으로 초기화

	frame = Mat(ROWS, COLS, CV_8UC3);
	bg_gray = Mat(ROWS, COLS, CV_8UC1);

	string str;
	int segmentCount;

	switch (mode){
	case PROGRESS_TIMER:	//bg init
		 // Mat(height, width, channel)
		bg = Mat(ROWS, COLS, CV_8UC3);
		
		count = 0;

		if (totalFrame < FRAMES_FOR_MAKE_BACKGROUND){
			FRAMES_FOR_MAKE_BACKGROUND = totalFrame;
		}
		m_ProgressCtrl.SetRange(0, FRAMES_FOR_MAKE_BACKGROUND - 1);

		//첫 프레임
		vc_Source.read(bg);

		//첫 프레임 컬러로 저장(이진영상 출력 배경용)
		if (imwrite(getColorBackgroundFilePath(fileNameNoExtension), bg)){
			printf("Color Background Saved Completed\n");
		}

		cvtColor(bg, bg_gray, CV_RGB2GRAY);

		SetTimer(PROGRESS_TIMER, 0, NULL);
		break;
	case PROGRESS_TIMER_SEG:	//segmentation
		m_ProgressCtrl.SetRange(0, totalFrame-1);
		frame_g = Mat(ROWS, COLS, CV_8UC1);
		InitComponentVectorQueue(&prevHumanDetectedVector_queue);
		count = 0;
		
		// 얻어낸 객체 프레임의 정보를 써 낼 텍스트 파일 정의
		if ((fp = fopen(getTextFilePath(fileNameNoExtension).c_str(), "r+")) == NULL)
			fp = fopen(getTextFilePath(fileNameNoExtension).c_str(), "w+");
		fprintf(fp, to_string(videoStartMsec).append("\n").c_str());	//첫줄에 영상시작시간 적어줌
		
		SetTimer(PROGRESS_TIMER_SEG, 0, NULL);
		break;
	case PROGRESS_TIMER_SAVE:	//save
		segmentArray = new segment[BUFFER];
		segmentCount = readSegmentTxtFile(segmentArray);

		frame = imread(getColorBackgroundFilePath(fileNameNoExtension));

		
		//큐 초기화
		InitQueue(&segment_queue);

		//출력할 객체를 큐에 삽입하는 부분
		for (int i = 0; i < segmentCount; i++) {
			// start timetag와 end timetag 사이면 enqueue
			// 아직 찾지 못했고 일치하는 타임태그를 찾았을 경우
			if (segmentArray[i].timeTag >= obj1_TimeTag && segmentArray[i].timeTag <= obj2_TimeTag) {
				if (segmentArray[i].timeTag == segmentArray[i].msec && ((CMFC_SyntheticDlg *)GetParent())->isDirectionAndColorMatch(segmentArray[i])) {
					//출력해야할 객체의 첫 프레임의 타임태그와 위치를 큐에 삽입
					segmentArray[i].first_timeTagFlag = true;
					Enqueue(&segment_queue, segmentArray[i], i);
				}
			}
			//탐색 중, obj2_TimeTag을 넘으면 진행 완료
			else if (segmentArray[i].timeTag > obj2_TimeTag) {
				break;
			}
		}

		//파일로 동영상을 저장하기 위한 준비  
		str = "./data/";
		str.append(fileNameNoExtension).append("/").append(fileNameNoExtension).append("_").append(currentDateTime()).append(".avi");
		outputVideo.open(str, VideoWriter::fourcc('X', 'V', 'I', 'D'),25, Size((int)frame.cols, (int)frame.rows), true);

		if (!outputVideo.isOpened())
		{
			cout << "동영상을 저장하기 위한 초기화 작업 중 에러 발생" << endl;
		}
		else {
			SetTimer(PROGRESS_TIMER_SAVE, 0, NULL);
		}
		break;
	default:
			break;
	}
	

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CProgressDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	CDialogEx::OnTimer(nIDEvent);
	string text;
	switch (nIDEvent) {
	case PROGRESS_TIMER:
		text = "("; text.append(to_string(count)).append("/").append(to_string(FRAMES_FOR_MAKE_BACKGROUND - 1)).append(")...배경 생성 중");
		m_StaticMessage.SetWindowTextA(text.c_str());
		if (count < FRAMES_FOR_MAKE_BACKGROUND - 1){
			vc_Source.read(frame); //get single frame
			cvtColor(frame, frame, CV_RGB2GRAY);
			temporalMedianBG(frame, bg_gray);
			m_ProgressCtrl.OffsetPos(1);
			count++;
		}
		else{
			KillTimer(PROGRESS_TIMER);
			if (imwrite(getBackgroundFilePath(fileNameNoExtension), bg_gray)){
				imwrite(getTempBackgroundFilePath(fileNameNoExtension), bg_gray);
				m_StaticMessage.SetWindowTextA(_T("배경 생성 성공!"));
				m_ButtonOK.EnableWindow(true);
				printf("Background Init Completed\n");
				isWorkCompleted = true;
			}
			else{
				m_StaticMessage.SetWindowTextA(_T("배경 생성 실패"));
				m_ButtonOK.EnableWindow(true);
				printf("Background Init Failed!!\n");
			}
		}
		break;
	case PROGRESS_TIMER_SEG:
		if (true){
			text = "("; text.append(to_string(count)).append("/").append(to_string(totalFrame-1)).append(")...세그멘테이션 진행 중");
			m_StaticMessage.SetWindowTextA(text.c_str());
			vc_Source.read(frame); //get single frame
			if (frame.empty()) {	//예외처리. 프레임이 없음
				perror("Empty Frame");
				m_ButtonOK.EnableWindow(true);
				isWorkCompleted = true;
				KillTimer(PROGRESS_TIMER_SEG);
				OnCancel();
				break;
			}

			int curFrameCount = (int)vc_Source.get(CV_CAP_PROP_POS_FRAMES);
			int curFrameCount_nomalized = curFrameCount%FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND;

			//그레이스케일 변환
			cvtColor(frame, frame_g, CV_RGB2GRAY);

			//다음에 쓸 배경을 만들어야 할 경우
			if (curFrameCount_nomalized >= (FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND - FRAMES_FOR_MAKE_BACKGROUND)){
				if (curFrameCount_nomalized == (FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND - FRAMES_FOR_MAKE_BACKGROUND)){	//새로 만드는 첫 배경 Init
					printf("Background Making Start : %d frame\n", curFrameCount);
					bg_gray = frame_g.clone();
				}
				else{	//배경 생성
					temporalMedianBG(frame_g, bg_gray);
				}
			}

			//만든 배경을 저장해야 할 경우
			if (curFrameCount_nomalized == FRAMECOUNT_FOR_MAKE_DYNAMIC_BACKGROUND - 1){
				imwrite(getTempBackgroundFilePath(fileNameNoExtension), bg_gray);
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

			// 영상을 처리하여 파일로 저장하기
			// humanDetectedVector = humanDetectedProcess(humanDetectedVector, prevHumanDetectedVector,
			//	frame, frameCount, videoStartMsec, currentMsec, fp);
			// humanDetected가 있을 경우에만 연산(함수호출 오버헤드 감소를 위함)
			// 영상을 처리하여 타임태그를 새로 부여하고 파일로 저장하기(2)
			if (humanDetectedVector.size() > 0)
				humanDetectedVector = humanDetectedProcess2(humanDetectedVector, prevHumanDetectedVector
				, prevHumanDetectedVector_queue, frame, count, (int)vc_Source.get(CV_CAP_PROP_POS_MSEC), fp, frame_g);

			// 큐가 full일 경우 한자리 비워주기
			if (IsComponentVectorQueueFull(&prevHumanDetectedVector_queue))
				RemoveComponentVectorQueue(&prevHumanDetectedVector_queue);

			// 큐에 매 수행마다 벡터를 무조건 넣어줘야함
			PutComponentVectorQueue(&prevHumanDetectedVector_queue, humanDetectedVector);

			// 벡터 메모리 해제를 빈 벡터 생성(prevHumanDetectedVector 메모리 해제)
			vector<component>().swap(prevHumanDetectedVector);

			// 현재 검출한 데이터를 이전 데이터에 저장하기
			prevHumanDetectedVector = humanDetectedVector;

			// 벡터 메모리 해제를 빈 벡터 생성
			vector<component>().swap(humanDetectedVector);

			m_ProgressCtrl.OffsetPos(1);
			count++;//increase frame count
		}
		break;
	case PROGRESS_TIMER_SAVE:
		text = "...합성 영상 저장 중"; 
		m_StaticMessage.SetWindowTextA(text.c_str());
		if (true){
			Mat bg_copy = frame.clone();
	
			// 불러온 배경을 이용하여 합성을 진행
			Mat syntheticResult = ((CMFC_SyntheticDlg *)GetParent())->getSyntheticFrame(&segment_queue, bg_copy, segmentArray);
			if (segment_queue.count == 0){
				printf("영상 저장 끝\n");
				text = "...합성 영상 저장 완료";
				m_StaticMessage.SetWindowTextA(text.c_str());
				KillTimer(PROGRESS_TIMER_SAVE);
				m_ButtonOK.EnableWindow(true);
				isWorkCompleted = true;
				syntheticResult = NULL; syntheticResult.release();
				bg_copy = NULL; bg_copy.release();
				outputVideo.release();
				break;
			}
			else {
				outputVideo << syntheticResult;
				syntheticResult = NULL; syntheticResult.release();
				bg_copy = NULL; bg_copy.release();
			}
		}
		break;
	default:
		break;
	}
}


void CProgressDlg::OnBnClickedMyok()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}


void CProgressDlg::OnBnClickedMycancel()
{
	// TODO: Add your control notification handler code here
}


BOOL CProgressDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	//엔터 및 esc 키 막기
	if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE) return TRUE;

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CProgressDlg::OnCancel()
{
	// TODO: Add your specialized code here and/or call the base class
	CDialogEx::OnCancel();
	vc_Source = NULL; vc_Source.release();
	frame = NULL; frame.release();
	bg = NULL; bg.release();
	frame_g = NULL;	frame_g.release();

	vector<component>().swap(humanDetectedVector);
	vector<component>().swap(prevHumanDetectedVector);

	segmentArray = NULL;
	delete[] segmentArray;

	if (mode == 1 && fp != NULL)
		fclose(fp);

	//정상적으로 작업이 완료되지 않았을 경우 강제 종료
	if (!isWorkCompleted){
		((CMFC_SyntheticDlg *)GetParent())->OnCancel();
	}
}
