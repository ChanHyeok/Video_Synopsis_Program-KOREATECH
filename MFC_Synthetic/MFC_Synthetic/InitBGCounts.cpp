/*
	영상을 로드해서 배경을 만들 프레임 갯수, 배경을 유지시킬 프레임 갯수를 입력받는 대화상자를 구현한 파일입니다.
	자세한 설명은 헤더파일에 기록해 두었습니다.
*/

// InitBGCounts.cpp : implementation file
//

#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "InitBGCounts.h"
#include "afxdialogex.h"


// CInitBGCounts dialog

IMPLEMENT_DYNAMIC(CInitBGCounts, CDialogEx)

CInitBGCounts::CInitBGCounts(CWnd* pParent /*=NULL*/)
	: CDialogEx(CInitBGCounts::IDD, pParent)
{
	m_pParentWnd = pParent; // 생성자 부분에 부모핸들을 저장
}

CInitBGCounts::~CInitBGCounts()
{
}

void CInitBGCounts::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, CEditBGMakeCounts);
	DDX_Control(pDX, IDC_EDIT2, CEditBGUpdateCounts);

	CEditBGMakeCounts.SetWindowText("800");
	CEditBGUpdateCounts.SetWindowText("1000");
}


BEGIN_MESSAGE_MAP(CInitBGCounts, CDialogEx)
	ON_BN_CLICKED(IDOK, &CInitBGCounts::OnBnClickedOk)
END_MESSAGE_MAP()


// CInitBGCounts message handlers
BOOL CInitBGCounts::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CInitBGCounts::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	//엔터 및 esc 키 막기
	if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE) return TRUE;

	return CDialogEx::PreTranslateMessage(pMsg);
}


void CInitBGCounts::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class

	CDialogEx::OnOK();
}


void CInitBGCounts::OnCancel()
{
	// TODO: Add your specialized code here and/or call the base class


	CDialogEx::OnCancel();
}



void CInitBGCounts::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	CString temp1, temp2;
	CEditBGMakeCounts.GetWindowTextA(temp1);
	CEditBGUpdateCounts.GetWindowTextA(temp2);

	// 배경 디폴트 값 설정부분
	if (atoi(temp1) > 0 && atoi(temp2) > 0) {
		BGMAKINGCOUNTS = atoi(temp1);
		BGUPDATECOUNTS = atoi(temp2);
	}
	CDialogEx::OnOK();
}

int CInitBGCounts::getBGMAKINGCOUNTS() {
	return BGMAKINGCOUNTS;
}
int CInitBGCounts::getBGUPDATECOUNTS() {
	return BGUPDATECOUNTS;
}