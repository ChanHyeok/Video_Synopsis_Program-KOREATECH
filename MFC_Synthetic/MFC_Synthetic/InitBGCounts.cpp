// InitBGCounts.cpp : implementation file
//

#include "stdafx.h"
#include "MFC_Synthetic.h"
#include "InitBGCounts.h"
#include "afxdialogex.h"
#include "MFC_SyntheticDlg.h"


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

	//Static Text Font
	m_font.CreateFont(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SWISS, "굴림체");

	GetDlgItem(IDC_STATIC0)->SetFont(&m_font);
	GetDlgItem(IDC_STATIC1)->SetFont(&m_font);

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
	

	m_font.DeleteObject();
	CDialogEx::OnCancel();
}



void CInitBGCounts::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	CString temp1, temp2;
	CEditBGMakeCounts.GetWindowTextA(temp1);
	CEditBGUpdateCounts.GetWindowTextA(temp2);

	// 배경 디폴트 값 설정부분
	BGMAKINGCOUNTS = atoi(temp1);
	BGUPDATECOUNTS = atoi(temp2);
	CDialogEx::OnOK();
}
