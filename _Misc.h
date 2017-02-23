
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();
	enum { IDD = IDD_ABOUTBOX };
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CAboutDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	CString str;
	str.Format( "Release date: %s, %s", __TIME__, __DATE__ );
	GetDlgItem( IDC_RELEASE_DATE )->SetWindowText( str );
	return TRUE;
}

void CFilemanDlg::OnHelpAbout()
{
	CAboutDlg dlg;
	dlg.DoModal();
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()
