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
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();

	int BGMAKINGCOUNTS;
	int BGUPDATECOUNTS;
	CEdit CEditBGMakeCounts;
	CEdit CEditBGUpdateCounts;
};
