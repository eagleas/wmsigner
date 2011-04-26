#include "stdafx.h"
#include "stdio.h"
#include "signer.h"
#include <errno.h>
#include <stdlib.h>
#include "cmdbase.h"

#define MAXBUF	4096
#define MAXSTR	1024

#ifndef ENCODE
#define ENCODE	0
#endif

#ifndef DECODE
#define DECODE	1
#endif

#ifndef CODE_ERR
#define CODE_ERR	0
#endif


const char* WMSignerVersion = "2.0b";
bool isIgnoreKeyFile = false;
char szKeyData[MAXBUF+1] = "";       /* Buffer for Signre-s key      */
int Key64Flag = FALSE;

int CommandLineParse( const int argc, const char *argv[], char *szLoginCL, char *szPwdCL, 
                      char *szFileNameCL, char *szKeyFileNameCL, char *szKeyData, 
                      char *szStringToSign, int *Key64Flag );

void NormStr( char *str );
size_t Code64( int job, char *buf_ascii, size_t ascii_size, char *buf_64, size_t buf_64_size );
int idx64( char ch );
int fatal_err( char *err_msg );

/*
 * For Code64 module 
 * 
 */

/* Bits BASE64 structure */
typedef struct __Bits64__ {
  unsigned b3 : 6; /* 1 Base 64 character  */
  unsigned b2 : 6; /* 2 Base 64 character  */
  unsigned b1 : 6; /* 3 Base 64 character  */
  unsigned b0 : 6; /* 4 Base 64 character  */
} BITS, *BITSPTR;

/* Union of Bits & Bytes */
typedef union __Base64__ {
  char a[3]; /* Byte array in the case  */
  BITS b;    /* Bits fields in the case */
} BASE64;

/* Base_64 characters  */
const char Ch64[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Base_64 index structure */

typedef struct __index64__ {
  char Ch;
  int Id;
} INDEX64, *INDEX64PTR;

/*  Presort array for indexes  */

const INDEX64 Id64[64] = {
  {'+',62},{'/',63},{'0',52},{'1',53},{'2',54},{'3',55},{'4',56},{'5',57},{'6',58},{'7',59},{'8',60},{'9',61},
  {'A',0},{'B',1},{'C',2},{'D',3},{'E',4},{'F',5},{'G',6},{'H',7},{'I',8},{'J',9},{'K',10},{'L',11},{'M',12},
  {'N',13},{'O',14},{'P',15},{'Q',16},{'R',17},{'S',18},{'T',19},{'U',20},{'V',21},{'W',22},{'X',23},{'Y',24},{'Z',25},
  {'a',26},{'b',27},{'c',28},{'d',29},{'e',30},{'f',31},{'g',32},{'h',33},{'i',34},{'j',35},{'k',36},{'l',37},{'m',38},
  {'n',39},{'o',40},{'p',41},{'q',42},{'r',43},{'s',44},{'t',45},{'u',46},{'v',47},{'w',48},{'x',49},{'y',50},{'z',51}
};

/***********************************/

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
  char szBufStr[MAXSTR]="";
  bool bRC = false;

 szLogin = "";
 szPwd = "";
 szFileName = "";

 FILE *file = fopen(szFName,"r");
  if (file != NULL)
  {
    if (fgets(szBufStr, MAXSTR, file))
    {
      szLogin = stripCRLF(szBufStr);
      if( strlen( szLogin ) < 2 ) { ErrorCode = -4; return bRC; }
      if (fgets(szBufStr, MAXSTR, file))
      {
        szPwd = stripCRLF(szBufStr);
        if( strlen( szPwd ) < 2 ) { ErrorCode = -5; return bRC; }
        if (fgets(szBufStr, MAXSTR, file))
        {
          szFileName = stripCRLF(szBufStr);
          if( strlen( szFileName ) < 2 ) { ErrorCode = -6; return bRC; }
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

//--------------------------------------------
/*  Parse Command string  */
int CommandLineParse( const int argc, const char *argv[], char *szLoginCL, char *szPwdCL, 
                      char *szFileNameCL, char *szKeyFileNameCL, char *szKeyData, 
                      char *szStringToSign, int *Key64Flag )
{
   int i = 1;
   int j = 0;
   int numparam = 0;
   char KeyBuffer[512];
   size_t Bytes = 0;

   while( i < argc ) {

     j = i + 1;

     if( (strcmp( argv[i], "-h") == 0) || (strcmp( argv[i], "--help") == 0) ){
       printf("wmsigner, Version %s (c) WebMoney Transfer (r), 2007\n\r\n\r", WMSignerVersion );
       printf(" -p   [--password]   : Password for key_file\n\r");
       printf(" -w   [--wmid]       : 123456789012 : WMID (12 Digits)\n\r");
       printf(" -s   [--sign]       : string_to_signification : signing specified string\n\r");
       printf(" -i   [--ini-path]   : Correct path to ini_file with ini_file_name *.ini\n\r");
       printf(" -k   [--key-path]   : Correct path to key_file with key_file_name\n\r");
       printf(" -K64 [--key-base64] : Text string in Base64 code, contain the key for wmsigner\n\r");
       printf(" -h   [--help]       : Help (this srceen)\n\r");
       printf(" -v   [--version]    : Version of program\n\r\n\r");
       exit(0);
     }

     if( (strcmp( argv[i], "-v") == 0) || (strcmp( argv[i], "--version") == 0) ){
       printf("wmsigner, Version %s (c) WebMoney Transfer (r), 2007\n\r", WMSignerVersion );
       exit(0);
     }

     if( (strcmp( argv[i], "-p") == 0) || (strcmp( argv[i], "--password") == 0) ) {
       if( j >= argc ) fatal_err("Password not defined!\n\r");
       strncpy( szPwdCL, argv[j], MAXSTR);
       numparam++;
     }

     if( (strcmp( argv[i], "-w") == 0) || (strcmp( argv[i], "--wmid") == 0)) {
       if( j >= argc ) fatal_err("WMID Not defined!\n\r");
       strncpy( szLoginCL, argv[j], MAXSTR);
       numparam++;
     }

     if( (strcmp( argv[i], "-s") == 0) || (strcmp( argv[i], "--sign") == 0)) {
     if( j >= argc ) fatal_err("String to signification not defined!\n\r");
       strncpy( szStringToSign, argv[j], MAXSTR);
       numparam++;
     }

     if( (strcmp( argv[i], "-i") == 0) || (strcmp( argv[i], "--ini-path") == 0)){
       if( j >= argc ) fatal_err("Ini file name (with path) not defined!\n\r");
       strncpy( szFileNameCL, argv[j], MAXSTR);
       numparam++;
     }

     if( (strcmp( argv[i], "-k") == 0) || (strcmp( argv[i], "--key-path") == 0)){
       if( j >= argc ) fatal_err("Key file not defined!\n\r");
       strncpy( szKeyFileNameCL, argv[j], MAXSTR);
       numparam++;
     }

     if( (strcmp( argv[i], "-K64") == 0) || (strcmp( argv[i], "--key-base64") == 0) ){
       if( j >= argc ) fatal_err("KEY_STRING in Base64 code not defined!\n\r");
       if( strlen( argv[j] ) != 220 ) fatal_err("Key string has illegal length!");
       strcpy( szKeyData, argv[j] );
       Bytes = Code64( ENCODE, KeyBuffer, 512, szKeyData, 220 );
       if( Bytes != 164 ) fatal_err("Bad key string in parameter!");
       memcpy( szKeyData, KeyBuffer, 164);
       *Key64Flag = TRUE;
       numparam++;
     }

     i+=2;

   }

   if( numparam ) {
     return( TRUE );
   }
   else {
     fatal_err("Illegal command line option found! Use option --help or -h for information.");
   }
   return( FALSE );
}

void NormStr( char *str )
{
  char *s = str;
  while( *s ) {
    if( (*s == 0x0d ) || (*s == 0x0a) || (*s == 0x04) ) { *s = 0; break; }
    s++;
  }
}

//--------------------------------------------

int main(int argc, char* argv[])
{
//--------------------------------------------------------------------
  szptr szLogin, szPwd, szFileName, szIn, szSign;
  char szBufforInv[MAXSTR+1] = "";
  char szError[80] = "";
  short siErrCode = 0;
  char szKeyDataENC[MAXBUF+1] = "";    /* Buffer for Signre-s key      */
  char szStringToSign[MAXBUF+1] = "";  /* String for signification     */
  /*-----------------------------------------------------------------*/
  char szLoginCL[MAXSTR+1] = ""; 
  char szPwdCL[MAXSTR+1] = ""; 
  char szFileNameCL[MAXSTR+1] = "";    /*  INI - Fil e                 */
  char szKeyFileNameCL[MAXSTR+1] = ""; /*  KWM - File                  */
  int CmdLineKey = FALSE;              /* ? Command line Key           */
  bool result = FALSE;
  int ErrorCode = 0;
  static char pszOut[MAXBUF+1] = "";
  size_t num_bytes = 0;
  /*-----------------------------------------------------------------*/

  szptr szIniFileFull = "";

#ifdef _WIN32
    char  drive[_MAX_DRIVE];
    char  dir[_MAX_DIR];
    char  fname[_MAX_FNAME];
    char  ext[_MAX_EXT];

    _splitpath((const char *)argv[0], drive, dir, fname, ext );
    szIniFileFull += drive;
    szIniFileFull += dir;
    szIniFileFull += fname;
    szIniFileFull += ".ini";
#else
    // unix sustem
    szIniFileFull = argv[0];
    szIniFileFull += ".ini";
#endif


//---------------------------------------------
 /*  Command line found ? Parsing data */
  if( argc > 1 ) {
    CmdLineKey = CommandLineParse( argc, (const char **)argv, szLoginCL, szPwdCL,
                                   szFileNameCL, szKeyFileNameCL, szKeyData, szStringToSign, &Key64Flag );
  }
  /*  End of Parse command line */


  /*  Replace Key File Name from command Line, if present  */
  if( strlen(szFileNameCL) ) szIniFileFull = szFileNameCL;

  if( (Key64Flag == TRUE) && (strlen(szLoginCL) > 1) && (strlen(szPwdCL) > 1))
	  isIgnoreKeyFile = true;

  // loading ini-file
  if( isIgnoreKeyFile == false )
   if (!LoadIniFile(szIniFileFull, szLogin, szPwd, szFileName, siErrCode))
   {
    sprintf(szError, "Error %d", siErrCode);
	printf(szError);
    return 2;
   }

  //  Replace Login and Password from command Line, if present
  if( strlen(szKeyFileNameCL) ) szFileName = szKeyFileNameCL;
  if( strlen(szLoginCL) ) szLogin  = szLoginCL;
  if( strlen(szPwdCL) ) szPwd = szPwdCL;

  // extracting char string
  if( strlen( szStringToSign )) {
    strncpy( szBufforInv, szStringToSign, MAXSTR);
  }
  else {
    if ( fgets(szBufforInv, MAXSTR, stdin) == 0 ) {
      exit(0);
    }
  }

  NormStr( szBufforInv );

  szIn = szBufforInv;

//----------------------------------------------------
// sigining (new)

  Signer sign(szLogin, szPwd, szFileName);

  if( Key64Flag == TRUE ) {
    sign.SetKeyFromCL( TRUE, szKeyData );
  }

  result = sign.Sign(szIn, szSign);
  ErrorCode = sign.ErrorCode();


  if ( result ){
	  strncpy( pszOut, szSign, MAXSTR);
      printf("%s", pszOut);
	  exit(0);
  }
  else {
    sprintf(pszOut, "Error %d", ErrorCode );
    printf("%s", pszOut);
    return (ErrorCode);
  }
 return 0;
//----------------------------------------------------
}

/****************************************************************************************************************************
 *  Base64 Encoder / Decoder module. Light Version 1.0
 *  Written by Georgy Mushkarev (c) for wmsigner Library. Moscow, Russia, 1992-2007.
 *  .........................................................................................................................
 *  Usage this module :
 *  size_t result = code64( JOB [ENCODE or DECODE], char *ASCII_Buffer, size_t ASCII_BUFFER_SIZE, 
 *                          char *BASE64_Buffer,  size_t BASE64_BUFFER_SIZE );
 *  Sample for decode from ascii to base64 : 
 *  result = code64( DECODE, char *ASCII_Buffer, size_t ASCII_BUFFER_SIZE, char *BASE64_Buffer,  size_t BASE64_BUFFER_SIZE );
 *  Parameter ASCII_BUFFER_SIZE - definition size input to decode sequence ! 
 *  function strlen() - may be used ONLY for text buffers, otherwize - BINARY DATA, use really size !
 *  Sample for encode from base64 to ascii : 
 *  result = code64( ENCODE, char *ASCII_Buffer, size_t ASCII_BUFFER_SIZE, char *BASE64_Buffer,  size_t BASE64_BUFFER_SIZE );
 *  Parameter BASE64_BUFFER_SIZE - definition size input to encode sequence !
 *  function strlen() may be used for counting size for BASE64 array, if last symbol == '\0', otherwise use really size !
 *  .........................................................................................................................
 *  Return value:
 *  size_t result - counter, number of bytes in OUTPUT array, whose successfully encoded/decoded.
 *  if result == 0 - error found, abnormal terminating, nothing to encoding or decoding.
 *  .........................................................................................................................
 *  Internal function: idx64( char Ch ); Binary search index in Base64 array, for Ch character in parameter
 *  .........................................................................................................................
 *  This module present "AS IS", absolutely no warranty, if anybody modyfied this source.
 ****************************************************************************************************************************/
size_t Code64( int job, char *buf_ascii, size_t ascii_size, char *buf_64, size_t buf_64_size )
{
  BASE64 b64;
  size_t i = 0, j = 0, k = 0, i3 = 0; /*  job indexes */
  size_t ask_len = 0;   /* Internal variable for size ascii buffer   */
  size_t buf64_len = 0; /* Internal variable for size base64 buffer  */

  b64.a[0] = 0; b64.a[1] = 0; b64.a[2] = 0;

/*  Decoding to Base64 code  */
  if( job == DECODE ) {
    ask_len = ascii_size;
    if( (ascii_size * 4 / 3) > buf_64_size ) return( CODE_ERR ); /* No enough space to decoding */
    if( ask_len == 0 ) return( CODE_ERR );
    i3 = (int) 3 * (ask_len / 3);      /* Dividing by 3       */
    if( i3 > 0 ) k = 3 - ask_len + i3; /* End of seq (1 or 2) */
    if( ask_len < 3 ) k = 3 - ask_len;

    while( i < i3 ) {
      b64.a[2] = buf_ascii[i];
      b64.a[1] = buf_ascii[i+1];
      b64.a[0] = buf_ascii[i+2];
      buf_64[j++] = Ch64[b64.b.b0];
      buf_64[j++] = Ch64[b64.b.b1];
      buf_64[j++] = Ch64[b64.b.b2];
      buf_64[j++] = Ch64[b64.b.b3];
      i+=3;
    }

    if( k == 2 ) {
      b64.a[2] = buf_ascii[i];
      b64.a[1] = 0;
      b64.a[0] = 0;
      buf_64[j++] = Ch64[b64.b.b0];
      buf_64[j++] = Ch64[b64.b.b1];
      buf_64[j++] = '=';
      buf_64[j++] = '=';
    }

    if( k == 1 ) {
      b64.a[2] = buf_ascii[i];
      b64.a[1] = buf_ascii[i+1];
      b64.a[0] = 0;
      buf_64[j++] = Ch64[b64.b.b0];
      buf_64[j++] = Ch64[b64.b.b1];
      buf_64[j++] = Ch64[b64.b.b2];
      buf_64[j++] = '=';
    }

    return( j );
  }

/* Restoring original characters */
  if( job == ENCODE ) {
    buf64_len = buf_64_size;
    if( buf64_len < 4 ) return( CODE_ERR ); /* Too small input buffer  */
    if( (buf_64_size * 3 / 4) > ascii_size ) return( CODE_ERR ); /* No enough space to encoding */
    i3 = buf64_len / 4;
    k = buf64_len - i3 * 4;
    if( k ) return( CODE_ERR ); /*  Illegal size for input sequence !! */
    k = 0;                      /*  Counter original bytes for output  */

    for( i = 0; i < buf64_len; i+=4 ) {
      b64.b.b0 = idx64( buf_64[i] );
      b64.b.b1 = idx64( buf_64[i+1] );
      b64.b.b2 = 0; b64.b.b3 = 0;
      if( buf_64[i+2] != '=' ) b64.b.b2 = idx64( buf_64[i+2] );
      else k--;
      if( buf_64[i+3] != '=' ) b64.b.b3 = idx64( buf_64[i+3] );
      else k--;
      buf_ascii[j++] = b64.a[2]; k++;
      buf_ascii[j++] = b64.a[1]; k++;
      buf_ascii[j++] = b64.a[0]; k++;
    }

    return( k );
  }

  return( CODE_ERR ); /*  Unknown command ! */
}

/*  Binary search character's index in array  */

int idx64( char ch )
{
  int start = 0;
  int pos   = 32;
  int end   = 63;

  while( (pos >= start) || (pos <= end) ) {
     if( ch == Id64[start].Ch ) return( Id64[start].Id );
     if( ch == Id64[pos].Ch )   return( Id64[pos].Id );
     if( ch == Id64[end].Ch )   return( Id64[end].Id );

     if( ch > Id64[pos].Ch ) {
        start = pos;
     }

     if( ch < Id64[pos].Ch ) {
        end = pos;
     }

     pos = (int) (start+end) / 2;
  }

/*  Illegal character found, task aborted !  */
  printf( "\n\rBase64 Fatal Error: Illegal character found : '%c' [%d] - No Base64 legal character!!\n\r", ch, ch );
  exit(1);
}

int fatal_err( char *err_msg )
{
  printf( "%s", err_msg);
  exit(1);
}

/*************  END BASE64 Modules ***************/
//----
