// FileZilla Server - a Windows ftp server

// Copyright (C) 2002-2016 - Tim Kosse <tim.kosse@filezilla-project.org>

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#if !defined(AFX_OPTIONSPAGE_H__3E30257D_89B3_4C59_9320_B26172C96CF5__INCLUDED_)
#define AFX_OPTIONSPAGE_H__3E30257D_89B3_4C59_9320_B26172C96CF5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OptionsPage.h : Header-Datei
//

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld COptionsPage

class COptionsDlg;

class COptionsPage : public CSAPrefsSubDlg
{
// Konstruktion
public:
	COptionsPage(COptionsDlg *pOptionsDlg, UINT nID, CWnd *pParent = NULL);   // Standardkonstruktor

	virtual BOOL IsDataValid();
	virtual void SaveData();
	virtual void LoadData();

// Dialogfelddaten
	//{{AFX_DATA(COptionsPage)
		// HINWEIS: Der Klassen-Assistent fügt hier Datenelemente ein
	//}}AFX_DATA


// Überschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
	//{{AFX_VIRTUAL(COptionsPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV-Unterstützung
	//}}AFX_VIRTUAL

// Implementierung
protected:

	COptionsDlg *m_pOptionsDlg;

	// Generierte Nachrichtenzuordnungsfunktionen
	//{{AFX_MSG(COptionsPage)
		// HINWEIS: Der Klassen-Assistent fügt hier Member-Funktionen ein
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_OPTIONSPAGE_H__3E30257D_89B3_4C59_9320_B26172C96CF5__INCLUDED_
