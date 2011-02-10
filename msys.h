#ifdef GCC

#define _In_
#define _Out_
#define _Inout_
#define _CRTIMP 

#ifndef _TIME64_T_DEFINED
typedef __int64 __time64_t;     /* 64-bit time value */
#define _TIME64_T_DEFINED
#endif

// time.h
/*
_CRTIMP double __cdecl _difftime64(_In_ __time64_t _Time1, _In_ __time64_t _Time2);
_CRT_INSECURE_DEPRECATE(_ctime64_s) _CRTIMP char * __cdecl _ctime64(_In_ const __time64_t * _Time);
_CRTIMP errno_t __cdecl _ctime64_s(_Out_z_cap_(_SizeInBytes) char *_Buf, _In_ size_t _SizeInBytes, _In_ const __time64_t * _Time);
__DEFINE_CPP_OVERLOAD_SECURE_FUNC_0_1(errno_t, _ctime64_s, char, _Buffer, _In_ const __time64_t *, _Time)

_CRT_INSECURE_DEPRECATE(_gmtime64_s) _CRTIMP struct tm * __cdecl _gmtime64(_In_ const __time64_t * _Time);
_CRTIMP errno_t __cdecl _gmtime64_s(_Out_ struct tm *_Tm, _In_ const __time64_t *_Time);

_CRT_INSECURE_DEPRECATE(_localtime64_s) _CRTIMP struct tm * __cdecl _localtime64(_In_ const __time64_t * _Time);
_CRTIMP errno_t __cdecl _localtime64_s(_Out_ struct tm *_Tm, _In_ const __time64_t *_Time);

_CRTIMP __time64_t __cdecl _mktime64(_Inout_ struct tm * _Tm);
_CRTIMP __time64_t __cdecl _mkgmtime64(_Inout_ struct tm * _Tm);
_CRTIMP __time64_t __cdecl _time64(_Out_opt_ __time64_t * _Time);
*/
// string.h
#ifdef UNICODE 

#define _tcslen     wcslen
#define _tcscpy     wcscpy
#define _tcscpy_s   wcscpy_s
#define _tcsncpy    wcsncpy
#define _tcsncpy_s(_dest, _size, _src, _count)   assert(_count <= _size); wcsncpy(_dest, _src, _count)  
#define _tcscat     wcscat
#define _tcscat_s(_dest, _size, _src)   wcsncat(_dest, _src, _size)
//#define _tcsupr     wcsupr
#define _tcsupr_s   wcsupr_s
//#define _tcslwr     wcslwr
#define _tcslwr_s   wcslwr_s

#define _stprintf_s swprintf_s
#define _stprintf   swprintf
#define _tprintf    wprintf

#define _vstprintf_s    vswprintf_s
#define _vstprintf      vswprintf

#define _tscanf     wscanf


#define TCHAR wchar_t

#else

#define _tcslen     strlen
#define _tcscpy     strcpy
#define _tcscpy_s   strcpy_s
#define _tcsncpy    strncpy
#define _tcsncpy_s  strncpy_s
#define _tcscat     strcat
#define _tcscat_s   strcat_s
#define _tcsupr     strupr
#define _tcsupr_s   strupr_s
#define _tcslwr     strlwr
#define _tcslwr_s   strlwr_s

#define _stprintf_s sprintf_s
#define _stprintf   sprintf
#define _tprintf    printf

#define _vstprintf_s    vsprintf_s
#define _vstprintf      vsprintf

#define _tscanf     scanf

#define TCHAR char
#endif
#endif
