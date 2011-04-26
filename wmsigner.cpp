#include "stdio.h"
#include "signer.h"
#include <errno.h>
#include <stdlib.h>

#ifndef _WIN32
#include <unistd.h>
#endif

/* .ini file format *************************
   You must use <exe-file name>.ini for
   your ini-file

354413238595
password
/usr/wmsigner/keyfile.kwm
*********************************************/



char* stripCRLF(char* szStrWithCRLF)
{
  if(szStrWithCRLF) {
    if (strlen(szStrWithCRLF)) {
      char *np = szStrWithCRLF;
      if((np = strchr(np, '\n')) !=  NULL) *np = '\0';
      np = szStrWithCRLF;
      if((np = strchr(np, '\r')) !=  NULL) *np = '\0';
    }
  }
  return szStrWithCRLF;
}



bool LoadIniFile(const char *szFName, szptr& szLogin, szptr& szPwd, szptr& szFileName, short &ErrorCode)
{
  char szBufStr[512];
  bool bRC = false;

  FILE *file = fopen(szFName,"r");

  if (file != NULL)
  {
    if (fgets(szBufStr, 512, file))
    {
      szLogin = stripCRLF(szBufStr);
      if (fgets(szBufStr, 512, file))
      {
        szPwd = stripCRLF(szBufStr);
        if (fgets(szBufStr, 512, file))
        {
          szFileName = stripCRLF(szBufStr);
          bRC = true;
        }
        else
          ErrorCode = -6;   // Keys FileName missing in INI file
      }
      else
        ErrorCode = -5;     // Password missing in INI file
    }
    else
      ErrorCode = -4;       // Login missing in INI file

    fclose(file);
  }
  else
    ErrorCode = 10*errno;

  return bRC;
}



int main(int argc, char* argv[])
{
  char szBufforInv[1024];
  char szError[80];
  szptr szLogin, szPwd, szFileName, szIn, szSign;
  short siErrCode = 0;



  // loading ini-file

  szptr szIniFileFull = "";

  #ifdef _WIN32
  {
    char  drive[_MAX_DRIVE];
    char  dir[_MAX_DIR];
    char  fname[_MAX_FNAME];
    char  ext[_MAX_EXT];

    _splitpath((const char *)argv[0], drive, dir, fname, ext );
    szIniFileFull += drive;
    szIniFileFull += dir;
    szIniFileFull += fname;
    szIniFileFull += ".ini";
  }
  #else
  {
    // unix sustem
    szIniFileFull = argv[0];
    szIniFileFull += ".ini";
  }
  #endif

  if (!LoadIniFile(szIniFileFull, szLogin, szPwd, szFileName, siErrCode))
  {
    sprintf(szError, "Error %d.", siErrCode);
    fputs(szError, stdout);
    return 2;
  }



  // extracting char string

  char eof = 0;
  while (!eof)
  {
    if (NULL == fgets(szBufforInv, 1024, stdin))
      eof = 1;
    else
    {
      // 004 key may be founded before \r\n sequence
      int sz = strlen(szBufforInv)-1;
      int depth = (sz<5 ? sz : 5);
      for(int i=0;i<depth;i++)
      {
        if (szBufforInv[sz-i] == '\x04')
        { szBufforInv[sz-i] = '\0';
          eof = 1;
          break;
        }
      }

      // cut newline chars
      for(int l=strlen(szBufforInv)-1;l>=0;l--)
      {
      char c = szBufforInv[l];

      if (c == 0x0D)
          szBufforInv[l] = '\0';
        else
          break;
      }

      szIn += szBufforInv;

    }
  }


  // sigining

  Signer sign(szLogin, szPwd, szFileName);
  if (sign.Sign(szIn, szSign))
  {
    fputs(szSign, stdout);
  }
  else
  {
    sprintf(szError, "Error %d", sign.ErrorCode());
    fputs(szError, stdout);
    return sign.ErrorCode();
  }

  return 0;
}
