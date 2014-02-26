/* このプログラムはWindowsのプリンタドライバー経由で印刷命令を直接プリンタに送り込む
 * プログラムのサンプルです。あくまでサンプルということで次のように単純化しています：
 * 
 * ・1ページ目の印刷データは'A'だけ
 * ・    ：
 * ・26ページ目の印刷データは'Z'だけ
 * ・部の区切りは"\r\n"
 * 
 * 印刷ダイアログで「ファイルに出力する」のチェックを付けて動作確認することをオススメします。
 * 
 *   2014-02-26 Suguru YAMAMOTO
 */
#include <windows.h>
#include <stdio.h>

static const char g_alphabets[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
#define IsEmpty(str)	((str)[0] == '\0')

int main( int argc, char* argv[] )
{
	int			rc;
	HANDLE		hPrinter = NULL;
	PRINTDLG	pd;
	DEVNAMES*	printerDevName = NULL;
	char		printerName[256];
	char		fileNameForPrintToFile[MAX_PATH+1];
	PRINTER_DEFAULTS	printerDefaults;
	int			copyCount;
	int			pageNum;
	DOC_INFO_1	docInfo;
	DWORD		jobID;

	UNREFERENCED_PARAMETER( argc ); // 警告殺し（使われない引数）
	UNREFERENCED_PARAMETER( argv ); // 警告殺し（使われない引数）

	// 準備
	fileNameForPrintToFile[0] = '\0';

	// プリンタをユーザに選択してもらう
	memset( &pd, 0, sizeof(pd) );
	pd.lStructSize = sizeof(pd);
	pd.hwndOwner = NULL; // プリンタダイアログの親ウィンドウ。このプログラムはコンソールなので無し。
	pd.Flags = 0//PD_PAGENUMS	// ページ番号の範囲指定をユーザが指定可能にする場合は指定
			   //| PD_HIDEPRINTTOFILE	// 「ファイルに出力する」を隠す場合はこれを指定。デバッグ中は表示した方が良いと思います。
			   ;
	pd.nMinPage = 1;	// 指定可能なページ番号の最小値
	pd.nMaxPage = 26;	// 指定可能なページ番号の最大値
	pd.nFromPage = 1;	// 最初の印刷ページ番号としてダイアログに初期表示する値
	pd.nToPage = 26;	// 最後に印刷ページ番号としてダイアログに初期表示する値
	rc = PrintDlg( &pd );
	if( rc == FALSE )
	{
		fprintf( stderr, "Failed to get printer information from the dialog. (LE:%d)\n", GetLastError() );
		goto cleanup;
	}
	printf( "pd.Flags: %08x\n", pd.Flags );
	printf( "From: page %d.\n", pd.nFromPage );	// ユーザが指定した印刷開始ページ番号
	printf( "To: page %d.\n", pd.nToPage );	// ユーザが指定した印刷終了ページ番号
	printf( "%d copies.\n", pd.nCopies ); // ユーザが指定した印刷部数
	printf( "Print all pages: %d\n", (pd.Flags & PD_ALLPAGES) != 0 ); // すべて印刷
	printf( "Print specific pages: %d\n", (pd.Flags & PD_PAGENUMS) != 0 ); // 指定したページだけ印刷
	printf( "Print selection: %d\n", (pd.Flags & PD_SELECTION) != 0 ); // 選択範囲の印刷

	// 「ファイルに出力する」がチェックされていた場合、出力先のファイル名をユーザに尋ねる
	if( (pd.Flags & PD_PRINTTOFILE) != 0 )
	{
		OPENFILENAME	ofn;

		memset( &ofn, 0, sizeof(ofn) );
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = NULL; // 親ウィンドウ。このプログラムはコンソールなので無し。
		ofn.lpstrFile = fileNameForPrintToFile;
		ofn.nMaxFile = sizeof(fileNameForPrintToFile) - 1;
		rc = GetSaveFileName( &ofn );
		if( rc == FALSE )
		{
			fprintf( stderr, "Failed to get name of the file to write print data. (LE:%d, ExtErr:0x%04x)\n", GetLastError(), CommDlgExtendedError() );
			goto cleanup;
		}
	}

	// プリンタ名は移動可能なメモリ領域（ハンドル）に割り当てられるので固定して取得
	printerDevName = (DEVNAMES*)GlobalLock( pd.hDevNames );
	{
		printf( "Printer driver: %s\n", (BYTE*)printerDevName + printerDevName->wDriverOffset );
		printf( "Printer device: %s\n", (BYTE*)printerDevName + printerDevName->wDeviceOffset );
		printf( "Printer port: %s\n", (BYTE*)printerDevName + printerDevName->wOutputOffset );
		strcpy( printerName, (char*)printerDevName + printerDevName->wDeviceOffset );
	}
	GlobalUnlock( pd.hDevNames );

	// プリンタを開く
	memset( &printerDefaults, 0, sizeof(printerDefaults) );
	printerDefaults.DesiredAccess = PRINTER_ACCESS_USE; // 印刷以上の目的では使用しない
	rc = OpenPrinter( printerName, &hPrinter, &printerDefaults );
	if( rc == FALSE )
	{
		fprintf( stderr, "Failed to open a printer '%s'. (LE:%d)\n", printerName, GetLastError() );
		goto cleanup;
	}
	printf( "Opened printer '%s'.\n", printerName );

	// Windowsの印刷スプーラーにデータ送信準備が整ったことを伝える
	memset( &docInfo, 0, sizeof(docInfo) );
	docInfo.pDocName = "Untitled 1"; // 印刷する文書の名前（ファイル名など）。主に表示用。
	if( IsEmpty(fileNameForPrintToFile) == FALSE )
		docInfo.pOutputFile = fileNameForPrintToFile;
	jobID = StartDocPrinter( hPrinter, 1, (BYTE*)&docInfo );
	if( jobID == 0 )
	{
		fprintf( stderr, "Failed to start print job. (LE:%d)\n", GetLastError() );
		goto cleanup;
	}

	// 指定された部数だけ印刷（プリンタに部数指定命令がある場合はそれを送るだけで良いはず）
	for( copyCount=0; copyCount<pd.nCopies; copyCount++ )
	{
		DWORD	writtenCount;
		char	buf[32];

		// １部の分だけ、データを書き込む
		for( pageNum=pd.nFromPage; pageNum<=pd.nToPage; pageNum++ )
		{
			sprintf( buf, "%c", g_alphabets[pageNum-1] );
			rc = WritePrinter( hPrinter, buf, strlen(buf), &writtenCount );
			if( rc == FALSE || writtenCount < strlen(buf) )
			{
				fprintf( stderr, "Failed on writing data to printer. (copy:%d, page:%d, LE:%d)\n", copyCount, pageNum, GetLastError() );
				goto cleanup;
			}
		}

		// 部の区切りとしてこのプログラムでは改行コードを送ることにする
		strcpy( buf, "\r\n" );
		rc = WritePrinter( hPrinter, buf, strlen(buf), &writtenCount );
		if( rc == FALSE || writtenCount < strlen(buf) )
		{
			fprintf( stderr, "Failed on writing data to printer. (copy:%d, page:%d, LE:%d)\n", copyCount, pageNum, GetLastError() );
			goto cleanup;
		}
	}
	printf( "Successfully printed '%c' to '%c', %d times.\n", 'A'+pd.nFromPage-1, 'A'+pd.nToPage-1, copyCount );

	rc = 0;

cleanup:
	if( pd.hDevMode != NULL )
		GlobalFree( pd.hDevMode );
	if( pd.hDevNames != NULL )
		GlobalFree( pd.hDevNames );
	if( hPrinter != NULL )
		ClosePrinter( hPrinter );

	return rc;
}
