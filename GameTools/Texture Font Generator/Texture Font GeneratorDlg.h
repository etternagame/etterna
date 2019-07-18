#pragma once
#include <afxwin.h>
#include <afxcmn.h>

#include "TextureFont.h"

class CTextureFontGeneratorDlg : public CDialog
{
	DECLARE_DYNAMIC(CTextureFontGeneratorDlg);

  public:
	CTextureFontGeneratorDlg(CWnd* pParent = NULL); // standard constructor
	virtual ~CTextureFontGeneratorDlg();

	enum
	{
		IDD = IDD_TEXTUREFONTGENERATOR_DIALOG
	};

  protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support

	HICON m_hIcon;
	CFont m_Font;

	BOOL CanExit();

	bool m_bUpdateFontNeeded;
	bool m_bUpdateFontViewAndCloseUpNeeded;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	DECLARE_MESSAGE_MAP()

	void UpdateFontViewAndCloseUp();
	void UpdateFont(bool bSavingDoubleRes);
	void UpdateCloseUp();

  public:
	vector<FontPageDescription> m_PagesToGenerate;

	afx_msg void OnCbnSelchangeShownPage();
	afx_msg void OnCbnSelchangeFamilyList();
	afx_msg void OnEnChangeFontSize();
	afx_msg void OnStyleAntialiased();
	afx_msg void OnStyleBold();
	afx_msg void OnStyleItalic();
	afx_msg void OnDeltaposSpinTop(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinBaseline(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangePadding();
	afx_msg void OnFileSave();
	afx_msg void OnFileExit();

	static int CALLBACK
	EnumFontFamiliesCallback(const LOGFONTA* pLogicalFontData,
							 const TEXTMETRICA* pPhysicalFontData,
							 DWORD FontType,
							 LPARAM lParam);

	CStatic m_FontView;
	CComboBox m_ShownPage;
	CComboBox m_FamilyList;
	CStatic m_TextOverlap;
	CEdit m_FontSize;
	CEdit m_Padding;
	CStatic m_ErrorOrWarning;
	CStatic m_CloseUp;
	CSpinButtonCtrl m_SpinTop;
	CSpinButtonCtrl m_SpinBaseline;
	CStatic m_FontType;
	afx_msg void OnOptionsDoubleres();
	afx_msg void OnOptionsExportstroketemplates();
	afx_msg void OnOptionsNumbersonly();
};
