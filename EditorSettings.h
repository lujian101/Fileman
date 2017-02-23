#pragma once
#include "afxpropertygridctrl.h"


// EditorSettings dialog

class EditorSettings : public CDialog
{
	DECLARE_DYNAMIC(EditorSettings)

public:
	EditorSettings(CWnd* pParent = NULL);   // standard constructor
	virtual ~EditorSettings();

// Dialog Data
	enum { IDD = IDD_EDITORSETTINGS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	virtual void OnOK();
public:
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnPropertyChanged( WPARAM wParam, LPARAM lParam );
protected:
	CMFCPropertyGridCtrl m_edSettings;
public:
	virtual BOOL OnInitDialog();
	virtual BOOL DestroyWindow();
};
