#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <fcntl.h>
#include <tchar.h>
#include <io.h>
#include "resource.h"

// g (Global optimization), s (Favor small code), y (No frame pointers).
#pragma optimize("gsy", on)
#pragma comment(linker, "/OPT:NOWIN98")		// Make section alignment really small.
#define WIN32_LEAN_AND_MEAN

#pragma comment(lib, "COMCTL32")
#pragma comment(lib, "Shlwapi")

HWND hDlg;
OPENFILENAME ofn;
TCHAR filename[MAX_PATH]= L"";
HANDLE hFile = NULL;
HINSTANCE hInst;
FILETIME created, last_accessed, last_written;
SYSTEMTIME created_sys, last_accessed_sys, last_written_sys;
SYSTEMTIME created_local, last_accessed_local, last_written_local;

BOOL	CALLBACK Main(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
void	GetFolderPath(HWND hwndOwner,HWND hwndTextBox);
BOOL	SelectFolder(LPTSTR pszSelectedDir, LPTSTR szCaption, HWND hOwner);
void	GetDateTime(void);
void	EnableControls(BOOL bEnable);
BOOL	SetDateTime(void);
HANDLE	OpenFileForRead(LPCWSTR szFileName);
HANDLE	OpenFileForWrite(LPCWSTR szFileName);

int WINAPI WinMain( HINSTANCE hinstCurrent,HINSTANCE hinstPrevious,LPSTR lpszCmdLine,int nCmdShow ){
	hInst = hinstCurrent;
	InitCommonControls();
	DialogBox(hinstCurrent,MAKEINTRESOURCE(IDD_MAIN),NULL,(DLGPROC)Main);
	return 0;
}

BOOL CALLBACK Main(HWND hDlgMain,UINT uMsg,WPARAM wParam,LPARAM lParam){
	HDC				hdc;
	PAINTSTRUCT		ps;

	hDlg = hDlgMain;

	switch( uMsg ){
		case WM_INITDIALOG:
			SendMessageA(hDlg,WM_SETICON,ICON_SMALL, (LPARAM) LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON)));
			SendMessageA(hDlg,WM_SETICON, ICON_BIG,(LPARAM) LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON)));
			CheckDlgButton(hDlg,IDC_OB_CREATED,BST_CHECKED);
			EnableControls(FALSE);
		break;
		case WM_COMMAND:
			if( LOWORD(wParam)==IDC_EXIT ){
				if( hFile!=NULL )
					CloseHandle(hFile);
				EndDialog(hDlg,wParam);
			}else if( LOWORD(wParam)==IDC_OPEN ){
				ZeroMemory(&ofn,sizeof(ofn));
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = hDlg;
				ofn.lpstrFilter = L"All files (*.*)\0*.*\0";
				ofn.lpstrFile = filename;
				ofn.nMaxFile = MAX_PATH;
				ofn.Flags = OFN_EXPLORER|OFN_PATHMUSTEXIST;

				if( GetOpenFileName(&ofn)==TRUE ){
					SetDlgItemText(hDlg, IDC_PATH, filename);
					EnableWindow(GetDlgItem(hDlg, IDC_GETTIME), TRUE);
				}
			}else if(LOWORD(wParam)==IDC_FOLDER){
				GetFolderPath(hDlg, GetDlgItem(hDlg, IDC_PATH));
				EnableWindow(GetDlgItem(hDlg, IDC_GETTIME), TRUE);
			}else if( LOWORD(wParam)==IDC_GETTIME ){
				GetDateTime();
			}else if( LOWORD(wParam)==IDC_SETTIME ){
				SetDateTime();
			}else if( LOWORD(wParam)==IDC_OB_CREATED ){
				GetDateTime();
			}else if( LOWORD(wParam)==IDC_OB_MODIFIED ){
				GetDateTime();
			}else if( LOWORD(wParam)==IDC_OB_ACCESSED ){
				GetDateTime();
			}
		break;
		case WM_PAINT:
			hdc= BeginPaint(hDlg, &ps);
			InvalidateRect(hDlg, NULL, TRUE);
			EndPaint(hDlg, &ps);
		break;
		case WM_CLOSE:
			EndDialog(hDlg,wParam);
			DestroyWindow(hDlg);
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
		break;
	}
	return FALSE;
}

BOOL SetDateTime(void){
	TCHAR		szPath[MAX_PATH];
	FILETIME	ftDateTime, ftSys;
	LPFILETIME	lpftCreation= NULL, lpftAccess= NULL, lpftModified= NULL;
	SYSTEMTIME	stDate, stTime;

	if( GetDlgItemText(hDlg,IDC_PATH,szPath,sizeof(szPath))==0 ){
		return FALSE;
	}
	if( PathFileExists(szPath)==FALSE ){
		return FALSE;
	}

	hFile = OpenFileForWrite(szPath);
	if ( hFile==INVALID_HANDLE_VALUE )
		return FALSE;

	

	if( IsDlgButtonChecked(hDlg,IDC_OB_CREATED)==BST_CHECKED ){
			lpftCreation = &ftDateTime;
	}
	if( IsDlgButtonChecked(hDlg,IDC_OB_ACCESSED)==BST_CHECKED ){
			lpftAccess = &ftDateTime;
	}
	if( IsDlgButtonChecked(hDlg,IDC_OB_MODIFIED)==BST_CHECKED ){
			lpftModified = &ftDateTime;	
	}
	DateTime_GetSystemtime(GetDlgItem(hDlg,IDC_DT_DATE),&stDate);
	DateTime_GetSystemtime(GetDlgItem(hDlg,IDC_DT_TIME),&stTime);
	stDate.wHour= stTime.wHour;
	stDate.wMinute= stTime.wMinute;
	stDate.wSecond= stTime.wSecond;

	SystemTimeToFileTime(&stDate,&ftSys);
	LocalFileTimeToFileTime(&ftSys,&ftDateTime);

	if( SetFileTime(hFile,lpftCreation,lpftAccess,lpftModified)==FALSE ){
		CloseHandle(hFile);
		return FALSE;
	}
	CloseHandle(hFile);
	return TRUE;
}

void GetDateTime(void){
	TCHAR		szPath[MAX_PATH];
	FILETIME	ftCreation, ftAccess, ftModified, ftLocal;
	SYSTEMTIME	stDateTime;
	LPFILETIME	lpFileTime;

	if( GetDlgItemText(hDlg,IDC_PATH,szPath,sizeof(szPath))==0 ){
		EnableControls(FALSE);
		return;
	}
	if( PathFileExists(szPath)==FALSE ){
		EnableControls(FALSE);
		return;
	}
	EnableControls(TRUE);
	
	hFile= OpenFileForRead(szPath);

	if( hFile==INVALID_HANDLE_VALUE ){
		EnableControls(FALSE);
		return;
	}
	EnableControls(TRUE);
	
	if( GetFileTime(hFile,&ftCreation,&ftAccess,&ftModified)==0 ){
		EnableControls(FALSE);
		CloseHandle(hFile);
		return;
	}
	EnableControls(TRUE);
	CloseHandle(hFile);

	if( IsDlgButtonChecked(hDlg,IDC_OB_CREATED)==BST_CHECKED ){
			lpFileTime= &ftCreation;
	}
	if( IsDlgButtonChecked(hDlg,IDC_OB_ACCESSED)==BST_CHECKED ){
			lpFileTime= &ftAccess;
	}
	if( IsDlgButtonChecked(hDlg,IDC_OB_MODIFIED)==BST_CHECKED ){
			lpFileTime= &ftModified;
	}
	FileTimeToLocalFileTime(lpFileTime,&ftLocal);
	FileTimeToSystemTime(&ftLocal,&stDateTime);
	
	DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_DT_DATE),GDT_VALID,&stDateTime);
	DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_DT_TIME),GDT_VALID,&stDateTime);
}

void EnableControls(BOOL bEnable){
	EnableWindow(GetDlgItem(hDlg,IDC_OB_CREATED),bEnable);
	EnableWindow(GetDlgItem(hDlg,IDC_OB_MODIFIED),bEnable);
	EnableWindow(GetDlgItem(hDlg,IDC_OB_ACCESSED),bEnable);

	EnableWindow(GetDlgItem(hDlg,IDC_GETTIME),bEnable);
	EnableWindow(GetDlgItem(hDlg,IDC_SETTIME),bEnable);

	EnableWindow(GetDlgItem(hDlg,IDC_DT_DATE),bEnable);
	EnableWindow(GetDlgItem(hDlg,IDC_DT_TIME),bEnable);
}

HANDLE OpenFileForRead(LPCWSTR szFileName){
	return CreateFile(szFileName, GENERIC_READ,FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
}

HANDLE OpenFileForWrite(LPCWSTR szFileName){
	return CreateFile(szFileName, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ,NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
}

// Get folder path into a text box.
void GetFolderPath(HWND hwndOwner,HWND hwndTextBox){
	TCHAR pstrDir[MAX_PATH];

	// Select a folder.
	if ( SelectFolder(pstrDir, NULL, hwndOwner)==TRUE ){
		SetWindowText(hwndTextBox, pstrDir);// if folder path is selected then set it to text box.
		EnableWindow(GetDlgItem(hDlg, IDC_GETTIME), TRUE);
	} 
}

BOOL SelectFolder(LPTSTR pszSelectedDir, LPTSTR szCaption, HWND hOwner){
    //The BROWSEINFO struct Contains parameters for the SHBrowseForFolder function .
	BROWSEINFO      browseInfo;
    BOOL            bValid= FALSE;
	LPITEMIDLIST	pIDL;
	
	//fill browseInfo (BROWSEINFO struct) with zeros
	ZeroMemory(&browseInfo, sizeof(BROWSEINFO));

    browseInfo.ulFlags=  BIF_RETURNFSANCESTORS;
	browseInfo.hwndOwner= hOwner;
	browseInfo.lpszTitle= szCaption;
  
	// show  the browse for folder dialog box 
	pIDL= SHBrowseForFolder(&browseInfo);

   if( pIDL!=NULL ){
	    // get the path of selected folder  
	   if( (SHGetPathFromIDList(pIDL, pszSelectedDir))!=0 ){
            bValid= TRUE;
	   }
	   //free the item id list allocated by SHBrowseForFolder
	   CoTaskMemFree(pIDL);
    }
    return(bValid);
}