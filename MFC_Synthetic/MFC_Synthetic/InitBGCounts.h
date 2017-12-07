/*
	영상을 로드해서 배경을 만들 프레임 갯수, 배경을 유지시킬 프레임 갯수를 입력받는 대화상자를 구현한 파일입니다.
	대화상자를 띄우고 OK, Cancel 리스너, 두 개의 EditText가 있습니다.
	배경의 만들 사전 프레임의 갯수를 받는 것은 디폴트로 800이 설정되어 있습니다.
	배경의 업데이트를 주기를 정하는 프레임의 갯수는 디폴트로 1000이 설정되어 있습니다.
*/

#pragma once
#include "afxwin.h"
// CInitBGCounts dialog

class CInitBGCounts : public CDialogEx
{
	DECLARE_DYNAMIC(CInitBGCounts)

public:
	CInitBGCounts(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInitBGCounts();

// Dialog Data
	enum { IDD = IDD_MFC_BG_COUNT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	int BGMAKINGCOUNTS;
	int BGUPDATECOUNTS;
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();

	int getBGMAKINGCOUNTS();
	int getBGUPDATECOUNTS();
	CEdit CEditBGMakeCounts;
	CEdit CEditBGUpdateCounts;


};
