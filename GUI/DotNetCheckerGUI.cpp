#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <mscoree.h>
#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>
#include "DotNetCheckerGUI.h"

using namespace std;

// In case the machine this is compiled on does not have the most recent platform SDK
// with these values defined, define them here
#ifndef SM_TABLETPC
#define SM_TABLETPC		86
#endif

#ifndef SM_MEDIACENTER
#define SM_MEDIACENTER	87
#endif

#define CountOf(x) sizeof(x)/sizeof(*x)

// Constants that represent registry key names and value names
// to use for detection
const TCHAR *g_szNetfx10RegKeyName = _T("Software\\Microsoft\\.NETFramework\\Policy\\v1.0");
const TCHAR *g_szNetfx10RegKeyValue = _T("3705");
const TCHAR *g_szNetfx10SPxMSIRegKeyName = _T("Software\\Microsoft\\Active Setup\\Installed Components\\{78705f0d-e8db-4b2d-8193-982bdda15ecd}");
const TCHAR *g_szNetfx10SPxOCMRegKeyName = _T("Software\\Microsoft\\Active Setup\\Installed Components\\{FDC11A6F-17D1-48f9-9EA3-9051954BAA24}");
const TCHAR *g_szNetfx11RegKeyName = _T("Software\\Microsoft\\NET Framework Setup\\NDP\\v1.1.4322");
const TCHAR *g_szNetfx20RegKeyName = _T("Software\\Microsoft\\NET Framework Setup\\NDP\\v2.0.50727");
const TCHAR *g_szNetfx30RegKeyName = _T("Software\\Microsoft\\NET Framework Setup\\NDP\\v3.0\\Setup");
const TCHAR *g_szNetfx30SpRegKeyName = _T("Software\\Microsoft\\NET Framework Setup\\NDP\\v3.0");
const TCHAR *g_szNetfx30RegValueName = _T("InstallSuccess");
const TCHAR *g_szNetfx35RegKeyName = _T("Software\\Microsoft\\NET Framework Setup\\NDP\\v3.5");
const TCHAR *g_szNetfx40ClientRegKeyName = _T("Software\\Microsoft\\NET Framework Setup\\NDP\\v4\\Client");
const TCHAR *g_szNetfx40FullRegKeyName = _T("Software\\Microsoft\\NET Framework Setup\\NDP\\v4\\Full");
const TCHAR *g_szNetfx40SPxRegValueName = _T("Servicing");
const TCHAR *g_szNetfx45RegKeyName = _T("Software\\Microsoft\\NET Framework Setup\\NDP\\v4\\Full");
const TCHAR *g_szNetfx45RegValueName = _T("Release");
const TCHAR *g_szNetfxStandardRegValueName = _T("Install");
const TCHAR *g_szNetfxStandardSPxRegValueName = _T("SP");
const TCHAR *g_szNetfxStandardVersionRegValueName = _T("Version");

// Version information for final release of .NET Framework 3.0
const int g_iNetfx30VersionMajor = 3;
const int g_iNetfx30VersionMinor = 0;
const int g_iNetfx30VersionBuild = 4506;
const int g_iNetfx30VersionRevision = 26;

// Version information for final release of .NET Framework 3.5
const int g_iNetfx35VersionMajor = 3;
const int g_iNetfx35VersionMinor = 5;
const int g_iNetfx35VersionBuild = 21022;
const int g_iNetfx35VersionRevision = 8;

// Version information for final release of .NET Framework 4
const int g_iNetfx40VersionMajor = 4;
const int g_iNetfx40VersionMinor = 0;
const int g_iNetfx40VersionBuild = 30319;
const int g_iNetfx40VersionRevision = 0;

// Version information for final release of .NET Framework 4.5
const int g_dwNetfx45ReleaseVersion = 378389;

// Version information for final release of .NET Framework 4.5.1
const int g_dwNetfx451ReleaseVersion = 378675;

// Version information for final release of .NET Framework 4.5.2
const int g_dwNetfx452ReleaseVersion = 379893;

// Version information for final release of .NET Framework 4.6
const int g_dwNetfx46ReleaseVersion = 393295;

// Version information for final release of .NET Framework 4.6.1
const int g_dwNetfx461ReleaseVersion = 394254;

// Version information for final release of .NET Framework 4.6.2
const int g_dwNetfx462ReleaseVersion = 394802;

// Version information for final release of .NET Framework 4.7
const int g_dwNetfx47ReleaseVersion = 460798;

// Constants for known .NET Framework versions used with the GetRequestedRuntimeInfo API
const TCHAR *g_szNetfx10VersionString = _T("v1.0.3705");
const TCHAR *g_szNetfx11VersionString = _T("v1.1.4322");
const TCHAR *g_szNetfx20VersionString = _T("v2.0.50727");
const TCHAR *g_szNetfx40VersionString = _T("v4.0.30319");

// Function prototypes
bool CheckNetfxBuildNumber(const TCHAR*, const TCHAR*, const int, const int, const int, const int);
bool CheckNetfxVersionUsingMscoree(const TCHAR*);
int GetNetfx10SPLevel();
int GetNetfxSPLevel(const TCHAR*, const TCHAR*);
DWORD GetProcessorArchitectureFlag();
bool IsCurrentOSTabletMedCenter();
bool IsNetfx10Installed();
bool IsNetfx11Installed();
bool IsNetfx20Installed();
bool IsNetfx30Installed();
bool IsNetfx35Installed();
bool IsNetfx40ClientInstalled();
bool IsNetfx40FullInstalled();
bool IsNetfx45Installed();
bool IsNetfx451Installed();
bool IsNetfx452Installed();
bool IsNetfx46Installed();
bool IsNetfx461Installed();
bool IsNetfx462Installed();
bool IsNetfx47Installed();
bool RegistryGetValue(HKEY, const TCHAR*, const TCHAR*, DWORD, LPBYTE, DWORD);


/******************************************************************
Function Name:  CheckNetfxVersionUsingMscoree
Description:    Uses the logic described in the sample code at
http://msdn2.microsoft.com/library/ydh6b3yb.aspx
to load mscoree.dll and call its APIs to determine
whether or not a specific version of the .NET
Framework is installed on the system
Inputs:         pszNetfxVersionToCheck - version to look for
Results:        true if the requested version is installed
false otherwise
******************************************************************/
bool CheckNetfxVersionUsingMscoree(const TCHAR *pszNetfxVersionToCheck)
{
	bool bFoundRequestedNetfxVersion = false;
	HRESULT hr = S_OK;

	// Check input parameter
	if (NULL == pszNetfxVersionToCheck)
		return false;

	HMODULE hmodMscoree = LoadLibraryEx(_T("mscoree.dll"), NULL, 0);
	if (NULL != hmodMscoree)
	{
		typedef HRESULT(STDAPICALLTYPE *GETCORVERSION)(LPWSTR szBuffer, DWORD cchBuffer, DWORD* dwLength);
		GETCORVERSION pfnGETCORVERSION = (GETCORVERSION)GetProcAddress(hmodMscoree, "GetCORVersion");

		// Some OSs shipped with a placeholder copy of mscoree.dll. The existence of mscoree.dll
		// therefore does NOT mean that a version of the .NET Framework is installed.
		// If this copy of mscoree.dll does not have an exported function named GetCORVersion
		// then we know it is a placeholder DLL.
		if (NULL == pfnGETCORVERSION)
			goto Finish;

		typedef HRESULT(STDAPICALLTYPE *CORBINDTORUNTIME)(LPCWSTR pwszVersion, LPCWSTR pwszBuildFlavor, REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv);
		CORBINDTORUNTIME pfnCORBINDTORUNTIME = (CORBINDTORUNTIME)GetProcAddress(hmodMscoree, "CorBindToRuntime");

		typedef HRESULT(STDAPICALLTYPE *GETREQUESTEDRUNTIMEINFO)(LPCWSTR pExe, LPCWSTR pwszVersion, LPCWSTR pConfigurationFile, DWORD startupFlags, DWORD runtimeInfoFlags, LPWSTR pDirectory, DWORD dwDirectory, DWORD *dwDirectoryLength, LPWSTR pVersion, DWORD cchBuffer, DWORD* dwlength);
		GETREQUESTEDRUNTIMEINFO pfnGETREQUESTEDRUNTIMEINFO = (GETREQUESTEDRUNTIMEINFO)GetProcAddress(hmodMscoree, "GetRequestedRuntimeInfo");

		if (NULL != pfnCORBINDTORUNTIME)
		{
			TCHAR szRetrievedVersion[50];
			DWORD dwLength = CountOf(szRetrievedVersion);

			if (NULL == pfnGETREQUESTEDRUNTIMEINFO)
			{
				// Having CorBindToRuntimeHost but not having GetRequestedRuntimeInfo means that
				// this machine contains no higher than .NET Framework 1.0, but the only way to
				// 100% guarantee that the .NET Framework 1.0 is installed is to call a function
				// to exercise its functionality
				if (0 == _tcscmp(pszNetfxVersionToCheck, g_szNetfx10VersionString))
				{
					hr = pfnGETCORVERSION(szRetrievedVersion, dwLength, &dwLength);

					if (SUCCEEDED(hr))
					{
						if (0 == _tcscmp(szRetrievedVersion, g_szNetfx10VersionString))
							bFoundRequestedNetfxVersion = true;
					}

					goto Finish;
				}
			}

			// Set error mode to prevent the .NET Framework from displaying
			// unfriendly error dialogs
			UINT uOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

			TCHAR szDirectory[MAX_PATH];
			DWORD dwDirectoryLength = 0;
			DWORD dwRuntimeInfoFlags = RUNTIME_INFO_DONT_RETURN_DIRECTORY | GetProcessorArchitectureFlag();

			// Check for the requested .NET Framework version
			hr = pfnGETREQUESTEDRUNTIMEINFO(NULL, pszNetfxVersionToCheck, NULL, STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN_HOST, NULL, szDirectory, CountOf(szDirectory), &dwDirectoryLength, szRetrievedVersion, CountOf(szRetrievedVersion), &dwLength);

			if (SUCCEEDED(hr))
				bFoundRequestedNetfxVersion = true;

			// Restore the previous error mode
			SetErrorMode(uOldErrorMode);
		}
	}

Finish:
	if (hmodMscoree)
	{
		FreeLibrary(hmodMscoree);
	}

	return bFoundRequestedNetfxVersion;
}


/******************************************************************
Function Name:  GetNetfx10SPLevel
Description:    Uses the detection method recommended at
http://blogs.msdn.com/astebner/archive/2004/09/14/229802.aspx
to determine what service pack for the
.NET Framework 1.0 is installed on the machine
Inputs:         NONE
Results:        integer representing SP level for .NET Framework 1.0
******************************************************************/
int GetNetfx10SPLevel()
{
	TCHAR szRegValue[MAX_PATH];
	TCHAR *pszSPLevel = NULL;
	int iRetValue = -1;
	bool bRegistryRetVal = false;

	// Need to detect what OS we are running on so we know what
	// registry key to use to look up the SP level
	if (IsCurrentOSTabletMedCenter())
		bRegistryRetVal = RegistryGetValue(HKEY_LOCAL_MACHINE, g_szNetfx10SPxOCMRegKeyName, g_szNetfxStandardVersionRegValueName, NULL, (LPBYTE)szRegValue, MAX_PATH);
	else
		bRegistryRetVal = RegistryGetValue(HKEY_LOCAL_MACHINE, g_szNetfx10SPxMSIRegKeyName, g_szNetfxStandardVersionRegValueName, NULL, (LPBYTE)szRegValue, MAX_PATH);

	if (bRegistryRetVal)
	{
		// This registry value should be of the format
		// #,#,#####,# where the last # is the SP level
		// Try to parse off the last # here
		pszSPLevel = _tcsrchr(szRegValue, _T(','));
		if (NULL != pszSPLevel)
		{
			// Increment the pointer to skip the comma
			pszSPLevel++;

			// Convert the remaining value to an integer
			iRetValue = _tstoi(pszSPLevel);
		}
	}

	return iRetValue;
}


/******************************************************************
Function Name:	GetNetfxSPLevel
Description:	Determine what service pack is installed for a
version of the .NET Framework using registry
based detection methods documented in the
.NET Framework deployment guides.
Inputs:         pszNetfxRegKeyName - registry key name to use for detection
pszNetfxRegValueName - registry value to use for detection
Results:        integer representing SP level for .NET Framework
******************************************************************/
int GetNetfxSPLevel(const TCHAR *pszNetfxRegKeyName, const TCHAR *pszNetfxRegValueName)
{
	DWORD dwRegValue = 0;

	if (RegistryGetValue(HKEY_LOCAL_MACHINE, pszNetfxRegKeyName, pszNetfxRegValueName, NULL, (LPBYTE)&dwRegValue, sizeof(DWORD)))
	{
		return (int)dwRegValue;
	}

	// We can only get here if the .NET Framework is not
	// installed or there was some kind of error retrieving
	// the data from the registry
	return -1;
}


/******************************************************************
Function Name:  GetProcessorArchitectureFlag
Description:    Determine the processor architecture of the
system (x86, x64, ia64)
Inputs:         NONE
Results:        DWORD processor architecture flag
******************************************************************/
DWORD GetProcessorArchitectureFlag()
{
	HMODULE hmodKernel32 = NULL;
	typedef void (WINAPI *PFnGetNativeSystemInfo) (LPSYSTEM_INFO);
	PFnGetNativeSystemInfo pfnGetNativeSystemInfo;

	SYSTEM_INFO sSystemInfo;
	memset(&sSystemInfo, 0, sizeof(sSystemInfo));

	bool bRetrievedSystemInfo = false;

	// Attempt to load kernel32.dll
	hmodKernel32 = LoadLibrary(_T("Kernel32.dll"));
	if (NULL != hmodKernel32)
	{
		// If the DLL loaded correctly, get the proc address for GetNativeSystemInfo
		pfnGetNativeSystemInfo = (PFnGetNativeSystemInfo)GetProcAddress(hmodKernel32, "GetNativeSystemInfo");
		if (NULL != pfnGetNativeSystemInfo)
		{
			// Call GetNativeSystemInfo if it exists
			(*pfnGetNativeSystemInfo)(&sSystemInfo);
			bRetrievedSystemInfo = true;
		}
		FreeLibrary(hmodKernel32);
	}

	if (!bRetrievedSystemInfo)
	{
		// Fallback to calling GetSystemInfo if the above failed
		GetSystemInfo(&sSystemInfo);
		bRetrievedSystemInfo = true;
	}

	if (bRetrievedSystemInfo)
	{
		switch (sSystemInfo.wProcessorArchitecture)
		{
		case PROCESSOR_ARCHITECTURE_INTEL:
			return RUNTIME_INFO_REQUEST_X86;
		case PROCESSOR_ARCHITECTURE_IA64:
			return RUNTIME_INFO_REQUEST_IA64;
		case PROCESSOR_ARCHITECTURE_AMD64:
			return RUNTIME_INFO_REQUEST_AMD64;
		default:
			return 0;
		}
	}

	return 0;
}


/******************************************************************
Function Name:	CheckNetfxBuildNumber
Description:	Retrieves the .NET Framework build number from
the registry and validates that it is not a pre-release
version number
Inputs:         NONE
Results:        true if the build number in the registry is greater
than or equal to the passed in version; false otherwise
******************************************************************/
bool CheckNetfxBuildNumber(const TCHAR *pszNetfxRegKeyName, const TCHAR *pszNetfxRegKeyValue, const int iRequestedVersionMajor, const int iRequestedVersionMinor, const int iRequestedVersionBuild, const int iRequestedVersionRevision)
{
	TCHAR szRegValue[MAX_PATH];
	TCHAR *pszToken = NULL;
	TCHAR *pszNextToken = NULL;
	int iVersionPartCounter = 0;
	int iRegistryVersionMajor = 0;
	int iRegistryVersionMinor = 0;
	int iRegistryVersionBuild = 0;
	int iRegistryVersionRevision = 0;
	bool bRegistryRetVal = false;

	// Attempt to retrieve the build number registry value
	bRegistryRetVal = RegistryGetValue(HKEY_LOCAL_MACHINE, pszNetfxRegKeyName, pszNetfxRegKeyValue, NULL, (LPBYTE)szRegValue, MAX_PATH);

	if (bRegistryRetVal)
	{
		// This registry value should be of the format
		// #.#.#####.##.  Try to parse the 4 parts of
		// the version here
		pszToken = _tcstok_s(szRegValue, _T("."), &pszNextToken);
		while (NULL != pszToken)
		{
			iVersionPartCounter++;

			switch (iVersionPartCounter)
			{
			case 1:
				// Convert the major version value to an integer
				iRegistryVersionMajor = _tstoi(pszToken);
				break;
			case 2:
				// Convert the minor version value to an integer
				iRegistryVersionMinor = _tstoi(pszToken);
				break;
			case 3:
				// Convert the build number value to an integer
				iRegistryVersionBuild = _tstoi(pszToken);
				break;
			case 4:
				// Convert the revision number value to an integer
				iRegistryVersionRevision = _tstoi(pszToken);
				break;
			default:
				break;

			}

			// Get the next part of the version number
			pszToken = _tcstok_s(NULL, _T("."), &pszNextToken);
		}
	}

	// Compare the version number retrieved from the registry with
	// the version number of the final release of the .NET Framework
	// that we are checking
	if (iRegistryVersionMajor > iRequestedVersionMajor)
	{
		return true;
	}
	else if (iRegistryVersionMajor == iRequestedVersionMajor)
	{
		if (iRegistryVersionMinor > iRequestedVersionMinor)
		{
			return true;
		}
		else if (iRegistryVersionMinor == iRequestedVersionMinor)
		{
			if (iRegistryVersionBuild > iRequestedVersionBuild)
			{
				return true;
			}
			else if (iRegistryVersionBuild == iRequestedVersionBuild)
			{
				if (iRegistryVersionRevision >= iRequestedVersionRevision)
				{
					return true;
				}
			}
		}
	}

	// If we get here, the version in the registry must be less than the
	// version of the final release of the .NET Framework we are checking,
	// so return false
	return false;
}


/******************************************************************
Function Name:  IsCurrentOSTabletMedCenter
Description:    Determine if the current OS is a Windows XP
Tablet PC Edition or Windows XP Media Center
Edition system
Inputs:         NONE
Results:        true if the OS is Tablet PC or Media Center
false otherwise
******************************************************************/
bool IsCurrentOSTabletMedCenter()
{
	// Use GetSystemMetrics to detect if we are on a Tablet PC or Media Center OS  
	return ((GetSystemMetrics(SM_TABLETPC) != 0) || (GetSystemMetrics(SM_MEDIACENTER) != 0));
}


/******************************************************************
Function Name:  IsNetfx10Installed
Description:    Uses the detection method recommended at
http://msdn.microsoft.com/library/ms994349.aspx
to determine whether the .NET Framework 1.0 is
installed on the machine
Inputs:         NONE
Results:        true if the .NET Framework 1.0 is installed
false otherwise
******************************************************************/
bool IsNetfx10Installed()
{
	TCHAR szRegValue[MAX_PATH];
	return (RegistryGetValue(HKEY_LOCAL_MACHINE, g_szNetfx10RegKeyName, g_szNetfx10RegKeyValue, NULL, (LPBYTE)szRegValue, MAX_PATH));
}


/******************************************************************
Function Name:  IsNetfx11Installed
Description:    Uses the detection method recommended at
http://msdn.microsoft.com/library/ms994339.aspx
to determine whether the .NET Framework 1.1 is
installed on the machine
Inputs:         NONE
Results:        true if the .NET Framework 1.1 is installed
false otherwise
******************************************************************/
bool IsNetfx11Installed()
{
	bool bRetValue = false;
	DWORD dwRegValue = 0;

	if (RegistryGetValue(HKEY_LOCAL_MACHINE, g_szNetfx11RegKeyName, g_szNetfxStandardRegValueName, NULL, (LPBYTE)&dwRegValue, sizeof(DWORD)))
	{
		if (1 == dwRegValue)
			bRetValue = true;
	}

	return bRetValue;
}


/******************************************************************
Function Name:	IsNetfx20Installed
Description:	Uses the detection method recommended at
http://msdn2.microsoft.com/library/aa480243.aspx
to determine whether the .NET Framework 2.0 is
installed on the machine
Inputs:         NONE
Results:        true if the .NET Framework 2.0 is installed
false otherwise
******************************************************************/
bool IsNetfx20Installed()
{
	bool bRetValue = false;
	DWORD dwRegValue = 0;

	if (RegistryGetValue(HKEY_LOCAL_MACHINE, g_szNetfx20RegKeyName, g_szNetfxStandardRegValueName, NULL, (LPBYTE)&dwRegValue, sizeof(DWORD)))
	{
		if (1 == dwRegValue)
			bRetValue = true;
	}

	return bRetValue;
}


/******************************************************************
Function Name:	IsNetfx30Installed
Description:	Uses the detection method recommended at
http://msdn.microsoft.com/library/aa964979.aspx
to determine whether the .NET Framework 3.0 is
installed on the machine
Inputs:	        NONE
Results:        true if the .NET Framework 3.0 is installed
false otherwise
******************************************************************/
bool IsNetfx30Installed()
{
	bool bRetValue = false;
	DWORD dwRegValue = 0;

	// Check that the InstallSuccess registry value exists and equals 1
	if (RegistryGetValue(HKEY_LOCAL_MACHINE, g_szNetfx30RegKeyName, g_szNetfx30RegValueName, NULL, (LPBYTE)&dwRegValue, sizeof(DWORD)))
	{
		if (1 == dwRegValue)
			bRetValue = true;
	}

	// A system with a pre-release version of the .NET Framework 3.0 can
	// have the InstallSuccess value.  As an added verification, check the
	// version number listed in the registry
	return (bRetValue && CheckNetfxBuildNumber(g_szNetfx30RegKeyName, g_szNetfxStandardVersionRegValueName, g_iNetfx30VersionMajor, g_iNetfx30VersionMinor, g_iNetfx30VersionBuild, g_iNetfx30VersionRevision));
}


/******************************************************************
Function Name:	IsNetfx35Installed
Description:	Uses the detection method recommended at
http://msdn.microsoft.com/library/cc160716.aspx
to determine whether the .NET Framework 3.5 is
installed on the machine
Inputs:	        NONE
Results:        true if the .NET Framework 3.5 is installed
false otherwise
******************************************************************/
bool IsNetfx35Installed()
{
	bool bRetValue = false;
	DWORD dwRegValue = 0;

	// Check that the Install registry value exists and equals 1
	if (RegistryGetValue(HKEY_LOCAL_MACHINE, g_szNetfx35RegKeyName, g_szNetfxStandardRegValueName, NULL, (LPBYTE)&dwRegValue, sizeof(DWORD)))
	{
		if (1 == dwRegValue)
			bRetValue = true;
	}

	// A system with a pre-release version of the .NET Framework 3.5 can
	// have the Install value.  As an added verification, check the
	// version number listed in the registry
	return (bRetValue && CheckNetfxBuildNumber(g_szNetfx35RegKeyName, g_szNetfxStandardVersionRegValueName, g_iNetfx35VersionMajor, g_iNetfx35VersionMinor, g_iNetfx35VersionBuild, g_iNetfx35VersionRevision));
}


/******************************************************************
Function Name:	IsNetfx40ClientInstalled
Description:	Uses the detection method recommended at
http://msdn.microsoft.com/library/ee942965(v=VS.100).aspx
to determine whether the .NET Framework 4 Client is
installed on the machine
Inputs:         NONE
Results:        true if the .NET Framework 4 Client is installed
false otherwise
******************************************************************/
bool IsNetfx40ClientInstalled()
{
	bool bRetValue = false;
	DWORD dwRegValue = 0;

	if (RegistryGetValue(HKEY_LOCAL_MACHINE, g_szNetfx40ClientRegKeyName, g_szNetfxStandardRegValueName, NULL, (LPBYTE)&dwRegValue, sizeof(DWORD)))
	{
		if (1 == dwRegValue)
			bRetValue = true;
	}

	// A system with a pre-release version of the .NET Framework 4 can
	// have the Install value.  As an added verification, check the
	// version number listed in the registry
	return (bRetValue && CheckNetfxBuildNumber(g_szNetfx40ClientRegKeyName, g_szNetfxStandardVersionRegValueName, g_iNetfx40VersionMajor, g_iNetfx40VersionMinor, g_iNetfx40VersionBuild, g_iNetfx40VersionRevision));
}


/******************************************************************
Function Name:	IsNetfx40FullInstalled
Description:	Uses the detection method recommended at
http://msdn.microsoft.com/library/ee942965(v=VS.100).aspx
to determine whether the .NET Framework 4 Full is
installed on the machine
Inputs:         NONE
Results:        true if the .NET Framework 4 Full is installed
false otherwise
******************************************************************/
bool IsNetfx40FullInstalled()
{
	bool bRetValue = false;
	DWORD dwRegValue = 0;

	if (RegistryGetValue(HKEY_LOCAL_MACHINE, g_szNetfx40FullRegKeyName, g_szNetfxStandardRegValueName, NULL, (LPBYTE)&dwRegValue, sizeof(DWORD)))
	{
		if (1 == dwRegValue)
			bRetValue = true;
	}

	// A system with a pre-release version of the .NET Framework 4 can
	// have the Install value.  As an added verification, check the
	// version number listed in the registry
	return (bRetValue && CheckNetfxBuildNumber(g_szNetfx40FullRegKeyName, g_szNetfxStandardVersionRegValueName, g_iNetfx40VersionMajor, g_iNetfx40VersionMinor, g_iNetfx40VersionBuild, g_iNetfx40VersionRevision));
}


/******************************************************************
Function Name:	IsNetfx45Installed
Description:	Uses the detection method recommended at
http://msdn.microsoft.com/en-us/library/ee942965(v=vs.110).aspx
to determine whether the .NET Framework 4.5 is
installed on the machine
Inputs:         NONE
Results:        true if the .NET Framework 4.5 is installed
false otherwise
******************************************************************/
bool IsNetfx45Installed()
{
	bool bRetValue = false;
	DWORD dwRegValue = 0;

	if (RegistryGetValue(HKEY_LOCAL_MACHINE, g_szNetfx45RegKeyName, g_szNetfx45RegValueName, NULL, (LPBYTE)&dwRegValue, sizeof(DWORD)))
	{
		if (g_dwNetfx45ReleaseVersion <= dwRegValue)
			bRetValue = true;
	}

	return bRetValue;
}


/******************************************************************
Function Name:	IsNetfx451Installed
Description:	Uses the detection method recommended at
http://msdn.microsoft.com/en-us/library/ee942965(v=vs.110).aspx
to determine whether the .NET Framework 4.5.1 is
installed on the machine
Inputs:         NONE
Results:        true if the .NET Framework 4.5.1 is installed
false otherwise
******************************************************************/
bool IsNetfx451Installed()
{
	bool bRetValue = false;
	DWORD dwRegValue = 0;

	if (RegistryGetValue(HKEY_LOCAL_MACHINE, g_szNetfx45RegKeyName, g_szNetfx45RegValueName, NULL, (LPBYTE)&dwRegValue, sizeof(DWORD)))
	{
		if (g_dwNetfx451ReleaseVersion <= dwRegValue)
			bRetValue = true;
	}

	return bRetValue;
}


/******************************************************************
Function Name:	IsNetfx452Installed
Description:	Uses the detection method recommended at
http://msdn.microsoft.com/en-us/library/ee942965(v=vs.110).aspx
to determine whether the .NET Framework 4.5.2 is
installed on the machine
Inputs:         NONE
Results:        true if the .NET Framework 4.5.2 is installed
false otherwise
******************************************************************/
bool IsNetfx452Installed()
{
	bool bRetValue = false;
	DWORD dwRegValue = 0;

	if (RegistryGetValue(HKEY_LOCAL_MACHINE, g_szNetfx45RegKeyName, g_szNetfx45RegValueName, NULL, (LPBYTE)&dwRegValue, sizeof(DWORD)))
	{
		if (g_dwNetfx452ReleaseVersion <= dwRegValue)
			bRetValue = true;
	}

	return bRetValue;
}


/******************************************************************
Function Name:	IsNetfx46Installed
Description:	Uses the detection method recommended at
http://msdn.microsoft.com/en-us/library/ee942965(v=vs.110).aspx
to determine whether the .NET Framework 4.6 is
installed on the machine
Inputs:         NONE
Results:        true if the .NET Framework 4.6 is installed
false otherwise
******************************************************************/
bool IsNetfx46Installed()
{
	bool bRetValue = false;
	DWORD dwRegValue = 0;

	if (RegistryGetValue(HKEY_LOCAL_MACHINE, g_szNetfx45RegKeyName, g_szNetfx45RegValueName, NULL, (LPBYTE)&dwRegValue, sizeof(DWORD)))
	{
		if (g_dwNetfx46ReleaseVersion <= dwRegValue)
			bRetValue = true;
	}

	return bRetValue;
}


/******************************************************************
Function Name:	IsNetfx461Installed
Description:	Uses the detection method recommended at
http://msdn.microsoft.com/en-us/library/ee942965(v=vs.110).aspx
to determine whether the .NET Framework 4.6.1 is
installed on the machine
Inputs:         NONE
Results:        true if the .NET Framework 4.6.1 is installed
false otherwise
******************************************************************/
bool IsNetfx461Installed()
{
	bool bRetValue = false;
	DWORD dwRegValue = 0;

	if (RegistryGetValue(HKEY_LOCAL_MACHINE, g_szNetfx45RegKeyName, g_szNetfx45RegValueName, NULL, (LPBYTE)&dwRegValue, sizeof(DWORD)))
	{
		if (g_dwNetfx461ReleaseVersion <= dwRegValue)
			bRetValue = true;
	}

	return bRetValue;
}


/******************************************************************
Function Name:	IsNetfx462Installed
Description:	Uses the detection method recommended at
http://msdn.microsoft.com/en-us/library/ee942965(v=vs.110).aspx
to determine whether the .NET Framework 4.6.2 is
installed on the machine
Inputs:         NONE
Results:        true if the .NET Framework 4.6.2 is installed
false otherwise
******************************************************************/
bool IsNetfx462Installed()
{
	bool bRetValue = false;
	DWORD dwRegValue = 0;

	if (RegistryGetValue(HKEY_LOCAL_MACHINE, g_szNetfx45RegKeyName, g_szNetfx45RegValueName, NULL, (LPBYTE)&dwRegValue, sizeof(DWORD)))
	{
		if (g_dwNetfx462ReleaseVersion <= dwRegValue)
			bRetValue = true;
	}

	return bRetValue;
}


/******************************************************************
Function Name:	IsNetfx47Installed
Description:	Uses the detection method recommended at
https://msdn.microsoft.com/en-us/library/ee942965(v=vs.110).aspx
to determine whether the .NET Framework 4.7 is
installed on the machine
Inputs:         NONE
Results:        true if the .NET Framework 4.7 is installed
false otherwise
******************************************************************/
bool IsNetfx47Installed()
{
	bool bRetValue = false;
	DWORD dwRegValue = 0;

	if (RegistryGetValue(HKEY_LOCAL_MACHINE, g_szNetfx45RegKeyName, g_szNetfx45RegValueName, NULL, (LPBYTE)&dwRegValue, sizeof(DWORD)))
	{
		if (g_dwNetfx47ReleaseVersion <= dwRegValue)
			bRetValue = true;
	}

	return bRetValue;
}


/******************************************************************
Function Name:  RegistryGetValue
Description:    Get the value of a reg key
Inputs:         HKEY hk - The hk of the key to retrieve
TCHAR *pszKey - Name of the key to retrieve
TCHAR *pszValue - The value that will be retrieved
DWORD dwType - The type of the value that will be retrieved
LPBYTE data - A buffer to save the retrieved data
DWORD dwSize - The size of the data retrieved
Results:        true if successful, false otherwise
******************************************************************/
bool RegistryGetValue(HKEY hk, const TCHAR * pszKey, const TCHAR * pszValue, DWORD dwType, LPBYTE data, DWORD dwSize)
{
	HKEY hkOpened;

	// Try to open the key
	if (RegOpenKeyEx(hk, pszKey, 0, KEY_READ, &hkOpened) != ERROR_SUCCESS)
	{
		return false;
	}

	// If the key was opened, try to retrieve the value
	if (RegQueryValueEx(hkOpened, pszValue, 0, &dwType, (LPBYTE)data, &dwSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hkOpened);
		return false;
	}

	// Clean up
	RegCloseKey(hkOpened);

	return true;
}


PCHAR*
CommandLineToArgvA2(
	PCHAR CmdLine,
	int* _argc
)
{
	PCHAR* argv;
	PCHAR  _argv;
	ULONG   len;
	ULONG   argc;
	CHAR   a;
	ULONG   i, j;

	BOOLEAN  in_QM;
	BOOLEAN  in_TEXT;
	BOOLEAN  in_SPACE;

	len = strlen(CmdLine);
	i = ((len + 2) / 2) * sizeof(PVOID) + sizeof(PVOID);

	argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
		i + (len + 2) * sizeof(CHAR));

	_argv = (PCHAR)(((PUCHAR)argv) + i);

	argc = 0;
	argv[argc] = _argv;
	in_QM = FALSE;
	in_TEXT = FALSE;
	in_SPACE = TRUE;
	i = 0;
	j = 0;

	while (a = CmdLine[i]) {
		if (in_QM) {
			if (a == '\"') {
				in_QM = FALSE;
			}
			else {
				_argv[j] = a;
				j++;
			}
		}
		else {
			switch (a) {
			case '\"':
				in_QM = TRUE;
				in_TEXT = TRUE;
				if (in_SPACE) {
					argv[argc] = _argv + j;
					argc++;
				}
				in_SPACE = FALSE;
				break;
			case ' ':
			case '\t':
			case '\n':
			case '\r':
				if (in_TEXT) {
					_argv[j] = '\0';
					j++;
				}
				in_TEXT = FALSE;
				in_SPACE = TRUE;
				break;
			default:
				in_TEXT = TRUE;
				if (in_SPACE) {
					argv[argc] = _argv + j;
					argc++;
				}
				_argv[j] = a;
				j++;
				in_SPACE = FALSE;
				break;
			}
		}
		i++;
	}
	_argv[j] = '\0';
	argv[argc] = NULL;

	(*_argc) = argc;
	return argv;
}

/*************************************************************************
* CommandLineToArgvA            [SHELL32.@]
*
* MODIFIED FROM https://www.winehq.org/ project
* We must interpret the quotes in the command line to rebuild the argv
* array correctly:
* - arguments are separated by spaces or tabs
* - quotes serve as optional argument delimiters
*   '"a b"'   -> 'a b'
* - escaped quotes must be converted back to '"'
*   '\"'      -> '"'
* - consecutive backslashes preceding a quote see their number halved with
*   the remainder escaping the quote:
*   2n   backslashes + quote -> n backslashes + quote as an argument delimiter
*   2n+1 backslashes + quote -> n backslashes + literal quote
* - backslashes that are not followed by a quote are copied literally:
*   'a\b'     -> 'a\b'
*   'a\\b'    -> 'a\\b'
* - in quoted strings, consecutive quotes see their number divided by three
*   with the remainder modulo 3 deciding whether to close the string or not.
*   Note that the opening quote must be counted in the consecutive quotes,
*   that's the (1+) below:
*   (1+) 3n   quotes -> n quotes
*   (1+) 3n+1 quotes -> n quotes plus closes the quoted string
*   (1+) 3n+2 quotes -> n+1 quotes plus closes the quoted string
* - in unquoted strings, the first quote opens the quoted string and the
*   remaining consecutive quotes follow the above rule.
*/

LPSTR* WINAPI CommandLineToArgvA(LPSTR lpCmdline, int* numargs)
{
	DWORD argc;
	LPSTR  *argv;
	LPSTR s;
	LPSTR d;
	LPSTR cmdline;
	int qcount, bcount;

	if (!numargs || *lpCmdline == 0)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
	}

	/* --- First count the arguments */
	argc = 1;
	s = lpCmdline;
	/* The first argument, the executable path, follows special rules */
	if (*s == '"')
	{
		/* The executable path ends at the next quote, no matter what */
		s++;
		while (*s)
			if (*s++ == '"')
				break;
	}
	else
	{
		/* The executable path ends at the next space, no matter what */
		while (*s && *s != ' ' && *s != '\t')
			s++;
	}
	/* skip to the first argument, if any */
	while (*s == ' ' || *s == '\t')
		s++;
	if (*s)
		argc++;

	/* Analyze the remaining arguments */
	qcount = bcount = 0;
	while (*s)
	{
		if ((*s == ' ' || *s == '\t') && qcount == 0)
		{
			/* skip to the next argument and count it if any */
			while (*s == ' ' || *s == '\t')
				s++;
			if (*s)
				argc++;
			bcount = 0;
		}
		else if (*s == '\\')
		{
			/* '\', count them */
			bcount++;
			s++;
		}
		else if (*s == '"')
		{
			/* '"' */
			if ((bcount & 1) == 0)
				qcount++; /* unescaped '"' */
			s++;
			bcount = 0;
			/* consecutive quotes, see comment in copying code below */
			while (*s == '"')
			{
				qcount++;
				s++;
			}
			qcount = qcount % 3;
			if (qcount == 2)
				qcount = 0;
		}
		else
		{
			/* a regular character */
			bcount = 0;
			s++;
		}
	}

	/* Allocate in a single lump, the string array, and the strings that go
	* with it. This way the caller can make a single LocalFree() call to free
	* both, as per MSDN.
	*/
	//AV: bad caused error, now typecasting (LPTSTR)
	argv = (LPSTR *)LocalAlloc(LMEM_FIXED, (argc + 1) * sizeof(LPSTR) + (strlen(lpCmdline) + 1) * sizeof(char));
	if (!argv)
		return NULL;
	cmdline = (LPSTR)(argv + argc + 1);
	strcpy(cmdline, lpCmdline);

	/* --- Then split and copy the arguments */
	argv[0] = d = cmdline;
	argc = 1;
	/* The first argument, the executable path, follows special rules */
	if (*d == '"')
	{
		/* The executable path ends at the next quote, no matter what */
		s = d + 1;
		while (*s)
		{
			if (*s == '"')
			{
				s++;
				break;
			}
			*d++ = *s++;
		}
	}
	else
	{
		/* The executable path ends at the next space, no matter what */
		while (*d && *d != ' ' && *d != '\t')
			d++;
		s = d;
		if (*s)
			s++;
	}
	/* close the executable path */
	*d++ = 0;
	/* skip to the first argument and initialize it if any */
	while (*s == ' ' || *s == '\t')
		s++;
	if (!*s)
	{
		/* There are no parameters so we are all done */
		argv[argc] = NULL;
		*numargs = argc;
		return argv;
	}

	/* Split and copy the remaining arguments */
	argv[argc++] = d;
	qcount = bcount = 0;
	while (*s)
	{
		if ((*s == ' ' || *s == '\t') && qcount == 0)
		{
			/* close the argument */
			*d++ = 0;
			bcount = 0;

			/* skip to the next one and initialize it if any */
			do {
				s++;
			} while (*s == ' ' || *s == '\t');
			if (*s)
				argv[argc++] = d;
		}
		else if (*s == '\\')
		{
			*d++ = *s++;
			bcount++;
		}
		else if (*s == '"')
		{
			if ((bcount & 1) == 0)
			{
				/* Preceded by an even number of '\', this is half that
				* number of '\', plus a quote which we erase.
				*/
				d -= bcount / 2;
				qcount++;
			}
			else
			{
				/* Preceded by an odd number of '\', this is half that
				* number of '\' followed by a '"'
				*/
				d = d - bcount / 2 - 1;
				*d++ = '"';
			}
			s++;
			bcount = 0;
			/* Now count the number of consecutive quotes. Note that qcount
			* already takes into account the opening quote if any, as well as
			* the quote that lead us here.
			*/
			while (*s == '"')
			{
				if (++qcount == 3)
				{
					*d++ = '"';
					qcount = 0;
				}
				s++;
			}
			if (qcount == 2)
				qcount = 0;
		}
		else
		{
			/* a regular character */
			*d++ = *s++;
			bcount = 0;
		}
	}
	*d = '\0';
	argv[argc] = NULL;
	*numargs = argc;

	return argv;
}

char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
	char ** itr = std::find(begin, end, option);
	if (itr != end && ++itr != end)
	{
		return *itr;
	}
	return 0;
}

/*
bool version::getinfo()
{
	string temp;
	bool bResult = false;
	DWORD size;
	DWORD dummy;
	char filename[130];
	unsigned int len;
	GetModuleFileName(NULL, filename, 128);
	size = GetFileVersionInfoSize(filename, &dummy);
	if (size == 0)
	{
		this->ver = "No Version Information!";
		return true;
	}
	char* buffer = new char[size];
	VS_FIXEDFILEINFO* data = NULL;
	if (buffer == NULL) { return true; }
	bResult = GetFileVersionInfo(filename, 0, size, (void*)buffer);
	if (!bResult)
	{
		this->ver = STRLASTERROR; // STRLASTERROR is a custom macro
		return true;
	}
	bResult = VerQueryValue(buffer, "\\", (void**)&data, &len);
	if (!bResult || data == NULL || len != sizeof(VS_FIXEDFILEINFO))
	{
		this->ver = "Could Not Retrieve Values!";
		return true;
	}
	// here I would extract the needed values
	delete[] buffer;
	this->valid = true;
	return false;
}
*/

//Msg Box to show parameters
void ErrMsg(const wchar_t * strType, const char * strMsg,  ...) {

	va_list vl;
	va_start(vl, strMsg);
	char cBuff[1024];  // May need to be bigger
	vsprintf(cBuff, strMsg, vl);
	wchar_t wBuff[1024];
	mbstowcs(wBuff, cBuff, strlen(cBuff) + 1);//Plus null

	MessageBox(NULL, wBuff, strType, MB_OK);
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
	return std::find(begin, end, option) != end;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
/*	LPWSTR *szArgList;
	int argCount;
	int choice;
	HANDLE hFile;
	*/
	int argc;
	
	bool setPrint = false;
	bool verFound = false;
	bool fileOut = false;

	setlocale(LC_ALL, "");

	LPSTR * argv = CommandLineToArgvA(GetCommandLineA(), &argc);

	LPWSTR fName;

	TCHAR szMessage[MAX_PATH];
	TCHAR szOutputString[MAX_PATH * 20];

	



	int iNetfx10SPLevel = -1;
	int iNetfx11SPLevel = -1;
	int iNetfx20SPLevel = -1;
	int iNetfx30SPLevel = -1;
	int iNetfx35SPLevel = -1;
	int iNetfx40ClientSPLevel = -1;
	int iNetfx40FullSPLevel = -1;
	int iNetfx45SPLevel = -1;
	int iNetfx451SPLevel = -1;
	int iNetfx452SPLevel = -1;
	int iNetfx46SPLevel = -1;
	int iNetfx461SPLevel = -1;
	int iNetfx462SPLevel = -1;
	int iNetfx47SPLevel = -1;

	if (cmdOptionExists(argv, argv + argc, "-h"))
	{
		//hilfe
		//MessageBox(NULL, L"Hilfe", L"Hilfe", MB_OK);
		wchar_t wtext3[20];
		mbstowcs(wtext3, VER_FILEVERSION_STR, strlen(VER_FILEVERSION_STR) + 1);//Plus null
																			   //LPWSTR ptr3 = wtext3;

		_stprintf_s(szMessage, MAX_PATH, _T("Net Version Check  V %s"), wtext3);
		_tcscpy_s(szOutputString, szMessage);
		_tcscat_s(szOutputString, _T("\nOptions:"));
		_tcscat_s(szOutputString, _T("\n  -v x.y.z   Find Version x.y.z, return TRUE if found"));
		_tcscat_s(szOutputString, _T("\n             Version can be 1.0 1.1 2.0 3.0 3.5 4.0 4.5"));
		_tcscat_s(szOutputString, _T("\n             4.5.1 4.5.2 4.6 4.6.1 4.6.2 4.7"));
		_tcscat_s(szOutputString, _T("\n  -h         This Help screen"));
		_tcscat_s(szOutputString, _T("\n  -p         Print Versions on Screen"));
		_tcscat_s(szOutputString, _T("\n  -f out.ext Output to File out.txt"));
		_tcscat_s(szOutputString, _T("\n\n  only -p and -f can be combined"));

		MessageBox(NULL, szOutputString, _T("Help"), MB_OK | MB_ICONINFORMATION);
		LocalFree(argv);
		return true;
	}

	// Determine whether or not the .NET Framework
	// 1.0, 1.1, 2.0, 3.0, 3.5, 4, 4.5, 4.5.1, 4.5.2, 4.6, 4.6.1, 4.6.2 or 4.7 are installed
	bool bNetfx10Installed = (IsNetfx10Installed() && CheckNetfxVersionUsingMscoree(g_szNetfx10VersionString));
	bool bNetfx11Installed = (IsNetfx11Installed() && CheckNetfxVersionUsingMscoree(g_szNetfx11VersionString));
	bool bNetfx20Installed = (IsNetfx20Installed() && CheckNetfxVersionUsingMscoree(g_szNetfx20VersionString));

	// The .NET Framework 3.0 is an add-in that installs
	// on top of the .NET Framework 2.0.  For this version
	// check, validate that both 2.0 and 3.0 are installed.
	bool bNetfx30Installed = (IsNetfx20Installed() && IsNetfx30Installed() && CheckNetfxVersionUsingMscoree(g_szNetfx20VersionString));

	// The .NET Framework 3.5 is an add-in that installs
	// on top of the .NET Framework 2.0 and 3.0.  For this version
	// check, validate that 2.0, 3.0 and 3.5 are installed.
	bool bNetfx35Installed = (IsNetfx20Installed() && IsNetfx30Installed() && IsNetfx35Installed() && CheckNetfxVersionUsingMscoree(g_szNetfx20VersionString));

	bool bNetfx40ClientInstalled = (IsNetfx40ClientInstalled() && CheckNetfxVersionUsingMscoree(g_szNetfx40VersionString));
	bool bNetfx40FullInstalled = (IsNetfx40FullInstalled() && CheckNetfxVersionUsingMscoree(g_szNetfx40VersionString));

	// The .NET Framework 4.5, 4.5.1, 4.5.2, 4.6, 4.6.1, 4.6.2 and 4.7 are in-place replacements for the .NET Framework 4.
	// They use the same runtime version as the .NET Framework 4.
	bool bNetfx45Installed = (IsNetfx45Installed() && CheckNetfxVersionUsingMscoree(g_szNetfx40VersionString));
	bool bNetfx451Installed = (IsNetfx451Installed() && CheckNetfxVersionUsingMscoree(g_szNetfx40VersionString));
	bool bNetfx452Installed = (IsNetfx452Installed() && CheckNetfxVersionUsingMscoree(g_szNetfx40VersionString));
	bool bNetfx46Installed = (IsNetfx46Installed() && CheckNetfxVersionUsingMscoree(g_szNetfx40VersionString));
	bool bNetfx461Installed = (IsNetfx461Installed() && CheckNetfxVersionUsingMscoree(g_szNetfx40VersionString));
	bool bNetfx462Installed = (IsNetfx462Installed() && CheckNetfxVersionUsingMscoree(g_szNetfx40VersionString));
	bool bNetfx47Installed = (IsNetfx47Installed() && CheckNetfxVersionUsingMscoree(g_szNetfx40VersionString));




	if (cmdOptionExists(argv, argv + argc, "-v"))
	{
		//vergleich mit version
		char * version = getCmdOption(argv, argv + argc, "-v");
		wchar_t wtext1[20];
		mbstowcs(wtext1, version, strlen(version) + 1);//Plus null
		LPWSTR ptr1 = wtext1;
		int iDbg1 = lstrcmpW(ptr1, L"4.6.1");
		int iDbg2 = lstrcmpW(ptr1, L"1.0");
		if (lstrcmpW(ptr1, L"1.0") == 0) {
			if (bNetfx10Installed) {
				return true;
			}
			else
			{
				return false;

			}
		}
		else if (lstrcmpW(ptr1, L"1.1") == 0) {
			if (bNetfx11Installed) {
				return true;
			}
			else
			{
				return false;

			}
		}
		else if (lstrcmpW(ptr1, L"2.0") == 0) {
			if (bNetfx20Installed) {
				return true;
			}
			else
			{
				return false;

			}
		}
		else if (lstrcmpW(ptr1, L"3.0") == 0) {
			if (bNetfx30Installed) {
				return true;
			}
			else
			{
				return false;

			}
		}
		else if (lstrcmpW(ptr1, L"3.5") == 0) {
			if (bNetfx35Installed) {
				return true;
			}
			else
			{
				return false;

			}
		}
		else if (lstrcmpW(ptr1, L"4.0") == 0) {
			if (bNetfx40ClientInstalled || bNetfx40FullInstalled) {
				return true;
			}
			else
			{
				return false;

			}
		}
		else if (lstrcmpW(ptr1, L"4.5") == 0) {
			if (bNetfx45Installed) {
				return true;
			}
			else
			{
				return false;

			}
		}
		else if (lstrcmpW(ptr1, L"4.5.1") == 0) {
			if (bNetfx451Installed) {
				return true;
			}
			else
			{
				return false;

			}
		}
		else if (lstrcmpW(ptr1, L"4.5.2") == 0) {
			if (bNetfx452Installed) {
				return true;
			}
			else
			{
				return false;

			}
		}
		else if (lstrcmpW(ptr1, L"4.6") == 0) {
			if (bNetfx46Installed) {
				return true;
			}
			else
			{
				return false;

			}
		}
		else if (lstrcmpW(ptr1, L"4.6.1") == 0) {
			if (bNetfx461Installed) {
				return true;
			}
			else
			{
				return false;

			}
		}
		else if (lstrcmpW(ptr1, L"4.6.2") == 0) {
			if (bNetfx462Installed) {
				return true;
			}
			else
			{
				return false;

			}
		}
		else if (lstrcmpW(ptr1, L"4.7") == 0) {
			if (bNetfx47Installed) {
				return true;
			}
			else
			{
				return false;

			}
		}
		else {
			return false;
		}
		//MessageBox(NULL, ptr1, L"Version", MB_OK);
		
	}

	
	//no parameter ?
	if (argc <= 1)
	{
		setPrint = true;
	}
	if (cmdOptionExists(argv, argv + argc, "-p"))
	{
		//hilfe
		setPrint = true;
		//MessageBox(NULL, L"Print", L"Print", MB_OK);
	}
	if (cmdOptionExists(argv, argv + argc, "-f"))
	{
		//ausgabe in datei
		char * filename = getCmdOption(argv, argv + argc, "-f");
		wchar_t wtext2[20];
		if (filename == NULL) {
			MessageBox(NULL, L"Filename missing", L"File", MB_OK);
			return false;
		}
		//not null now it can be copied
		mbstowcs(wtext2, filename, strlen(filename) + 1);//Plus null
		std::string sFilename(filename);

		if (strlen(filename)<=2) //to short, probably next parameter
		{
			
			ErrMsg(L"File", "Filename to short: %s", sFilename.c_str());
			//MessageBox(NULL, L"Filename to short", L"File", MB_OK);
			return false;
		}
		if ((strlen(filename) == 3) && (strstr(filename, "nul") != 0 || strstr(filename, "com") != 0 || strstr(filename, "lpt") != 0 || strstr(filename, "con") != 0 || strstr(filename, "aux") != 0)) {
			
			ErrMsg(L"File", "Filename uses system reserved name: %s", sFilename.c_str());
			//MessageBox(NULL, L"Filename uses system reserved names", L"File", MB_OK);
			return false;
		}
		for (int i = 0; filename[i] != '\0'; ++i)
		{
			if ( '\\' == filename[i] || '\/' == filename[i] || '\<' == filename[i] || '\>' == filename[i] || '\|' == filename[i] || '\"' == filename[i] || '\?' == filename[i] || '\*' == filename[i])
			{
				ErrMsg(L"File", "Illegal character \"%c\" in Filename: %s", filename[i], sFilename.c_str());
				//MessageBox(NULL, L"Filename has illegal characters", L"File", MB_OK);
				return false;
			}
		}
		
		
		fName = wtext2;
		fileOut = true;
		//MessageBox(NULL, fName, L"File", MB_OK);
	}
	LocalFree(argv);

	/*
	szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);

	if (szArgList == NULL)
	{
		choice = 0;
	}

	for (int i = 0; i < argCount; i++)
	{
		MessageBox(NULL, szArgList[i], L"Arglist contents", MB_OK);
		
	}

	LocalFree(szArgList);
	*/
	
	// If .NET Framework 1.0 is installed, get the
	// service pack level
	if (bNetfx10Installed)
	{
		iNetfx10SPLevel = GetNetfx10SPLevel();

		if (iNetfx10SPLevel > 0)
			_stprintf_s(szMessage, MAX_PATH, _T(".NET Framework 1.0 service pack %i is installed."), iNetfx10SPLevel);
		else
			_stprintf_s(szMessage, MAX_PATH, _T(".NET Framework 1.0 is installed with no service packs."));

		_tcscpy_s(szOutputString, szMessage);
	}
	else
	{
		_tcscpy_s(szOutputString, _T(".NET Framework 1.0 is not installed."));
	}

	// If .NET Framework 1.1 is installed, get the
	// service pack level
	if (bNetfx11Installed)
	{
		iNetfx11SPLevel = GetNetfxSPLevel(g_szNetfx11RegKeyName, g_szNetfxStandardSPxRegValueName);

		if (iNetfx11SPLevel > 0)
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 1.1 service pack %i is installed."), iNetfx11SPLevel);
		else
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 1.1 is installed with no service packs."));

		_tcscat_s(szOutputString, szMessage);
	}
	else
	{
		_tcscat_s(szOutputString, _T("\n\n.NET Framework 1.1 is not installed."));
	}

	// If .NET Framework 2.0 is installed, get the
	// service pack level
	if (bNetfx20Installed)
	{
		iNetfx20SPLevel = GetNetfxSPLevel(g_szNetfx20RegKeyName, g_szNetfxStandardSPxRegValueName);

		if (iNetfx20SPLevel > 0)
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 2.0 service pack %i is installed."), iNetfx20SPLevel);
		else
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 2.0 is installed with no service packs."));

		_tcscat_s(szOutputString, szMessage);
	}
	else
	{
		_tcscat_s(szOutputString, _T("\n\n.NET Framework 2.0 is not installed."));
	}

	// If .NET Framework 3.0 is installed, get the
	// service pack level
	if (bNetfx30Installed)
	{
		iNetfx30SPLevel = GetNetfxSPLevel(g_szNetfx30SpRegKeyName, g_szNetfxStandardSPxRegValueName);

		if (iNetfx30SPLevel > 0)
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 3.0 service pack %i is installed."), iNetfx30SPLevel);
		else
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 3.0 is installed with no service packs."));

		_tcscat_s(szOutputString, szMessage);
	}
	else
	{
		_tcscat_s(szOutputString, _T("\n\n.NET Framework 3.0 is not installed."));
	}

	// If .NET Framework 3.5 is installed, get the
	// service pack level
	if (bNetfx35Installed)
	{
		iNetfx35SPLevel = GetNetfxSPLevel(g_szNetfx35RegKeyName, g_szNetfxStandardSPxRegValueName);

		if (iNetfx35SPLevel > 0)
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 3.5 service pack %i is installed."), iNetfx35SPLevel);
		else
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 3.5 is installed with no service packs."));

		_tcscat_s(szOutputString, szMessage);
	}
	else
	{
		_tcscat_s(szOutputString, _T("\n\n.NET Framework 3.5 is not installed."));
	}

	// If .NET Framework 4 Client is installed, get the
	// service pack level
	if (bNetfx40ClientInstalled)
	{
		iNetfx40ClientSPLevel = GetNetfxSPLevel(g_szNetfx40ClientRegKeyName, g_szNetfx40SPxRegValueName);

		if (iNetfx40ClientSPLevel > 0)
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 4 client service pack %i is installed."), iNetfx40ClientSPLevel);
		else
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 4 client is installed with no service packs."));

		_tcscat_s(szOutputString, szMessage);
	}
	else
	{
		_tcscat_s(szOutputString, _T("\n\n.NET Framework 4 client is not installed."));
	}

	// If .NET Framework 4 Full is installed, get the
	// service pack level
	if (bNetfx40FullInstalled)
	{
		iNetfx40FullSPLevel = GetNetfxSPLevel(g_szNetfx40FullRegKeyName, g_szNetfx40SPxRegValueName);

		if (iNetfx40FullSPLevel > 0)
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 4 full service pack %i is installed."), iNetfx40FullSPLevel);
		else
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 4 full is installed with no service packs."));

		_tcscat_s(szOutputString, szMessage);
	}
	else
	{
		_tcscat_s(szOutputString, _T("\n\n.NET Framework 4 full is not installed."));
	}

	// If .NET Framework 4.5 is installed, get the
	// service pack level
	if (bNetfx45Installed)
	{
		iNetfx45SPLevel = GetNetfxSPLevel(g_szNetfx45RegKeyName, g_szNetfx40SPxRegValueName);

		if (iNetfx45SPLevel > 0)
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 4.5 service pack %i is installed."), iNetfx45SPLevel);
		else
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 4.5 is installed with no service packs."));

		_tcscat_s(szOutputString, szMessage);
	}
	else
	{
		_tcscat_s(szOutputString, _T("\n\n.NET Framework 4.5 is not installed."));
	}

	// If .NET Framework 4.5.1 is installed, get the
	// service pack level
	if (bNetfx451Installed)
	{
		iNetfx451SPLevel = GetNetfxSPLevel(g_szNetfx45RegKeyName, g_szNetfx40SPxRegValueName);

		if (iNetfx451SPLevel > 0)
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 4.5.1 service pack %i is installed."), iNetfx451SPLevel);
		else
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 4.5.1 is installed with no service packs."));

		_tcscat_s(szOutputString, szMessage);
	}
	else
	{
		_tcscat_s(szOutputString, _T("\n\n.NET Framework 4.5.1 is not installed."));
	}

	// If .NET Framework 4.5.2 is installed, get the
	// service pack level
	if (bNetfx452Installed)
	{
		iNetfx452SPLevel = GetNetfxSPLevel(g_szNetfx45RegKeyName, g_szNetfx40SPxRegValueName);

		if (iNetfx452SPLevel > 0)
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 4.5.2 service pack %i is installed."), iNetfx452SPLevel);
		else
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 4.5.2 is installed with no service packs."));

		_tcscat_s(szOutputString, szMessage);
	}
	else
	{
		_tcscat_s(szOutputString, _T("\n\n.NET Framework 4.5.2 is not installed."));
	}

	// If .NET Framework 4.6 is installed, get the
	// service pack level
	if (bNetfx46Installed)
	{
		iNetfx46SPLevel = GetNetfxSPLevel(g_szNetfx45RegKeyName, g_szNetfx40SPxRegValueName);

		if (iNetfx46SPLevel > 0)
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 4.6 service pack %i is installed."), iNetfx46SPLevel);
		else
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 4.6 is installed with no service packs."));

		_tcscat_s(szOutputString, szMessage);
	}
	else
	{
		_tcscat_s(szOutputString, _T("\n\n.NET Framework 4.6 is not installed."));
	}

	// If .NET Framework 4.6.1 is installed, get the
	// service pack level
	if (bNetfx461Installed)
	{
		iNetfx461SPLevel = GetNetfxSPLevel(g_szNetfx45RegKeyName, g_szNetfx40SPxRegValueName);

		if (iNetfx461SPLevel > 0)
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 4.6.1 service pack %i is installed."), iNetfx461SPLevel);
		else
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 4.6.1 is installed with no service packs."));

		_tcscat_s(szOutputString, szMessage);
	}
	else
	{
		_tcscat_s(szOutputString, _T("\n\n.NET Framework 4.6.1 is not installed."));
	}

	// If .NET Framework 4.6.2 is installed, get the
	// service pack level
	if (bNetfx462Installed)
	{
		iNetfx462SPLevel = GetNetfxSPLevel(g_szNetfx45RegKeyName, g_szNetfx40SPxRegValueName);

		if (iNetfx462SPLevel > 0)
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 4.6.2 service pack %i is installed."), iNetfx462SPLevel);
		else
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 4.6.2 is installed with no service packs."));

		_tcscat_s(szOutputString, szMessage);
	}
	else
	{
		_tcscat_s(szOutputString, _T("\n\n.NET Framework 4.6.2 is not installed."));
	}

	// If .NET Framework 4.7 is installed, get the
	// service pack level
	if (bNetfx47Installed)
	{
		iNetfx47SPLevel = GetNetfxSPLevel(g_szNetfx45RegKeyName, g_szNetfx40SPxRegValueName);

		if (iNetfx47SPLevel > 0)
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 4.7 service pack %i is installed."), iNetfx47SPLevel);
		else
			_stprintf_s(szMessage, MAX_PATH, _T("\n\n.NET Framework 4.7 is installed with no service packs."));

		_tcscat_s(szOutputString, szMessage);
	}
	else
	{
		_tcscat_s(szOutputString, _T("\n\n.NET Framework 4.7 is not installed."));
	}

	if (setPrint)
	{
		MessageBox(NULL, szOutputString, _T(".NET Framework Install Info"), MB_OK | MB_ICONINFORMATION);
	}

	if (fileOut) {
		wofstream myfile(fName);
		//int iMaxLen = _tcslen(szOutputString);
		if (myfile.is_open())
		{
			wstring fileText(szOutputString);
			//for (int count = 0; count <iMaxLen; count++) {
				//myfile << szOutputString[count] << " ";
				myfile << fileText;
			//}
			myfile.close();
		}
		else {
			MessageBox(NULL, L"Error opening file for output", _T("File Output Error"), MB_OK | MB_ICONINFORMATION);
			return false;
		}
		return true;
	}

	

	return true;
}
