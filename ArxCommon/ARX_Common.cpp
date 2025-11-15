/*
===========================================================================
ARX FATALIS GPL Source Code
Copyright (C) 1999-2010 Arkane Studios SA, a ZeniMax Media company.

This file is part of the Arx Fatalis GPL Source Code ('Arx Fatalis Source Code'). 

Arx Fatalis Source Code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public 
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Arx Fatalis Source Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Arx Fatalis Source Code.  If not, see 
<http://www.gnu.org/licenses/>.

In addition, the Arx Fatalis Source Code is also subject to certain additional terms. You should have received a copy of these 
additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Arx 
Fatalis Source Code. If not, please request a copy in writing from Arkane Studios at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing Arkane Studios, c/o 
ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/
//////////////////////////////////////////////////////////////////////////////////////
//   @@        @@@        @@@                @@                           @@@@@     //
//   @@@       @@@@@@     @@@     @@        @@@@                         @@@  @@@   //
//   @@@       @@@@@@@    @@@    @@@@       @@@@      @@                @@@@        //
//   @@@       @@  @@@@   @@@  @@@@@       @@@@@@     @@@               @@@         //
//  @@@@@      @@  @@@@   @@@ @@@@@        @@@@@@@    @@@            @  @@@         //
//  @@@@@      @@  @@@@  @@@@@@@@         @@@@ @@@    @@@@@         @@ @@@@@@@      //
//  @@ @@@     @@  @@@@  @@@@@@@          @@@  @@@    @@@@@@        @@ @@@@         //
// @@@ @@@    @@@ @@@@   @@@@@            @@@@@@@@@   @@@@@@@      @@@ @@@@         //
// @@@ @@@@   @@@@@@@    @@@@@@           @@@  @@@@   @@@ @@@      @@@ @@@@         //
// @@@@@@@@   @@@@@      @@@@@@@@@@      @@@    @@@   @@@  @@@    @@@  @@@@@        //
// @@@  @@@@  @@@@       @@@  @@@@@@@    @@@    @@@   @@@@  @@@  @@@@  @@@@@        //
//@@@   @@@@  @@@@@      @@@      @@@@@@ @@     @@@   @@@@   @@@@@@@    @@@@@ @@@@@ //
//@@@   @@@@@ @@@@@     @@@@        @@@  @@      @@   @@@@   @@@@@@@    @@@@@@@@@   //
//@@@    @@@@ @@@@@@@   @@@@             @@      @@   @@@@    @@@@@      @@@@@      //
//@@@    @@@@ @@@@@@@   @@@@             @@      @@   @@@@    @@@@@       @@        //
//@@@    @@@  @@@ @@@@@                          @@            @@@                  //
//            @@@ @@@                           @@             @@        STUDIOS    //
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
// ARX_Common
//////////////////////////////////////////////////////////////////////////////////////
//
// Description:
//		Common debugging and logging infrastructure for the entire Arx Fatalis engine.
//		This file provides assertion handling, crash reporting, console logging,
//		and file-based logging capabilities used across all engine modules.
//
//		Key Features:
//		- Singleton debug class (ArxDebug) for centralized logging
//		- Console window for real-time debug output
//		- File logging to timestamped log files
//		- Assertion handling with file/line information
//		- Color-coded console output (warnings, errors, normal logs)
//		- Hierarchical log tagging with indentation
//
// Updates: (date) (person) (update)
//
// Code:	Jean-Yves CORBEL
//			  Xavier RICHTER
//
// Copyright (c) 1999-2010 ARKANE Studios SA. All rights reserved
/////////////////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------------//
// Include Files
//-----------------------------------------------------------------------------------//
#include <ARX_Common.h>      // ArxDebug class definition and debug macros
#include <windows.h>         // Windows API for console, file operations, message boxes
#include <stdio.h>           // Standard I/O for file operations
#include <signal.h>          // Signal handling (not actively used)
//#include <ARX_StackLogger.h> // Stack trace logging (commented out/disabled)
#include <time.h>            // Timestamp generation for log files
#include <fcntl.h>           // File control (not actively used)
#include <io.h>              // Low-level I/O (not actively used)




//-----------------------------------------------------------------------------------//
// Static Member Initialization
//-----------------------------------------------------------------------------------//
// Singleton instance pointer - initialized to NULL, created on first GetInstance() call
ArxDebug		*	ArxDebug::m_pInstance				= NULL	;



//-----------------------------------------------------------------------------------//
// FUNCTION: cpy_wstr
// PURPOSE:  Convert wide character string (wchar_t*) to narrow character string (char*)
//           This is a simple conversion that truncates wide characters to 8-bit chars.
//           Used for converting Windows API wide strings to standard C strings.
//
// PARAMS:   buf - Destination buffer for narrow characters
//           src - Source wide character string
//           max - Maximum number of characters to copy (excluding null terminator)
//
// NOTES:    This function performs a naive conversion by casting each wchar_t to char,
//           which may lose information for non-ASCII characters. It's primarily used
//           for file paths and error messages which are typically ASCII.
//-----------------------------------------------------------------------------------//
void ArxDebug::cpy_wstr(char * buf, const wchar_t * src, size_t max)
{
	if (src)
	{
		// Copy characters one by one, truncating each wchar_t to char
		while (max > 0 && *src != 0)
		{
			*buf++ = (char) src[0];  // Truncate wide char to narrow char
			--max;
			src++;
		}
	}

	// Always null-terminate the destination string
	*buf = 0;
}


//-----------------------------------------------------------------------------------//
// FUNCTION: Assert
// PURPOSE:  Handle assertion failures by gathering debug information and displaying
//           an error message. This is called by the ARX_ASSERT macro when an assertion
//           fails during development/debugging.
//
// PARAMS:   _sMessage - The assertion expression that failed (as wide string)
//           _sFile    - Source file where assertion occurred (as wide string)
//           _iLine    - Line number where assertion occurred
//
// NOTES:    Stack trace functionality is commented out. In the original implementation,
//           this would generate a crash report file with full call stack information.
//           The function formats an error message with program name, file, line, and
//           expression details.
//-----------------------------------------------------------------------------------//
void ArxDebug::Assert(const wchar_t * _sMessage, const wchar_t * _sFile, unsigned int _iLine)
{
	char msgbuf[8192];                              // Buffer for formatted assertion message
	char fn[MAX_PATH + 1];                          // Buffer for executable filename
	char msg[MAX_PATH + 1];                         // Buffer for assertion message
	char iFile[MAX_PATH + 1];                       // Buffer for source filename

	// Convert wide strings to narrow strings for processing
	cpy_wstr(msg, _sMessage, MAX_PATH);
	cpy_wstr(iFile, _sFile, MAX_PATH);

	// Handle empty file name (shouldn't happen, but defensive programming)
	if (iFile[0] == 0)
	{
		strcpy(iFile, "<unknown>");
	}

	// Handle empty assertion message (shouldn't happen, but defensive programming)
	if (msg[0] == 0)
	{
		strcpy(msg, "?");
	}

	// Ensure null termination
	fn[MAX_PATH] = 0;

	// Get the executable filename from Windows
	if (! GetModuleFileNameA(NULL, fn, MAX_PATH))
	{
		strcpy(fn, "<unknown>");
	}

	// Format the complete assertion failure message
	sprintf(msgbuf, "Assertation failed!\n\nProgram: %s\nFile: %s, Line %u\n\nExpression: %s",
	        fn, iFile, _iLine, msg);

	// NOTE: Stack trace and crash file generation disabled in this version
	// Original implementation would:
	// 1. Capture the call stack using ArxStackLogger
	// 2. Create a timestamped crash report in the Log folder
	// 3. Include stack trace, assertion details, and system information
	/*std::string stackTrace ;
	  ArxStackLogger::StackLogger s ;
	  s.GetStackTrace(stackTrace);
	  CreateCrashFile(fn, msg, iFile, _iLine, stackTrace);
	  */
}

//-----------------------------------------------------------------------------------//
// FUNCTION: CreateLogDirectory
// PURPOSE:  Create the "../Log" directory if it doesn't already exist. This directory
//           stores timestamped log files and crash reports.
//
// RETURNS:  true if directory exists or was successfully created, false on error
//
// NOTES:    Uses Windows API CreateDirectoryA to create the folder. If the folder
//           already exists, this is not considered an error. Displays message boxes
//           to the user if critical errors occur (path not found, permission denied).
//-----------------------------------------------------------------------------------//
bool ArxDebug::CreateLogDirectory()
{
	bool bReturn = true ;

	// Path to log directory relative to executable location
	char * sLogReposiriry = "..\\Log";

	// Attempt to create the directory
	if (CreateDirectoryA(sLogReposiriry, NULL) == 0)
	{
		// Creation failed - check why
		DWORD iCodeError = GetLastError();

		switch (iCodeError)
		{
			case ERROR_ALREADY_EXISTS :
				// Directory already exists - this is fine, not an error
				break ;

			case ERROR_PATH_NOT_FOUND :
				// Parent directory doesn't exist - critical error
				bReturn = false ;
				MessageBoxA(NULL, "Folder Path not found", "File ", MB_OK | MB_ICONHAND | MB_SETFOREGROUND | MB_TASKMODAL);
				break ;

			default:
				// Other error (permissions, disk full, etc.)
				bReturn = false ;
				MessageBoxA(NULL, "Folder Log cannot be create. May be you dont have the permission or enought space disk.", "File ", MB_OK | MB_ICONHAND | MB_SETFOREGROUND | MB_TASKMODAL);

		}
	}

	return bReturn ;
}


//-----------------------------------------------------------------------------------//
// FUNCTION: CreateCrashFile (COMMENTED OUT - DISABLED)
// PURPOSE:  Generate a detailed crash report file when an assertion fails. This function
//           would create a timestamped text file containing the crash details and full
//           call stack trace.
//
// PARAMS:   _sFn         - Executable filename
//           _sMsg        - Assertion/error message
//           _sFile       - Source file where crash occurred
//           _iLine       - Line number where crash occurred
//           _sStackTrace - Complete call stack at time of crash
//
// NOTES:    This functionality is currently disabled (commented out). When enabled, it:
//           1. Prompts user with a message box asking if they want to generate a report
//           2. Creates "../Log/CrashReport__<timestamp>.txt" if user agrees
//           3. Writes build date, crash details, and full call stack to the file
//           4. Handles file creation errors with appropriate message boxes
//
//           Crash reports are valuable for post-mortem debugging and bug tracking.
//-----------------------------------------------------------------------------------//
/*
void ArxDebug::CreateCrashFile(const char * _sFn , const char * _sMsg , const char * _sFile , unsigned int _iLine, const std::string & _sStackTrace)
{
	DWORD nCode;

	// Format the crash information for display
	std::ostringstream oss ;
	oss << "Executable : " << _sFn << "\nFile : " << _sFile << "	Line : " << _iLine << "\nCause : " << _sMsg;
	std::string _sFileOss = oss.str().c_str();

	// Ask user if they want to generate a crash report
	oss << "\n Do you want to generate a report into your Log folder ? ";

	nCode = MessageBoxA(NULL, oss.str().c_str(), "Arx Runtime Assertation ", MB_YESNO |
	                    MB_ICONHAND | MB_SETFOREGROUND | MB_TASKMODAL);

	// If user clicked "Yes" and log directory was created successfully
	if ((nCode == IDYES) && CreateLogDirectory())
	{
		// Open an output file stream for the crash report
		std::ofstream fsFile ;
		std::ostringstream ossFileName ;
		// Generate timestamped filename: CrashReport__<unix_timestamp>.txt
		ossFileName << "..\\Log\\CrashReport__" << time(NULL) << ".txt";

		fsFile.open(ossFileName.str().c_str(), std::ios::out | std::ios::trunc);

		if (!fsFile)
		{
			// File creation failed - notify user
			MessageBoxA(NULL, "Crash report cannot be create. May be you don't have the permission or enough space disk.", "Crash Report ", MB_OK | MB_ICONHAND | MB_SETFOREGROUND | MB_TASKMODAL);
			return ;
		}

		// Write crash report contents:
		// - Build date
		// - Crash details (executable, file, line, cause)
		// - Complete call stack trace
		fsFile << __DATE__ << "\n\n" << _sFileOss << "\n\n ------------------------------------------ CallStack Log --------------------------------- \n\n" << _sStackTrace;

		fsFile.close();
	}
}
*/

//-----------------------------------------------------------------------------------//
// SINGLETON PATTERN IMPLEMENTATION
//-----------------------------------------------------------------------------------//

//-----------------------------------------------------------------------------------//
// FUNCTION: GetInstance (static)
// PURPOSE:  Get the singleton instance of ArxDebug. Creates the instance on first call.
//           This ensures only one debug/logging system exists throughout the application.
//
// PARAMS:   _bLogIntoFile - Whether to enable file logging (default: true)
//
// RETURNS:  Pointer to the singleton ArxDebug instance
//
// NOTES:    Thread-safety is not implemented - this assumes single-threaded access
//           or access during initialization only.
//-----------------------------------------------------------------------------------//
ArxDebug * ArxDebug::GetInstance(bool _bLogIntoFile /*= true*/)
{
	if (!m_pInstance)
	{
		// Create the singleton instance if it doesn't exist yet
		m_pInstance = new ArxDebug(_bLogIntoFile);
	}

	return m_pInstance;
}

//-----------------------------------------------------------------------------------//
// FUNCTION: CleanInstance (static)
// PURPOSE:  Destroy the singleton instance and free its memory. Should be called
//           during application shutdown.
//
// NOTES:    After calling this, GetInstance() will create a new instance if called again.
//-----------------------------------------------------------------------------------//
void ArxDebug::CleanInstance()
{
	if (m_pInstance)
	{
		delete m_pInstance ;  // Destructor will clean up console and log file
	}
}

//-----------------------------------------------------------------------------------//
// CONSTRUCTOR & DESTRUCTOR
//-----------------------------------------------------------------------------------//

//-----------------------------------------------------------------------------------//
// CONSTRUCTOR: ArxDebug
// PURPOSE:     Initialize the debug system: create console window and optionally
//              start file logging.
//
// PARAMS:      _bLogIntoFile - If true, create a timestamped log file in ../Log/
//
// NOTES:       This is private (singleton pattern) - use GetInstance() instead.
//-----------------------------------------------------------------------------------//
ArxDebug::ArxDebug(bool _bLogIntoFile /*= true*/)
{
	// Initialize member variables
	m_bConsoleInitialize	= false ;  // Will be set to true if console creation succeeds
	m_bOpenLogFile			= false ;  // Will be set to true if log file opens successfully
	m_uiTabulation			= 0		;  // Indentation level for hierarchical logging

	// Create debug console window and redirect stdout/stderr to it
	RedirectIOToConsole();

	// Optionally create timestamped log file
	if (_bLogIntoFile)
	{
		StartLogSession();
	}
}

//-----------------------------------------------------------------------------------//
// DESTRUCTOR: ~ArxDebug
// PURPOSE:    Clean up debug system: free console, close log file, reset singleton pointer
//-----------------------------------------------------------------------------------//
ArxDebug::~ArxDebug()
{
	CleanConsole();     // Free the debug console if it was created
	EndLogSession();    // Close the log file if it was opened

	m_pInstance = NULL ;    // Clear singleton pointer
	m_uiTabulation = 0	;   // Reset indentation level
}


//-----------------------------------------------------------------------------------//
// CONSOLE CREATION AND MANAGEMENT
//-----------------------------------------------------------------------------------//

//-----------------------------------------------------------------------------------//
// FUNCTION: RedirectIOToConsole
// PURPOSE:  Create a debug console window and redirect standard I/O streams to it.
//           This allows printf, cout, cerr, etc. to output to the debug console.
//
// NOTES:    Uses Windows-specific console APIs. The console is separate from the
//           main game window, useful for debugging while the game runs.
//           If console creation fails, standard I/O simply won't be visible.
//-----------------------------------------------------------------------------------//
void ArxDebug::RedirectIOToConsole()
{
	int hConHandle;   // Not used in current implementation
	long lStdHandle;  // Not used in current implementation
	FILE * fp;        // Not used in current implementation

	// Create the debug console window
	if ((m_bConsoleInitialize = CreateDebugConsole()))
	{
		// Redirect standard output streams to the new console
		freopen("CONOUT$", "wt", stdout);  // Redirect stdout (printf, cout)
		freopen("CONERR$", "wt", stderr);  // Redirect stderr (cerr)
		freopen("CONIN$",  "r",  stdin) ;  // Redirect stdin (cin, scanf) - rarely used
	}

}

//-----------------------------------------------------------------------------------//
// FUNCTION: CreateDebugConsole
// PURPOSE:  Allocate a Windows console window for the application and configure
//           its buffer size for scrolling.
//
// RETURNS:  true if console was successfully created, false on error
//
// NOTES:    The console is titled "Arx Fatalis Debug Window" and has a large
//           scrollback buffer (defined by ARXCOMMON_MAX_CONSOLE_ROWS/LINES).
//           If AllocConsole() fails, it's typically because a console already exists.
//-----------------------------------------------------------------------------------//
bool ArxDebug::CreateDebugConsole()
{
	CONSOLE_SCREEN_BUFFER_INFO coninfo;

	// Allocate a new console for this application
	// This fails if a console already exists (not an error for us)
	if (!AllocConsole())
	{
		MessageBoxA(NULL, "Cannot create Log Console ", "Log Console Error ", MB_OK | MB_ICONHAND | MB_SETFOREGROUND | MB_TASKMODAL);
		return false ;
	}

	// Set a descriptive title for the console window
	SetConsoleTitleA("Arx Fatalis Debug Window");

	// Increase the screen buffer size to allow scrolling through lots of log messages
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.X = ARXCOMMON_MAX_CONSOLE_ROWS ;   // Width in characters
	coninfo.dwSize.Y = ARXCOMMON_MAX_CONSOLE_LINES;   // Height in lines (scrollback)
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);
	return true ;
}

//-----------------------------------------------------------------------------------//
// FUNCTION: CleanConsole
// PURPOSE:  Free the debug console window if it was created.
//
// NOTES:    Called automatically in the destructor. Safe to call even if console
//           wasn't successfully created (checked via m_bConsoleInitialize flag).
//-----------------------------------------------------------------------------------//
void ArxDebug::CleanConsole()
{
	if (m_bConsoleInitialize)
	{
		FreeConsole();  // Windows API - frees the console allocated by AllocConsole()
	}
}


//-----------------------------------------------------------------------------------//
// FILE LOGGING FUNCTIONS
//-----------------------------------------------------------------------------------//

//-----------------------------------------------------------------------------------//
// FUNCTION: StartLogSession
// PURPOSE:  Create and open a timestamped log file for writing. All subsequent log
//           messages will be written to both the console and this file.
//
// NOTES:    Log file is created at: "../Log/Log__<timestamp>.txt"
//           The timestamp is Unix time (seconds since Jan 1, 1970)
//           File is opened in append mode, so multiple runs can share a file if
//           they have the same timestamp (unlikely).
//           If file creation fails, logging continues to console only.
//-----------------------------------------------------------------------------------//
void ArxDebug::StartLogSession()
{
	// Only create log file if directory creation succeeds
	if (CreateLogDirectory())
	{
		// Generate unique filename using Unix timestamp
		unsigned int uiID = static_cast<unsigned int>(time(NULL));

		std::ostringstream ossFileName ;
		ossFileName << "..\\Log\\Log__" << uiID << ".txt";

		// Open file in append mode (ios::app allows multiple sessions to share file)
		m_fsFile.open(ossFileName.str().c_str(), std::ios::out | std::ios::app);

		if (m_fsFile)
		{
			m_bOpenLogFile = true ;  // File opened successfully
		}
		else
		{
			// File creation failed - log to console only
			m_bOpenLogFile = false ;
			MessageBoxA(NULL, "Log file cannot be create. May be you dont have the permission or enought space disk.", "Log File ", MB_OK | MB_ICONHAND | MB_SETFOREGROUND | MB_TASKMODAL);
		}
	}
}



//-----------------------------------------------------------------------------------//
// FUNCTION: LogTypeManager
// PURPOSE:  Set console text color and add timestamped prefix based on log type
//           (normal log, warning, or error). This provides visual distinction in
//           the debug console.
//
// PARAMS:   eType - Type of log message (eLog, eLogWarning, eLogError)
//
// NOTES:    Uses Windows console color API to set text attributes:
//           - Normal logs: Default color (typically gray)
//           - Warnings: Warning color (typically yellow)
//           - Errors: Error color (typically red)
//
//           Timestamp format: [Type : HHh MMm SSs] :
//           Example: [Warning : 14h 32m 05s] : Player health low
//
//           The m_ossBuffer member is cleared and populated with the prefix.
//-----------------------------------------------------------------------------------//
void ArxDebug::LogTypeManager(ARX_DEBUG_LOG_TYPE eType)
{
	// Get handle to console output for setting text color
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	// Get current local time for timestamp
	time_t timestamp;
	struct tm * t;
	timestamp = time(NULL);
	t = localtime(&timestamp);

	// Set color and create timestamped prefix based on log type
	switch (eType)
	{
		case eLogWarning :
			SetConsoleTextAttribute(hStdOut, ARXDEBUG_COLOR_WARNING);
			m_ossBuffer << "[Warning : " << t->tm_hour << "h " << t->tm_min << "m " << t->tm_sec << "s] : ";
			break;
		case eLogError :
			SetConsoleTextAttribute(hStdOut, ARXDEBUG_COLOR_ERROR);
			m_ossBuffer << "[Error : " << t->tm_hour << "h " << t->tm_min << "m " << t->tm_sec << "s] : ";
			break;
		case eLog :
		default:
			SetConsoleTextAttribute(hStdOut, ARXDEBUG_COLOR_DEFAULT);
			m_ossBuffer << "[Log : " << t->tm_hour << "h " << t->tm_min << "m " << t->tm_sec << "s] : ";
	}
}


//-----------------------------------------------------------------------------------//
// FUNCTION: AddTabulation
// PURPOSE:  Add indentation (tabs) to the output buffer based on current nesting level.
//           This creates a hierarchical visual structure for logs within tagged sections.
//
// PARAMS:   _ossBuffer - Output string stream to add tabs to
//
// NOTES:    The number of tabs is determined by m_uiTabulation member variable.
//           Use OpenTag() to increase indentation, CloseTag() to decrease it.
//           Makes it easy to see log message hierarchy and nesting depth.
//-----------------------------------------------------------------------------------//
void ArxDebug::AddTabulation(std::ostringstream & _ossBuffer)
{
	// Add one tab character for each level of nesting
	for (unsigned int i = 0 ; i < m_uiTabulation ; ++i)
	{
		_ossBuffer << "\t";
	}
}


//-----------------------------------------------------------------------------------//
// FUNCTION: Log (variadic)
// PURPOSE:  Main logging function. Writes a formatted message to both the debug console
//           and the log file (if enabled). Supports printf-style formatting.
//
// PARAMS:   eType     - Log message type (eLog, eLogWarning, eLogError)
//           _sMessage - Format string (printf-style)
//           ...       - Variable arguments for format string
//
// USAGE:    ArxDebug::GetInstance()->Log(eLog, "Player position: (%f, %f, %f)", x, y, z);
//           ArxDebug::GetInstance()->Log(eLogWarning, "Low memory: %d MB remaining", mem);
//           ArxDebug::GetInstance()->Log(eLogError, "Failed to load texture: %s", filename);
//
// NOTES:    Messages are:
//           1. Formatted with vsnprintf (handles variable arguments)
//           2. Prefixed with timestamp and type by LogTypeManager()
//           3. Indented based on current tag nesting level
//           4. Written to log file (if open) with immediate flush
//           5. Written to console (if initialized) with immediate flush
//
//           Immediate flushing ensures logs are preserved even if the app crashes.
//-----------------------------------------------------------------------------------//
void ArxDebug::Log(ARX_DEBUG_LOG_TYPE eType, const char * _sMessage, ...)
{
	// Buffer to hold the formatted message
	char sBuffer[ARXCOMMON_BUFFERSIZE];

	// Process variable arguments using standard va_list mechanism
	va_list arg_ptr ;
	va_start(arg_ptr, _sMessage);
	_vsnprintf(sBuffer, ARXCOMMON_BUFFERSIZE, _sMessage, arg_ptr);
	va_end(arg_ptr) ;

	// Add timestamp, type prefix, and set console color
	LogTypeManager(eType);

	// Add indentation based on tag nesting level
	AddTabulation(m_ossBuffer);

	// Append the actual message and newline
	m_ossBuffer << sBuffer << "\n";

	// Write to log file if it's open
	if (m_bOpenLogFile)
	{
		m_fsFile << m_ossBuffer.str().c_str();
		m_fsFile.flush();  // Immediate flush - ensures log survives crashes
	}

	// Write to debug console if it exists
	if (m_bConsoleInitialize)
	{
		std::cout << m_ossBuffer.str().c_str();
		std::cout.flush();  // Immediate flush - ensures real-time display
	}
}


//-----------------------------------------------------------------------------------//
// FUNCTION: EndLogSession
// PURPOSE:  Close the log file stream if it's open. Called automatically in destructor,
//           but can also be called manually to end logging before shutdown.
//
// NOTES:    After calling this, log messages will only appear in the console, not the file.
//           Sets m_bOpenLogFile flag to false to prevent writing to closed stream.
//-----------------------------------------------------------------------------------//
void ArxDebug::EndLogSession()
{
	if (m_bOpenLogFile)
	{
		m_fsFile.close();
		m_bOpenLogFile = false ;
	}
}




//-----------------------------------------------------------------------------------//
// HIERARCHICAL LOGGING FUNCTIONS
//-----------------------------------------------------------------------------------//

//-----------------------------------------------------------------------------------//
// FUNCTION: OpenTag
// PURPOSE:  Start a new hierarchical section in the log with a descriptive label.
//           All subsequent log messages will be indented until CloseTag() is called.
//
// PARAMS:   _sTag - Descriptive name for this log section
//
// USAGE:    OpenTag("Texture Loading");
//             Log(eLog, "Loading texture: player.tga");
//             Log(eLog, "Texture size: 512x512");
//           CloseTag();
//
// NOTES:    Creates a visual separator with the tag name:
//           [Log : <build_time>] :
//               ---------------- Texture Loading ----------------
//
//           Increases indentation level (m_uiTabulation) so all subsequent logs
//           are indented beneath this tag. Tags can be nested for hierarchical logging.
//
//           Example output:
//           [Log : 14:32:05] :
//           	 ---------------- Game Init ----------------
//           [Log : 14:32:05] : 		Starting DirectX...
//           [Log : 14:32:05] : 			Loading shaders...
//-----------------------------------------------------------------------------------//
void ArxDebug::OpenTag(const char * _sTag)
{
	// Increase indentation level for all logs within this tag
	++m_uiTabulation;

	// Create tag header with build time
	m_ossBuffer << "[Log : " << __TIME__ << "] :";
	AddTabulation(m_ossBuffer);
	m_ossBuffer << " ---------------- " << _sTag << " ----------------" << "\n";

	// Write tag to log file
	if (m_bOpenLogFile)
	{
		m_fsFile << m_ossBuffer.str().c_str();
		m_fsFile.flush();
	}

	// Write tag to console
	if (m_bConsoleInitialize)
	{
		std::cout << m_ossBuffer.str().c_str();
	}
}


//-----------------------------------------------------------------------------------//
// FUNCTION: CloseTag
// PURPOSE:  End the current hierarchical log section and decrease indentation level.
//
// NOTES:    Decrements m_uiTabulation to reduce indentation for subsequent logs.
//           Safe to call even if no tag is open (uses ternary to prevent underflow).
//           Should be paired with OpenTag() calls, but doesn't enforce strict pairing.
//-----------------------------------------------------------------------------------//
void ArxDebug::CloseTag()
{
	// Decrease indentation, but don't go below zero
	(m_uiTabulation == 0) ? m_uiTabulation : --m_uiTabulation;
}


//-----------------------------------------------------------------------------------//
// End of ARX_Common.cpp
//-----------------------------------------------------------------------------------//





