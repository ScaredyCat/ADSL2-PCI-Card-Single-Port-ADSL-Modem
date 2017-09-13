// Pack.cpp: implementation of the CPack class.
//
//////////////////////////////////////////////////////////////////////
/*  Pack Header

	  +-------------+----------+-------+--------+
	  | "PCAEXPERT" | Identify | Start | Length | 
	  |  9 Byte     |  1 Byte  | 4 Byte|4 Byte  |
	  +-------------+----------+-------+--------+
	  |  Content                                |
	  |                                         |
	  +-----------------------------------------+

 Identify: 0 - record file name
           1 - file




*/

#include "stdafx.h"
#include "Pack.h"
#include <afxtempl.h>
#include "direct.h"
#include "dbaccess.h"
#include "personal.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Define class information
//////////////////////////////////////////////////////////////////////
#define lenStartField	4
#define lenLengthField	4


//////////////////////////////////////////////////////////////////////
// Global variant
//////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPack::CPack()
{
	//get AP location, default in .\debug\xx.exe
	//so, we have to process it after getting location of this AP
	GetModuleFileName( GetModuleHandle(NULL), projectpath.GetBuffer(1024), 1024);
	projectpath.ReleaseBuffer();

	//process AP location
	int length_projectpath=projectpath.GetLength();
	int target_projectpath=projectpath.ReverseFind('\\');
	projectpath.Delete(target_projectpath,length_projectpath-target_projectpath);

}

CPack::~CPack()
{

}

void CPack::CreatePack(CString SourthPath, CString DestPath)
{

	if (mTitle.GetLength()==0)
	{
		AfxMessageBox("CPack:\nPlease setup Title information.");
		exit(1);
	}

	mDestFile=DestPath;
	char Buffer[256];
	char Buffer_TITLE[256];
	strcpy(Buffer_TITLE,mTitle);
	
	CFile InFile(SourthPath,CFile::modeRead|CFile::typeBinary);
	CFile OutFile(DestPath,CFile::modeCreate|CFile::modeWrite|CFile::typeBinary);
	UINT nBytesRead ;
	unsigned long lActualIn,lActualOut;
	unsigned long lAddressStart,lAddressLength,lPossitionLength,lPContentStart,lPContentEnd;
	lActualIn=InFile.Seek(0,CFile::begin);
	lActualOut=OutFile.Seek(0,CFile::begin);
	
	//attach Header Mark : PCAEXPERT
	OutFile.Write(Buffer_TITLE,9);


	// attach Header


	// attach Identify field(1 byte). 0:filename 1:data


	// attach name
	strcpy(Buffer,"0");
	OutFile.Write(Buffer,1);
	lActualOut=OutFile.Seek(0,CFile::current);

	// attach Start field (4 byte)
	lAddressStart=lActualOut;
	lPContentStart=lAddressStart;
	OutFile.Write(&lAddressStart,sizeof(lAddressStart));
	lActualOut=OutFile.Seek(0,CFile::current);

	// keep space for Length field(4byte)
	lPossitionLength=lActualOut; //position of length field
	lActualOut=OutFile.Seek(4,CFile::current);
	lPContentStart=lActualOut;
	// attach name: PCAdb.mdb
	strcpy(Buffer,InFile.GetFileName());
	OutFile.Write(Buffer,InFile.GetFileName().GetLength());
	lPContentEnd=OutFile.Seek(0,CFile::current);

	// write Length field
	lAddressLength=lPContentEnd-lPContentStart;
	OutFile.Seek(lPossitionLength,CFile::begin);
	OutFile.Write(&lAddressLength,sizeof(lAddressLength));

	OutFile.Seek(lPContentEnd,CFile::begin);
	



	// attach mdb
	strcpy(Buffer,"1");
	OutFile.Write(Buffer,1);
	lActualOut=OutFile.Seek(0,CFile::current);

	// attach Start field (4 byte)
	lAddressStart=lActualOut;
	//lPContentStart=lAddressStart;
	OutFile.Write(&lAddressStart,sizeof(lAddressStart));
	lActualOut=OutFile.Seek(0,CFile::current);

	// keep space for Length field(4byte)
	lPossitionLength=lActualOut; //position of length field
	OutFile.Seek(4,CFile::current);
	
	// attach PCAdb.mdb
	lActualOut=OutFile.Seek(0,CFile::current);
	lPContentStart=lActualOut;
	nBytesRead=InFile.Read( Buffer, 256);
	OutFile.Write(Buffer,nBytesRead);


	while(nBytesRead==256)
	{
		nBytesRead=InFile.Read( Buffer, 256);
		OutFile.Write(Buffer,nBytesRead);

	}
	

	lPContentEnd=OutFile.Seek(0,CFile::current);

	// write Length field
	lAddressLength=lPContentEnd-lPContentStart;
	OutFile.Seek(lPossitionLength,CFile::begin);
	OutFile.Write(&lAddressLength,sizeof(lAddressLength));

	OutFile.Seek(lPContentEnd,CFile::begin);

	//close file
	InFile.Close();
	OutFile.Close();



}

void CPack::SetTitle(CString title)
{
	mTitle=title;
}

int CPack::UnPack(CString SourceFilePath)
{
	char newDirectory[128];
	strcpy(newDirectory,projectpath);
	strcat(newDirectory,"\\tmppack");
	//_rmdir(newDirectory);
	UnpackPath=newDirectory;
	if(_mkdir(newDirectory)<0)
	{
		// force kill tmppack
		CleanTemp();
		_rmdir(projectpath+"\\tmppack");
		if(_mkdir(newDirectory)<0)
		{
			AfxMessageBox("mkdir :\nCann't create tmppack directory");
			return 0;
		}
	}

	if (mTitle.GetLength()==0)
	{
		AfxMessageBox("CPack:\nPlease setup Title information.");
		exit(1);
	}





	char Buffer[256];
	char Buffer_TITLE[10];
	char DestName[256];
	unsigned long lCopyLength; // count remain copy length
	int ByteRead;
	CFileException e;
	CString PathName;

	CFile FileInput(SourceFilePath,CFile::modeRead|CFile::typeBinary);
	
	CFile FileOutput;

	//varify extension file
	FileInput.Read(Buffer_TITLE,9);
	Buffer_TITLE[9]='\0';

	if(strcmp(Buffer_TITLE,mTitle)!=0)
	{
		AfxMessageBox("Invalid Extension File!");
		return 0;
	}

	

	while(CheckRemainLength(&FileInput))
	{
		unsigned long bb=FileInput.Seek(0,CFile::current);


		// read Identify field
		FileInput.Read(Buffer,1);
		bb=FileInput.Seek(0,CFile::current);
		// read Start & Length field
		unsigned long lPossitionStart,lContentLength;
		FileInput.Read(&lPossitionStart,4);
		FileInput.Read(&lContentLength,4);


		// get filename
		FileInput.Read(Buffer,lContentLength);
		strcpy(DestName,Buffer);
		DestName[lContentLength]='\0';
		bb=FileInput.Seek(0,CFile::current);
		
		//------------------------------------
		PathName=newDirectory;
		PathName=PathName+"\\";
		PathName.Insert(PathName.GetLength(),DestName);
		FileOutput.Open(PathName,CFile::modeCreate|CFile::modeWrite|CFile::typeBinary,&e);

		// store destination filename
		ArrayFileList.Add(PathName);
		// read Identify field
		FileInput.Read(Buffer,1);
		bb=FileInput.Seek(0,CFile::current);
		// read Start & Length field
		FileInput.Read(&lPossitionStart,4);
		FileInput.Read(&lContentLength,4);
		bb=FileInput.Seek(0,CFile::current);
		// output data 
		lCopyLength=lContentLength;


		while(lCopyLength>256)
		{
			ByteRead=FileInput.Read(Buffer,256);
			FileOutput.Write(Buffer,ByteRead);
			lCopyLength=lCopyLength-256;
		}
	
		ByteRead=FileInput.Read(Buffer,lCopyLength);
		FileOutput.Write(Buffer,ByteRead);
	
		FileOutput.Close();
	
	} // end loop of unpack files

	//if(!CleanTemp())
	//{
	//	AfxMessageBox("CPack::CleanTemp Error\n Cann't remove all temp file");
	//	return;
	//}
		return 1;
}

void CPack::AddPack(CString SourthPath)
{
	if (mDestFile.GetLength()==0)
	{
		AfxMessageBox("CPack:\nYou must use CreatePack in advance.");
		exit(1);
	}


	char Buffer[256];
	char Buffer_TITLE[256];
	strcpy(Buffer_TITLE,mTitle);
	
	CFile InFile(SourthPath,CFile::modeRead|CFile::typeBinary);
	CFile OutFile(mDestFile,CFile::modeNoTruncate|CFile::modeWrite|CFile::typeBinary);
	
	UINT nBytesRead ;
	unsigned long lActualIn,lActualOut;
	unsigned long lAddressStart,lAddressLength,lPossitionLength,lPContentStart,lPContentEnd;
	lActualIn=InFile.Seek(0,CFile::begin);
	lActualOut=OutFile.SeekToEnd();
	lActualOut=OutFile.Seek(0,CFile::current);	

	// attach Header


	// attach Identify field(1 byte). 0:filename 1:data


	// attach name
	strcpy(Buffer,"0");
	OutFile.Write(Buffer,1);
	lActualOut=OutFile.Seek(0,CFile::current);

	// attach Start field (4 byte)
	lAddressStart=lActualOut;
	lPContentStart=lAddressStart+lenStartField+lenLengthField;
	OutFile.Write(&lPContentStart,sizeof(lPContentStart));
	lActualOut=OutFile.Seek(0,CFile::current);

	// keep space for Length field(4byte)
	lPossitionLength=lActualOut; //position of length field
	lActualOut=OutFile.Seek(4,CFile::current);
	//lPContentStart=lActualOut;

	// attach name: PCAdb.mdb
	strcpy(Buffer,InFile.GetFileName());
	OutFile.Write(Buffer,InFile.GetFileName().GetLength());
	lPContentEnd=OutFile.Seek(0,CFile::current);

	// write Length field
	lAddressLength=lPContentEnd-lPContentStart;
	OutFile.Seek(lPossitionLength,CFile::begin);
	OutFile.Write(&lAddressLength,sizeof(lAddressLength));

	OutFile.Seek(lPContentEnd,CFile::begin);
	



	// attach data
	strcpy(Buffer,"1");
	OutFile.Write(Buffer,1);
	lActualOut=OutFile.Seek(0,CFile::current);

	// attach Start field (4 byte)
	lAddressStart=lActualOut;
	lPContentStart=lAddressStart+lenStartField+lenLengthField;
	OutFile.Write(&lPContentStart,sizeof(lPContentStart));
	lActualOut=OutFile.Seek(0,CFile::current);

	// attach Length field(4byte)
	lPossitionLength=lActualOut; //position of length field
	//OutFile.Seek(lenLengthField,CFile::current);	
	lAddressLength=InFile.Seek(0,CFile::end);
	OutFile.Write(&lAddressLength,sizeof(lAddressLength));
	InFile.SeekToBegin();

	
	// attach PCAdb.mdb
	lActualOut=OutFile.Seek(0,CFile::current);
	lPContentStart=lActualOut;

	nBytesRead=InFile.Read( Buffer, 256);
	while(1)
	{	
		if(nBytesRead==0)
			break;

		OutFile.Write(Buffer,nBytesRead);
		nBytesRead=InFile.Read( Buffer, 256);
		lActualOut=OutFile.Seek(0,CFile::current);
		
	}
	

	lPContentEnd=OutFile.Seek(0,CFile::current);
	

	InFile.Close();
	OutFile.Close();

}

bool CPack::CheckRemainLength(CFile *cfile)
{
	bool result=true;
	unsigned long oriLength;
	char Buffer[256];
	oriLength=cfile->Seek(0,CFile::current);
	if (cfile->Read(Buffer,256)<9)
	{
		//there are no data
		result=false;
	}
	else
		result=true;

	// move pointer back
	cfile->Seek(oriLength,CFile::begin);
	return result;
}

bool CPack::CleanTemp()
{
	// delete all files in \\tmppack

	CArray<CString,CString> ArrayDelete;
	CString FilePath;
	char path[256];	

	bool result=true;	

	CFileFind finder;
	BOOL bWorking;
	bWorking=finder.FindFile(projectpath+"\\tmppack\\*.*");
	while(bWorking)
	{
		bWorking=finder.FindNextFile();
		if(finder.IsDots())
			continue;	
		//FileName=finder.GetFilePath();
		ArrayDelete.Add(finder.GetFilePath());
		//pack.AddPack(finder.GetFilePath());
	}


	for(int i=0;i<ArrayDelete.GetSize();i++)
	{
		strcpy(path,"");	
		strcat(path,ArrayDelete[i]);

		TRY{
			CFile::Remove(path);  }
		CATCH(CFileException, e)
		{
			result=false;
		}
		END_CATCH
	}




//	CString FilePath;
	FilePath=projectpath+"\\tmppack";	
	_rmdir(FilePath);
	
	return result;
}

int CPack::CSVExport(CString sDest)
{

	int result=1;

	char cIdentify[256]="PCAEXPERT CSV\r\n";
	char cTmp[256];

	//create CFile::cfile
	CFile cFile(sDest,CFile::modeCreate|CFile::modeWrite);
	cFile.Seek(0,CFile::begin);


//skip this Identify ...... 
	//Write "PCAEXPERT"
//	cFile.Write(cIdentify,strlen(cIdentify));
	

	//DB setup
	CDBAccess db;
	CString strInitial,errStr;
	strInitial="Provider=Microsoft.Jet.OLEDB.4.0;Data Source="+projectpath+"\\PCAdb.mdb";
	
	
	db.Initialize(strInitial,errStr);
	db.OpenRecordSet("SELECT * FROM personal ",errStr);
	if( !errStr.IsEmpty() )
	{
		AfxMessageBox(errStr);   
		result=0;
	}

	int iDBCount;
	iDBCount=db.GetRecordCount();
	
	CString sGet;

	while(!db.IsEOF())
	{
//skip class
		// get class
//		sGet.Format("%d,",db.GetIntFromField(1));
//		strcpy(cTmp,sGet);
		// write class
//		cFile.Write(cTmp,strlen(cTmp));

		// get name
		db.GetStringFromField(2,(LPTSTR)sGet.GetBuffer(128),128);
		sGet.ReleaseBuffer();
		sGet=TranFullComma(sGet);
		strcpy(cTmp,sGet);
		strcat(cTmp,",");
		// write name
		cFile.Write(cTmp,strlen(cTmp));

		// get id
		db.GetStringFromField(3,(LPTSTR)sGet.GetBuffer(128),128);
		sGet.ReleaseBuffer();
		sGet=TranFullComma(sGet);
		strcpy(cTmp,sGet);
		strcat(cTmp,",");
		// write id
		cFile.Write(cTmp,strlen(cTmp));

		//get division
		db.GetStringFromField(4,(LPTSTR)sGet.GetBuffer(128),128);
		sGet.ReleaseBuffer();
		sGet=TranFullComma(sGet);
		strcpy(cTmp,sGet);
		strcat(cTmp,",");
		//write division
		cFile.Write(cTmp,strlen(cTmp));

		//get department
		db.GetStringFromField(5,(LPTSTR)sGet.GetBuffer(128),128);
		sGet.ReleaseBuffer();
		sGet=TranFullComma(sGet);
		strcpy(cTmp,sGet);
		strcat(cTmp,",");
		//write department
		cFile.Write(cTmp,strlen(cTmp));

		//get telephone
		db.GetStringFromField(6,(LPTSTR)sGet.GetBuffer(128),128);
		sGet.ReleaseBuffer();
		sGet=TranFullComma(sGet);
		strcpy(cTmp,sGet);
		strcat(cTmp,",");
		//write telephone
		cFile.Write(cTmp,strlen(cTmp));

		//get remark
		db.GetStringFromField(7,(LPTSTR)sGet.GetBuffer(128),128);
		sGet.ReleaseBuffer();
		sGet=TranFullComma(sGet);
		strcpy(cTmp,sGet);
		strcat(cTmp,",");
		//write remark
		cFile.Write(cTmp,strlen(cTmp));

		//write \r\n for new line
		strcpy(cTmp,"\r\n");
		cFile.Write(cTmp,strlen(cTmp));

		// move db record to next
		db.MoveNext();
	}


	cFile.Close();
	return result;
}

CString CPack::TranFullComma(CString sData)
{
	// Replace ',' into '¡A'	
	sData.Replace(",","¡A");
	return sData;
}

CString CPack::TranHalfComma(CString sData)
{
	// Replace '¡A' into ',' 
	sData.Replace("¡A",",");	
	return sData;
}

int CPack::CSVImport(CString sSource)
{

	// In order to keep off the length of user's cvs filed overflow,
	// we will limit the length of field

	int iLengthName=50;
	int iLengthId=50;
	int iLengthDivision=50;
	int iLengthDepartment=50;
	int iLengthTelephone=50;
	int iLengthRemark=255;


	// CSVImport will store CSV data into CSVArray

	int result=1;
	int head,trail;
	InfoPersonal personal;
	//create File
	FILE *file;
	file=fopen(sSource,"r");

	char cTmp[256];
	char cAdd[256];
	CString sProcess;
	int byteRead;



	while(fgets(cTmp,255,file)!=NULL)
	{
		

		sProcess.Format("%s",cTmp);
		head=trail=0;



		//get field: name		
		head=sProcess.Find(",",head);
		if (head==-1)
		{
			personal.m_name=sProcess.Mid(trail,sProcess.GetLength()-trail);	
			personal.m_name=personal.m_name.Mid(0,iLengthName);
			personal.m_telephone="0";
			//add data into CSVArray
			CSVArray.Add(personal);
			continue;

		}
		else
			personal.m_name=sProcess.Mid(trail,head-trail);

		personal.m_name=personal.m_name.Mid(0,iLengthName);
		head++;
		trail=head;

		//get field: id
		head=sProcess.Find(",",head);
		if (head==-1)
		{
			personal.m_id=sProcess.Mid(trail,sProcess.GetLength()-trail);	
			personal.m_id=personal.m_id.Mid(0,iLengthName);
			personal.m_telephone="0";
			//add data into CSVArray
			CSVArray.Add(personal);
			continue;

		}
		else
			personal.m_id=sProcess.Mid(trail,head-trail);
		personal.m_id=personal.m_id.Mid(0,iLengthId);
		head++;
		trail=head;
		
		//get field: division
		head=sProcess.Find(",",head);
		if (head==-1)
		{
			personal.m_division=sProcess.Mid(trail,sProcess.GetLength()-trail);	
			personal.m_division=personal.m_division.Mid(0,iLengthName);
			personal.m_telephone="0";
			//add data into CSVArray
			CSVArray.Add(personal);
			continue;

		}
		else
			personal.m_division=sProcess.Mid(trail,head-trail);
		personal.m_division=personal.m_division.Mid(0,iLengthDivision);
		head++;
		trail=head;

		//get field: department
		head=sProcess.Find(",",head);
		if (head==-1)
		{
			personal.m_department=sProcess.Mid(trail,sProcess.GetLength()-trail);	
			personal.m_department=personal.m_department.Mid(0,iLengthName);
			personal.m_telephone="0";
			//add data into CSVArray
			CSVArray.Add(personal);
			continue;

		}
		else
			personal.m_department=sProcess.Mid(trail,head-trail);
		personal.m_department=personal.m_department.Mid(0,iLengthDepartment);
		head++;
		trail=head;

		//get field: telephone
		head=sProcess.Find(",",head);
		if (head==-1)
		{
			personal.m_telephone=sProcess.Mid(trail,sProcess.GetLength()-trail);	
			personal.m_telephone=personal.m_telephone.Mid(0,iLengthName);			
			//add data into CSVArray
			CSVArray.Add(personal);
			continue;

		}
		else
			personal.m_telephone=sProcess.Mid(trail,head-trail);
		personal.m_telephone=personal.m_telephone.Mid(0,iLengthTelephone);
		head++;
		trail=head;

		//get field: remark
		head=sProcess.Find(",",head);

			// because this field is the last
			// the value of head will be '-1'
		    // if this value is not '-1'
		    // we will stop process after this field

		if (head==-1)
		{
			personal.m_remark=sProcess.Mid(trail,sProcess.GetLength()-trail);			

		}
		else
		{
			personal.m_remark=sProcess.Mid(trail,head-trail);
		}
		personal.m_remark=personal.m_remark.Mid(0,iLengthRemark);

		//add data into CSVArray
		CSVArray.Add(personal);
	}

	fclose(file);
	return result;
}
