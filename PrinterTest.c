/* ���̃v���O������Windows�̃v�����^�h���C�o�[�o�R�ň�����߂𒼐ڃv�����^�ɑ��荞��
 * �v���O�����̃T���v���ł��B�����܂ŃT���v���Ƃ������ƂŎ��̂悤�ɒP�������Ă��܂��F
 * 
 * �E1�y�[�W�ڂ̈���f�[�^��'A'����
 * �E    �F
 * �E26�y�[�W�ڂ̈���f�[�^��'Z'����
 * �E���̋�؂��"\r\n"
 * 
 * ����_�C�A���O�Łu�t�@�C���ɏo�͂���v�̃`�F�b�N��t���ē���m�F���邱�Ƃ��I�X�X�����܂��B
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

	UNREFERENCED_PARAMETER( argc ); // �x���E���i�g���Ȃ������j
	UNREFERENCED_PARAMETER( argv ); // �x���E���i�g���Ȃ������j

	// ����
	fileNameForPrintToFile[0] = '\0';

	// �v�����^�����[�U�ɑI�����Ă��炤
	memset( &pd, 0, sizeof(pd) );
	pd.lStructSize = sizeof(pd);
	pd.hwndOwner = NULL; // �v�����^�_�C�A���O�̐e�E�B���h�E�B���̃v���O�����̓R���\�[���Ȃ̂Ŗ����B
	pd.Flags = 0//PD_PAGENUMS	// �y�[�W�ԍ��͈͎̔w������[�U���w��\�ɂ���ꍇ�͎w��
			   //| PD_HIDEPRINTTOFILE	// �u�t�@�C���ɏo�͂���v���B���ꍇ�͂�����w��B�f�o�b�O���͕\�����������ǂ��Ǝv���܂��B
			   ;
	pd.nMinPage = 1;	// �w��\�ȃy�[�W�ԍ��̍ŏ��l
	pd.nMaxPage = 26;	// �w��\�ȃy�[�W�ԍ��̍ő�l
	pd.nFromPage = 1;	// �ŏ��̈���y�[�W�ԍ��Ƃ��ă_�C�A���O�ɏ����\������l
	pd.nToPage = 26;	// �Ō�Ɉ���y�[�W�ԍ��Ƃ��ă_�C�A���O�ɏ����\������l
	rc = PrintDlg( &pd );
	if( rc == FALSE )
	{
		fprintf( stderr, "Failed to get printer information from the dialog. (LE:%d)\n", GetLastError() );
		goto cleanup;
	}
	printf( "pd.Flags: %08x\n", pd.Flags );
	printf( "From: page %d.\n", pd.nFromPage );	// ���[�U���w�肵������J�n�y�[�W�ԍ�
	printf( "To: page %d.\n", pd.nToPage );	// ���[�U���w�肵������I���y�[�W�ԍ�
	printf( "%d copies.\n", pd.nCopies ); // ���[�U���w�肵���������
	printf( "Print all pages: %d\n", (pd.Flags & PD_ALLPAGES) != 0 ); // ���ׂĈ��
	printf( "Print specific pages: %d\n", (pd.Flags & PD_PAGENUMS) != 0 ); // �w�肵���y�[�W�������
	printf( "Print selection: %d\n", (pd.Flags & PD_SELECTION) != 0 ); // �I��͈͂̈��

	// �u�t�@�C���ɏo�͂���v���`�F�b�N����Ă����ꍇ�A�o�͐�̃t�@�C���������[�U�ɐq�˂�
	if( (pd.Flags & PD_PRINTTOFILE) != 0 )
	{
		OPENFILENAME	ofn;

		memset( &ofn, 0, sizeof(ofn) );
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = NULL; // �e�E�B���h�E�B���̃v���O�����̓R���\�[���Ȃ̂Ŗ����B
		ofn.lpstrFile = fileNameForPrintToFile;
		ofn.nMaxFile = sizeof(fileNameForPrintToFile) - 1;
		rc = GetSaveFileName( &ofn );
		if( rc == FALSE )
		{
			fprintf( stderr, "Failed to get name of the file to write print data. (LE:%d, ExtErr:0x%04x)\n", GetLastError(), CommDlgExtendedError() );
			goto cleanup;
		}
	}

	// �v�����^���͈ړ��\�ȃ������̈�i�n���h���j�Ɋ��蓖�Ă���̂ŌŒ肵�Ď擾
	printerDevName = (DEVNAMES*)GlobalLock( pd.hDevNames );
	{
		printf( "Printer driver: %s\n", (BYTE*)printerDevName + printerDevName->wDriverOffset );
		printf( "Printer device: %s\n", (BYTE*)printerDevName + printerDevName->wDeviceOffset );
		printf( "Printer port: %s\n", (BYTE*)printerDevName + printerDevName->wOutputOffset );
		strcpy( printerName, (char*)printerDevName + printerDevName->wDeviceOffset );
	}
	GlobalUnlock( pd.hDevNames );

	// �v�����^���J��
	memset( &printerDefaults, 0, sizeof(printerDefaults) );
	printerDefaults.DesiredAccess = PRINTER_ACCESS_USE; // ����ȏ�̖ړI�ł͎g�p���Ȃ�
	rc = OpenPrinter( printerName, &hPrinter, &printerDefaults );
	if( rc == FALSE )
	{
		fprintf( stderr, "Failed to open a printer '%s'. (LE:%d)\n", printerName, GetLastError() );
		goto cleanup;
	}
	printf( "Opened printer '%s'.\n", printerName );

	// Windows�̈���X�v�[���[�Ƀf�[�^���M���������������Ƃ�`����
	memset( &docInfo, 0, sizeof(docInfo) );
	docInfo.pDocName = "Untitled 1"; // ������镶���̖��O�i�t�@�C�����Ȃǁj�B��ɕ\���p�B
	if( IsEmpty(fileNameForPrintToFile) == FALSE )
		docInfo.pOutputFile = fileNameForPrintToFile;
	jobID = StartDocPrinter( hPrinter, 1, (BYTE*)&docInfo );
	if( jobID == 0 )
	{
		fprintf( stderr, "Failed to start print job. (LE:%d)\n", GetLastError() );
		goto cleanup;
	}

	// �w�肳�ꂽ������������i�v�����^�ɕ����w�薽�߂�����ꍇ�͂���𑗂邾���ŗǂ��͂��j
	for( copyCount=0; copyCount<pd.nCopies; copyCount++ )
	{
		DWORD	writtenCount;
		char	buf[32];

		// �P���̕������A�f�[�^����������
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

		// ���̋�؂�Ƃ��Ă��̃v���O�����ł͉��s�R�[�h�𑗂邱�Ƃɂ���
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
