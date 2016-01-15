#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "asiosys.h"
#include "asio.h"
#include "asiodrivers.h"
#include "asiolist.h"
#include <windows.h>
#include "myTools.h"			//self made library
#include "signal_analysis.h"	//self made library
#include "myWav.h"
#include <iostream>
#include <vector>
///////////////macros////////////////
#define NUM 1024
#define HALFNUM 512
#define TWICENUM 2048 
#define TwoPow31Minus1 2147483647
#define TwoPi 6.28318531

//////////asio variables////////////////
AsioDriverList * asioDrvList = 0; //object of asiodriverlist
LPASIODRVSTRUCT DrvList;    // link-list of the compatable drivers with information 
int nleftcount = 0, nrightcount = 0;
short spLeftBuffer[TWICENUM];
short spRightBuffer[TWICENUM];
float *Gkernel, prevfrequency=0, currfrequency;

///////////////brushes for rectangles and color pens for colored lines/////////////////////
HBRUSH hbrushGrey = CreateSolidBrush(RGB(20, 20, 20));
HBRUSH hbrushbackground = CreateSolidBrush(RGB(0, 0, 0));			//160,255,255
HPEN hpenred = CreatePen(PS_SOLID, 0, RGB(100, 0, 0));
HPEN hpenyellow = CreatePen(PS_SOLID, 0, RGB(255, 255, 0));
HPEN hpenblue = CreatePen(PS_SOLID, 0, RGB(0, 0, 100));
COLORREF textcolor = RGB(150, 150, 150);

FILE *fp = wav_write("test1.wav", 44100, 2, 16, 60000);

SIZE sStringScreenSize;
LRESULT CALLBACK WndProc(
	HWND hwnd,
	UINT Message,
	WPARAM wParam,
	LPARAM lParam
	);

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow
	);


char  ASIO_DRIVER_NAME[32];
int cur = 0;

enum
{
	// number of input and outputs supported by the host application
	// you can change these to higher or lower values
	kMaxInputChannels = 2,
	kMaxOutputChannels = 2
};


// internal data storage
typedef struct DriverInfo
{
	// ASIOInit()
	ASIODriverInfo driverInfo;

	// ASIOGetChannels()
	long           inputChannels;
	long           outputChannels;

	// ASIOGetBufferSize()
	long           minSize;
	long           maxSize;
	long           preferredSize;
	long           granularity;

	// ASIOGetSampleRate()
	ASIOSampleRate sampleRate;

	// ASIOOutputReady()
	bool           postOutput;

	// ASIOGetLatencies ()
	long           inputLatency;
	long           outputLatency;

	// ASIOCreateBuffers ()
	long inputBuffers;	// becomes number of actual created input buffers
	long outputBuffers;	// becomes number of actual created output buffers
	ASIOBufferInfo bufferInfos[kMaxInputChannels + kMaxOutputChannels]; // buffer info's

	// ASIOGetChannelInfo()
	ASIOChannelInfo channelInfos[kMaxInputChannels + kMaxOutputChannels]; // channel info's
	// The above two arrays share the same indexing, 
	// as the data in them are linked together

	// Information from ASIOGetSamplePosition()
	// data is converted to double floats for easier use, 
	// however 64 bit integer can be used, too
	double         nanoSeconds;
	double         samples;
	double         tcSamples;	// time code samples

	// bufferSwitchTimeInfo()
	ASIOTime       tInfo;	// time info state
	unsigned long  sysRefTime;// system reference time, when bufferSwitch() was called

	// Signal the end of processing in this example
	bool           stopped;
} DriverInfo;


DriverInfo asioDriverInfo = { 0 };
ASIOCallbacks asioCallbacks;

//----------------------------------------------------------------------------------
// some external references
extern AsioDrivers* asioDrivers;
bool loadAsioDriver(char *name);

long init_asio_static_data(DriverInfo *asioDriverInfo);
ASIOError create_asio_buffers(DriverInfo *asioDriverInfo);
unsigned long get_sys_reference_time();

// callback prototypes
void bufferSwitch(long index, ASIOBool processNow);
ASIOTime *bufferSwitchTimeInfo(ASIOTime *timeInfo, long index, ASIOBool processNow);
void sampleRateChanged(ASIOSampleRate sRate);
long asioMessages(long selector, long value, void* message, double* opt);

void respond_keystroke(UINT Message, WPARAM wParam, LPARAM lParam);
void load_buffer(int *pnBuffer, int nLeftOrRight);
void store_left_buffer(int *);
void store_right_buffer(int *);
void reset_detection();

void first_input(UINT message,
	WPARAM wParam,
	LPARAM lParam);
void highlight(int cur);
void last_action();
HWND hwnd;
HDC hDC;
void(*pfnRespond)(
	UINT message,
	WPARAM wParam,
	LPARAM lParam);
void(*pfnDraw)();
int  uWidth = GetSystemMetrics(SM_CXFULLSCREEN);
int  uHeight = GetSystemMetrics(SM_CYFULLSCREEN);
/////////////some rectangles//////////////////////
RECT signalrectL = { uWidth / 20 - 2, uHeight / 20 - 2, (19 * uWidth) / 20 + 2, uHeight / 2.0 - uHeight / 20.0 + 2 };
RECT signalrectR = { uWidth / 20 - 2, uHeight / 2.0 + uHeight / 20.0 - 2, (19 * uWidth) / 20 + 2, uHeight - uHeight / 20 + 2 };
RECT freqRect = { (uWidth * 5) / 6, uHeight / 2+20, uWidth, uHeight / 2 - 5 };
// this is a half screen black rectangle on which the sound signal is displayed
RECT creativerect = { 2*uWidth/3, uHeight / 2 - uHeight/20.0 + 2, uWidth, uHeight / 2.0 + uHeight / 20.0 - 2 };
// to clear the creative area i.e. lower half screen whenever mode is changed

// This is where all the input to the window goes to
LRESULT CALLBACK WndProc(
	HWND hwnd,
	UINT Message,
	WPARAM wParam,
	LPARAM lParam
	)
{

	switch (Message)
	{
	case WM_CREATE:
		// this creates a full-screen window
		ShowWindow(hwnd, SW_MAXIMIZE);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		pfnRespond(Message, wParam, lParam);
		break;
	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}



void second_output()
{
	RECT rectCorner = { 0, 0, uWidth*1.2, uHeight*1.3 };
	FillRect(hDC, &rectCorner, hbrushbackground);
	FillRect(hDC, &signalrectL, hbrushGrey);
	FillRect(hDC, &signalrectR, hbrushGrey);

	//SelectObject(hDC, hpenyellow);
	SelectObject(hDC, hpenred);
	MoveToEx(hDC, uWidth / 20.0 - 1, uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 - 1 / 2.0), NULL);
	LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 - 1 / 2.0));
	MoveToEx(hDC, uWidth / 20.0 - 1, uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 + 1 / 2.0), NULL);
	LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 + 1 / 2.0));
	MoveToEx(hDC, uWidth / 20.0 - 1, uHeight/2 + uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 - 1 / 2.0), NULL);
	LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 2 + uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 - 1 / 2.0));
	MoveToEx(hDC, uWidth / 20.0 - 1, uHeight / 2 + uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 + 1 / 2.0), NULL);
	LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 2 + uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 + 1 / 2.0));
	SelectObject(hDC, hpenblue);
	MoveToEx(hDC, uWidth / 20.0 - 1, uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 - 3 / 4.0), NULL);
	LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 - 3 / 4.0));
	MoveToEx(hDC, uWidth / 20.0 - 1, uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 + 3 / 4.0), NULL);
	LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 + 3 / 4.0));
	MoveToEx(hDC, uWidth / 20.0 - 1, uHeight / 4, NULL);
	LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 4);
	MoveToEx(hDC, uWidth / 20.0 - 1, uHeight / 2 + uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 - 3 / 4.0), NULL);
	LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 2 + uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 - 3 / 4.0));
	MoveToEx(hDC, uWidth / 20.0 - 1, uHeight / 2 + uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 + 3 / 4.0), NULL);
	LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 2 + uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 + 3 / 4.0));
	MoveToEx(hDC, uWidth / 20.0 - 1, uHeight / 2 + uHeight / 4, NULL);
	LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 2 + uHeight / 4);
	//setting the upper signal displaying rectangle and 5 lines in it

	
//	SetBkColor(hDC, RGB(128, 128, 128));
	SetTextColor(hDC, textcolor);
	SetBkMode(hDC, TRANSPARENT);			
	TextOut(hDC, uWidth / 3, uHeight/2-7
		, "Press SPACE to start or stop", 28);
	TextOut(hDC, uWidth / 3, uHeight/2+7
		, "Press Esc to exit", 20);

}
void last_action()
{
	if (!asioDriverInfo.stopped)
		ASIOStop();

	ASIODisposeBuffers();

	ASIOExit();
	asioDrivers->removeCurrentDriver();
}

RECT rectRecording = { 2 * uWidth / 3, uHeight - 15, uWidth, uHeight };


void second_input(UINT Message, WPARAM wParam, LPARAM lParam)
{
	int i = 0,j=0;
	SetBkMode(hDC, TRANSPARENT);
	char * reponse_str;
	SetTextColor(hDC, textcolor);
	if (Message == WM_KEYDOWN && wParam == VK_SPACE)
	{
		FillRect(hDC, &rectRecording, hbrushbackground);
		// if it's stopped start it, else stop it
		if (asioDriverInfo.stopped)
		{
			if (ASIOStart() == ASE_OK)
			{
				// Now all is up and running
				reponse_str = "Reading Started!";
				FillRect(hDC, &creativerect, hbrushbackground);
				TextOut(hDC, 2*uWidth / 3, uHeight/2-5
					, reponse_str, strlen(reponse_str));
				asioDriverInfo.stopped = false;
			}
		}
		else
		{
			// stop ASIO
			if (ASIOStop() == ASE_OK)
			{
				// Now it's stopped
				reponse_str = "Reading stopped.";
				FillRect(hDC, &creativerect, hbrushbackground);
				TextOut(hDC, 2*uWidth/3, uHeight/2-5
					, reponse_str, strlen(reponse_str));
				asioDriverInfo.stopped = true;
			}
		}
	}
	else if (Message == WM_KEYDOWN && (wParam == (char)27)) // the "X" key
	{
		last_action();
		PostQuitMessage(0);
	}
}

void second_action()
{
	// load ASIO driver
	// Init ASIO, (allocate the buffers)
	// load the driver, this will setup all the necessary internal data structures
	char str[300];
	if (loadAsioDriver(ASIO_DRIVER_NAME))
	{
		// initialize the driver
		if (ASIOInit(&asioDriverInfo.driverInfo) == ASE_OK)
		{
			
			sprintf(str,
				"asioVersion:   %d\n"
				"driverVersion: %d\n"
				"Name:          %s\n"
				"ErrorMessage:  %s\n",
				asioDriverInfo.driverInfo.asioVersion,
				asioDriverInfo.driverInfo.driverVersion,
				asioDriverInfo.driverInfo.name,
				asioDriverInfo.driverInfo.errorMessage
				);
			OutputDebugString(str);
			if (init_asio_static_data(&asioDriverInfo) == 0)
			{
				// set up the asioCallback structure and create the ASIO data buffer
				asioCallbacks.bufferSwitch = &bufferSwitch;
				asioCallbacks.sampleRateDidChange = &sampleRateChanged;
				asioCallbacks.asioMessage = &asioMessages;
				asioCallbacks.bufferSwitchTimeInfo = &bufferSwitchTimeInfo;
				if (create_asio_buffers(&asioDriverInfo) == ASE_OK)
					printf("asio loaded successfully");
			}
		}
	}
	second_output();
	pfnRespond = second_input;
}
void first_output()
{

	int i;
	RECT rectCorner = { 0, 0, uWidth, uHeight*1.2 };
	FillRect(hDC, &rectCorner, hbrushbackground);
	//	SetBkColor(hDC, RGB(128, 128, 128));
	SetBkMode(hDC, TRANSPARENT);
	SetTextColor(hDC, textcolor);

	char * entryMsg = "Press SPACE to select";
	char *exitMsg = "Press Esc to exit";

	// if no driver then print the errorMsg

	if (DrvList == NULL)
	{
		// print the following message to the screen

		char  *errorMsg[4];
		errorMsg[0] = "Make sure you've installed ASIO4ALL v2.";
		errorMsg[1] = "If you have an external sound interface,";
		errorMsg[2] = "make sure it's connected and turned on,";
		errorMsg[3] = "and that you've loaded its ASIO driver";
		for (i = 0; i<4; i++)
		{
			GetTextExtentPoint(hDC, errorMsg[i], strlen(errorMsg[i]), &sStringScreenSize);
			TextOut(hDC, uWidth / 3, (uHeight - sStringScreenSize.cy * 4) / 2 + i*sStringScreenSize.cy
				, errorMsg[i], strlen(errorMsg[i]));
		}
		Sleep(5000);
		// add pause statement
		exit(0);

	}
	// Display all the drivers on the screen
	//  function available in object asioDrvList can be seen in asiolist.h
	else
	{
		char  *drvName;
		drvName = DrvList->drvname;

		GetTextExtentPoint(hDC, drvName, strlen(drvName), &sStringScreenSize);

		TextOut(hDC, uWidth / 3, (uHeight - sStringScreenSize.cy*asioDrvList->asioGetNumDev()) / 2 - 2 * sStringScreenSize.cy
			, entryMsg, strlen(entryMsg));

		for (i = 0; i<asioDrvList->asioGetNumDev(); i++)
		{
			drvName = DrvList->drvname;
			TextOut(hDC, uWidth / 3, (uHeight - (sStringScreenSize.cy * asioDrvList->asioGetNumDev())) / 2 + i*sStringScreenSize.cy
				, drvName, strlen(drvName));
			DrvList = DrvList->next;
		}
		highlight(0);
		TextOut(hDC, uWidth / 3, (uHeight - sStringScreenSize.cy*asioDrvList->asioGetNumDev()) / 2 + (i + 5)*sStringScreenSize.cy
			, exitMsg, strlen(exitMsg));
	}


}

void highlight(int cur){

	int uCornerX = uWidth / 3;
	int uCornerY = (uHeight - (sStringScreenSize.cy * asioDrvList->asioGetNumDev())) / 2 + cur*sStringScreenSize.cy;
	RECT rectCorner = { uCornerX, uCornerY, uCornerX + 200, uCornerY + sStringScreenSize.cy };
	//unhighlight previous and highlight current(cur) driver index
	DrawFocusRect(hDC, &rectCorner);
}


void first_input(UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	// note cur moves in circular order acc to key up or down
	// when enter is pressed asio driver name is assigned a value acc to cur
	switch (message)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_UP:
			highlight(cur);
			cur = (cur - 1 + asioDrvList->asioGetNumDev()) % asioDrvList->asioGetNumDev();
			highlight(cur);
			break;

		case VK_DOWN:
			highlight(cur);
			cur = (cur + 1) % asioDrvList->asioGetNumDev();
			highlight(cur);
			break;

		case VK_SPACE:-
			asioDrvList->asioGetDriverName(0, ASIO_DRIVER_NAME, 32);
			second_action();
			break;

		case  (char)27:
			PostQuitMessage(0);
			break;
		}
	}

}

void first_action()
{
	// Created an object of class AsioDriverList

	asioDrvList = new AsioDriverList();

	// DrvList now contains the entire linklist of asiodrivers installed in system
	DrvList = asioDrvList->lpdrvlist;
	first_output();

}


// The 'main' function of Win32 GUI programs: this is where execution starts
int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow
	)
{
	WNDCLASSEX wc; // A properties struct of our window 

	MSG Msg; // A temporary location for all messages 

	// zero out the struct and set the stuff we want to modify 
	memset(&wc, 0, sizeof(wc));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc; // This is where we will send messages to 
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = "WindowClass";
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION); // Load a standard icon 
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	if (!RegisterClassEx(&wc))
	{
		MessageBox(
			NULL,
			"Window Registration Failed!",
			"Error!",
			MB_ICONEXCLAMATION | MB_OK
			);
		return 0;
	}
	hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		"WindowClass",
		"Caption",
		WS_POPUP | WS_MAXIMIZE | WS_SYSMENU, // these flags are a for full-screen
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL
		);

	if (hwnd == NULL)
	{
		MessageBox(
			NULL,
			"Window Creation Failed!",
			"Error!",
			MB_ICONEXCLAMATION | MB_OK
			);
		return 0;
	}

	// for selection of asio driver  
	hDC = GetDC(hwnd);
	Gkernel = get_gaussian_filter(7, 1.0);
	first_action();
	pfnRespond = first_input;

	while (GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg); // Translate keycodes to chars if present 
		DispatchMessage(&Msg); // Send it to WndProc 
	}

	return Msg.wParam;
}

//----------------------------------------------------------------------------------
long init_asio_static_data(DriverInfo *asioDriverInfo)
{
	char str[300];
	asioDriverInfo->stopped = true;
	// collect the informational data of the driver
	// get the number of available channels
	if (ASIOGetChannels(&asioDriverInfo->inputChannels, &asioDriverInfo->outputChannels) == ASE_OK)
	{
		sprintf(str,
			"ASIOGetChannels (inputs: %d, outputs: %d);\n",
			asioDriverInfo->inputChannels,
			asioDriverInfo->outputChannels
			);
		OutputDebugString(str);
		// get the usable buffer sizes
		if (ASIOGetBufferSize(&asioDriverInfo->minSize, &asioDriverInfo->maxSize, &asioDriverInfo->preferredSize, &asioDriverInfo->granularity) == ASE_OK)
		{
			sprintf(str,
				"ASIOGetBufferSize (min: %d, max: %d, preferred: %d, granularity: %d);\n",
				asioDriverInfo->minSize, asioDriverInfo->maxSize,
				asioDriverInfo->preferredSize,
				asioDriverInfo->granularity
				);
			OutputDebugString(str);
			// get the currently selected sample rate
			if (ASIOGetSampleRate(&asioDriverInfo->sampleRate) == ASE_OK)
			{
				sprintf(str,"ASIOGetSampleRate (sampleRate: %f);\n", asioDriverInfo->sampleRate);
				OutputDebugString(str);
				if (asioDriverInfo->sampleRate <= 0.0 || asioDriverInfo->sampleRate > 96000.0)
				{
					// Driver does not store it's internal sample rate, 
					// so set it to a know one.
					// Usually you should check beforehand, 
					// that the selected sample rate is valid
					// with ASIOCanSampleRate().
					if (ASIOSetSampleRate(44100.0) == ASE_OK)
					{
						if (ASIOGetSampleRate(&asioDriverInfo->sampleRate) == ASE_OK) {
							sprintf(str, "ASIOGetSampleRate (sampleRate: %f);\n", asioDriverInfo->sampleRate);
							OutputDebugString(str);
						}else
							return -6;
					}
					else
						return -5;
				}
				// check wether the driver requires the ASIOOutputReady() optimization
				// (can be used by the driver to reduce output latency by one block)
				if (ASIOOutputReady() == ASE_OK)
					asioDriverInfo->postOutput = true;
				else
					asioDriverInfo->postOutput = false;
				sprintf(str,"ASIOOutputReady(); - %s\n", asioDriverInfo->postOutput ? "Supported" : "Not supported");
				OutputDebugString(str);
				return 0;
			}
			return -3;
		}
		return -2;
	}
	return -1;
}


//----------------------------------------------------------------------------------
// conversion from 64 bit ASIOSample/ASIOTimeStamp to double float
#if NATIVE_INT64
#define ASIO64toDouble(a)  (a)
#else
const double twoRaisedTo32 = 4294967296.;
#define ASIO64toDouble(a)  ((a).lo + (a).hi * twoRaisedTo32)
#endif

ASIOTime *bufferSwitchTimeInfo(ASIOTime *timeInfo, long index, ASIOBool processNow)
{	// the actual processing callback.
	// Beware that this is normally in a seperate thread, 
	// hence be sure that you take care of thread synchronization. 
	// This is omitted here for simplicity.
	static long processedSamples = 0;

	// store the timeInfo for later use
	asioDriverInfo.tInfo = *timeInfo;
	
	// get the time stamp of the buffer, not necessary if no
	// synchronization to other media is required
	if (timeInfo->timeInfo.flags & kSystemTimeValid)
		asioDriverInfo.nanoSeconds = ASIO64toDouble(timeInfo->timeInfo.systemTime);
	else
		asioDriverInfo.nanoSeconds = 0;

	if (timeInfo->timeInfo.flags & kSamplePositionValid)
		asioDriverInfo.samples = ASIO64toDouble(timeInfo->timeInfo.samplePosition);
	else
		asioDriverInfo.samples = 0;

	if (timeInfo->timeCode.flags & kTcValid)
		asioDriverInfo.tcSamples = ASIO64toDouble(timeInfo->timeCode.timeCodeSamples);
	else
		asioDriverInfo.tcSamples = 0;


	// get the system reference time
	asioDriverInfo.sysRefTime = get_sys_reference_time();
	/*
#if _DEBUG
	// a few debug messages for the Windows device driver developer
	// tells you the time when driver got its interrupt and the delay 
	// until the app receives the event notification.
	static double last_samples = 0;
	char tmp[128];
	sprintf(
		tmp,
		"diff: %d / %d ms / %d ms / %d samples                 \n",
		asioDriverInfo.sysRefTime - (long)(asioDriverInfo.nanoSeconds / 1000000.0),
		asioDriverInfo.sysRefTime,
		(long)(asioDriverInfo.nanoSeconds / 1000000.0),
		(long)(asioDriverInfo.samples - last_samples)
		);
	OutputDebugString(tmp);
	last_samples = asioDriverInfo.samples;
#endif
*/
	// buffer size in samples
	long buffSize = asioDriverInfo.preferredSize;

	// perform the processing; this part gives you access of all the channels

	for (int i = 0; i < asioDriverInfo.inputBuffers + asioDriverInfo.outputBuffers; i++)
	{
		if (asioDriverInfo.bufferInfos[i].isInput == false)
		{
			load_buffer(
				(int *)asioDriverInfo.bufferInfos[i].buffers[index],
				(i % 2)
				);
		}
		else
		{
//			store_buffer((int *)asioDriverInfo.bufferInfos[i].buffers[index],(i % 2));
			if (i % 2 == 0){
				store_left_buffer((int *)asioDriverInfo.bufferInfos[i].buffers[index]);
			}
			else{
				store_right_buffer((int *)asioDriverInfo.bufferInfos[i].buffers[index]);
			}

		}

	}

	// finally if the driver supports the ASIOOutputReady() optimization, do it here, all data are in place
	if (asioDriverInfo.postOutput)
		ASIOOutputReady();

	//processedSamples += buffSize;

	return 0L;
}

//----------------------------------------------------------------------------------
void bufferSwitch(long index, ASIOBool processNow)
{	// the actual processing callback.
	// Beware that this is normally in a seperate thread, 
	// hence be sure that you take care about thread synchronization. 
	// This is omitted here for simplicity.

	// as this is a "back door" into the bufferSwitchTimeInfo
	// a timeInfo needs to be created though it will only set the 
	// timeInfo.samplePosition and timeInfo.systemTime fields and 
	// the according flags
	ASIOTime  timeInfo;
	memset(&timeInfo, 0, sizeof(timeInfo));

	// get the time stamp of the buffer, not necessary if no
	// synchronization to other media is required
	if (ASIOGetSamplePosition(&timeInfo.timeInfo.samplePosition, &timeInfo.timeInfo.systemTime) == ASE_OK)
		timeInfo.timeInfo.flags = kSystemTimeValid | kSamplePositionValid;

	bufferSwitchTimeInfo(&timeInfo, index, processNow);
}


//----------------------------------------------------------------------------------
void sampleRateChanged(ASIOSampleRate sRate)
{
	// do whatever you need to do if the sample rate changed
	// usually this only happens during external sync.
	// Audio processing is not stopped by the driver, actual sample rate
	// might not have even changed, maybe only the sample rate status of an
	// AES/EBU or S/PDIF digital input at the audio device.
	// You might have to update time/sample related conversion routines, etc.
}

//----------------------------------------------------------------------------------
long asioMessages(long selector, long value, void* message, double* opt)
{
	// currently the parameters "value", "message" and "opt" are not used.
	long ret = 0;
	switch (selector)
	{
	case kAsioSelectorSupported:
		if (value == kAsioResetRequest
			|| value == kAsioEngineVersion
			|| value == kAsioResyncRequest
			|| value == kAsioLatenciesChanged
			// the following three were added for ASIO 2.0, 
			// you don't necessarily have to support them
			|| value == kAsioSupportsTimeInfo
			|| value == kAsioSupportsTimeCode
			|| value == kAsioSupportsInputMonitor)
			ret = 1L;
		break;
	case kAsioResetRequest:
		// defer the task and perform the reset of the driver 
		// during the next "safe" situation
		// You cannot reset the driver right now, 
		// as this code is called from the driver.
		// Reset the driver is done by completely destruct is. 
		// I.e. ASIOStop(), ASIODisposeBuffers(), Destruction
		// Afterwards you initialize the driver again.
		asioDriverInfo.stopped;  // In this sample the processing will just stop
		ret = 1L;
		break;
	case kAsioResyncRequest:
		// This informs the application, that the driver encountered 
		// some non fatal data loss.
		// It is used for synchronization purposes of different media.
		// Added mainly to work around the Win16Mutex problems 
		// in Windows 95/98 with the Windows Multimedia system, 
		// which could loose data because the Mutex was hold too long
		// by another thread.
		// However a driver can issue it in other situations, too.
		ret = 1L;
		break;
	case kAsioLatenciesChanged:
		// This will inform the host application 
		// that the drivers were latencies changed.
		// Beware, it this does not mean that the buffer sizes have changed!
		// You might need to update internal delay data.
		ret = 1L;
		break;
	case kAsioEngineVersion:
		// return the supported ASIO version of the host application
		// If a host applications does not implement this selector, 
		// ASIO 1.0 is assumed by the driver
		ret = 2L;
		break;
	case kAsioSupportsTimeInfo:
		// informs the driver wether the asioCallbacks.bufferSwitchTimeInfo() 
		// callback is supported.
		// For compatibility with ASIO 1.0 drivers 
		// the host application should always support
		// the "old" bufferSwitch method, too.
		ret = 1;
		break;
	case kAsioSupportsTimeCode:
		// informs the driver wether application is interested 
		// in time code info.
		// If an application does not need to know about time code, 
		// the driver has less work to do.
		ret = 0;
		break;
	}
	return ret;
}


//----------------------------------------------------------------------------------
ASIOError create_asio_buffers(DriverInfo *asioDriverInfo)
{	// create buffers for all inputs and outputs of the card with the 
	// preferredSize from ASIOGetBufferSize() as buffer size
	long i;
	ASIOError result;

	// fill the bufferInfos from the start without a gap
	ASIOBufferInfo *info = asioDriverInfo->bufferInfos;

	// prepare inputs (Though this is not necessaily required, 
	// no opened inputs will work, too
	if (asioDriverInfo->inputChannels > kMaxInputChannels)
		asioDriverInfo->inputBuffers = kMaxInputChannels;
	else
		asioDriverInfo->inputBuffers = asioDriverInfo->inputChannels;
	for (i = 0; i < asioDriverInfo->inputBuffers; i++, info++)
	{
		info->isInput = ASIOTrue;
		info->channelNum = i;
		info->buffers[0] = info->buffers[1] = 0;
	}

	// prepare outputs
	if (asioDriverInfo->outputChannels > kMaxOutputChannels)
		asioDriverInfo->outputBuffers = kMaxOutputChannels;
	else
		asioDriverInfo->outputBuffers = asioDriverInfo->outputChannels;
	for (i = 0; i < asioDriverInfo->outputBuffers; i++, info++)
	{
		info->isInput = ASIOFalse;
		info->channelNum = i;
		info->buffers[0] = info->buffers[1] = 0;
	}

	// create and activate buffers
	result = ASIOCreateBuffers(asioDriverInfo->bufferInfos,
		asioDriverInfo->inputBuffers + asioDriverInfo->outputBuffers,
		asioDriverInfo->preferredSize, &asioCallbacks);
	if (result == ASE_OK)
	{
		// now get all the buffer details, sample word length, 
		// name, word clock group and activation
		for (i = 0; i < asioDriverInfo->inputBuffers + asioDriverInfo->outputBuffers; i++)
		{
			asioDriverInfo->channelInfos[i].channel = asioDriverInfo->bufferInfos[i].channelNum;
			asioDriverInfo->channelInfos[i].isInput = asioDriverInfo->bufferInfos[i].isInput;
			result = ASIOGetChannelInfo(&asioDriverInfo->channelInfos[i]);
			if (result != ASE_OK)
				break;
		}

		if (result == ASE_OK)
		{
			// get the input and output latencies
			// Latencies often are only valid after ASIOCreateBuffers()
			// (input latency is the age of the first sample 
			// in the currently returned audio block)
			// (output latency is the time the first sample 
			// in the currently returned audio block requires to get to the output)
			result = ASIOGetLatencies(&asioDriverInfo->inputLatency, &asioDriverInfo->outputLatency);
			if (result == ASE_OK)
				printf(
				"ASIOGetLatencies (input: %d, output: %d);\n",
				asioDriverInfo->inputLatency,
				asioDriverInfo->outputLatency
				);
		}
	}
	return result;
}

unsigned long get_sys_reference_time()
{	// get the system reference time
	return 0;//TimeGetTime();
}

void load_buffer(int *pnBuffer, int nLeftOrRight)
{
	static int nBufferIndex = 0;
	if (nLeftOrRight == 1)
	{
		nBufferIndex++;
	}
	return;
}

void store_left_buffer(int *pnbuffer){
	
	
	SetTextColor(hDC, textcolor);
	SetBkMode(hDC, TRANSPARENT);
	TextOutA(hDC, 10, 20, "Left:", 6);
	POINT leftplot[TWICENUM];
	RECT rect, smallrect,tinyrect;
	int i, j, k;
	char str[300];
	short *gaussianBuffer;
	int nWrite = 0;
	short sbuffer[512];
	float test;
	float *ffreq;
	for (i = 0; i < 512; i++){
		sbuffer[i] = pnbuffer[i] / 65536;	//convert 32bit integer into 16bit short
	}
	nWrite = fwrite(sbuffer, sizeof(short), 512, fp);

		switch (nleftcount){
			case 0:
				for (i = 0; i < HALFNUM; i++){				
					spLeftBuffer[i] = sbuffer[i];
				}											
				break;
			case 1:
				for (i = 0; i < HALFNUM; i++){
					spLeftBuffer[i+HALFNUM] = sbuffer[i];
				}
				break;
			case 2:
				for (i = NUM; i < HALFNUM; i++){
					spLeftBuffer[i+NUM] = sbuffer[i];
				}
				break;
			case 3:
				for (i = 0; i < HALFNUM; i++){				
					spLeftBuffer[i +NUM+HALFNUM] = sbuffer[i];
				}
				gaussianBuffer = filter1D(spLeftBuffer, TWICENUM, Gkernel, 7, 3, 0);
					ffreq = get_pitch_of_sample(gaussianBuffer, TWICENUM);
					currfrequency = ffreq[0];
					if ((currfrequency - prevfrequency)*(currfrequency - prevfrequency)>25.0 && currfrequency < 20000.0){
						FillRect(hDC, &freqRect, hbrushbackground);		//repaint black rectangle
						sprintf(str, "Freq: %dHz", (int)currfrequency);	//print the data
						TextOut(hDC, (uWidth * 5) / 6, uHeight / 2 - 7
							, str, strlen(str));
						prevfrequency = currfrequency;
					}
				for (i = 0; i < TWICENUM; i++){
					test = gaussianBuffer[i];
				}
				FillRect(hDC, &signalrectL, hbrushGrey);		//repaint black rectangle
				for (i = 0; i < TWICENUM; i++){
					leftplot[i].x = (int)(uWidth / 20.0 + ((18 * uWidth) / 20.0)*((i*1.0) / TWICENUM));
					leftplot[i].y = (int)(uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 - gaussianBuffer[i] /32768.0));
				}
				SelectObject(hDC, hpenblue);			//drawing 3 blue lines and two red lines 
				MoveToEx(hDC, uWidth/20.0-1, uHeight / 4, NULL);
				LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 4);
				SelectObject(hDC, hpenred);
				MoveToEx(hDC, uWidth / 20.0 - 1, uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1-1 / 2.0), NULL);
				LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight /20.0 + (uHeight/4.0-uHeight/20.0)*(1-1/2.0));
				MoveToEx(hDC, uWidth / 20.0 - 1, uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1+1 / 2.0), NULL);
				LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1+1 / 2.0));
				SelectObject(hDC, hpenblue);
				MoveToEx(hDC, uWidth / 20.0 - 1, uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1-3 / 4.0), NULL);
				LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1-3 / 4.0));
				MoveToEx(hDC, uWidth / 20.0 - 1, uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1+3 / 4.0), NULL);
				LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1+3 / 4.0));

				SelectObject(hDC, hpenyellow);
				Polyline(hDC, leftplot, TWICENUM);			//plot the spLeftBuffer. Since the buffer keeps coming 
															//hence we get a dynamic wave effect
				for (i = 0; i < HALFNUM+NUM; i++){			//left-shift the 2048 size array by a step of 512
															//to accomodate the next sample
					spLeftBuffer[i] = spLeftBuffer[i+HALFNUM];
				}
				break;
		}

		if (nleftcount!=3)
		nleftcount++;
		
		return;
}

void store_right_buffer(int *pnbuffer){

	POINT rightplot[TWICENUM];
	RECT rect, smallrect, tinyrect;
	int i, j, k;
	char str[300];
	int nWrite = 0;
	short sbuffer[512];
	for (i = 0; i < 512; i++){
		sbuffer[i] = pnbuffer[i] / 65536;	//convert 32bit integer into 16bit short
	}
	nWrite = fwrite(sbuffer, sizeof(short), 512, fp);

	SetTextColor(hDC, textcolor);
	SetBkMode(hDC, TRANSPARENT);
	TextOutA(hDC, 10, uHeight / 2 + 20, "Right:", 6);

	switch (nrightcount){
	case 0:
		for (i = 0; i < HALFNUM; i++){
			spRightBuffer[i] = sbuffer[i];
		}
		break;
	case 1:
		for (i = 0; i < HALFNUM; i++){
			spRightBuffer[i + HALFNUM] = sbuffer[i];
		}
		break;
	case 2:
		for (i = NUM; i < HALFNUM; i++){
			spRightBuffer[i + NUM] = sbuffer[i];
		}
		break;
	case 3:
		for (i = 0; i < HALFNUM; i++){
			spRightBuffer[i + NUM + HALFNUM] = sbuffer[i];
		}
		FillRect(hDC, &signalrectR, hbrushGrey);		//repaint black rectangle
		for (i = 0; i < TWICENUM; i++){
			rightplot[i].x = (int)(uWidth / 20.0 + ((18 * uWidth) / 20.0)*((i*1.0) / TWICENUM));
			rightplot[i].y = (int)(uHeight / 20.0 + uHeight/2 + (uHeight / 4.0 - uHeight / 20.0)*(1 - spRightBuffer[i] / 32768.0));
		}
		SelectObject(hDC, hpenblue);			//drawing 3 blue lines and two red lines 
		MoveToEx(hDC, uWidth / 20.0 - 1, uHeight / 2 + uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 - 3 / 4.0), NULL);
		LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 2 + uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 - 3 / 4.0));
		MoveToEx(hDC, uWidth / 20.0 - 1, uHeight / 2 + uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 + 3 / 4.0), NULL);
		LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 2 + uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 + 3 / 4.0));
		MoveToEx(hDC, uWidth / 20.0 - 1, uHeight / 2 + uHeight / 4, NULL);
		LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 2 + uHeight / 4);
		SelectObject(hDC, hpenred);
		MoveToEx(hDC, uWidth / 20.0 - 1, uHeight / 2 + uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 - 1 / 2.0), NULL);
		LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 2 + uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 - 1 / 2.0));
		MoveToEx(hDC, uWidth / 20.0 - 1, uHeight / 2 + uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 + 1 / 2.0), NULL);
		LineTo(hDC, uWidth * 19 / 20.0 + 1, uHeight / 2 + uHeight / 20.0 + (uHeight / 4.0 - uHeight / 20.0)*(1 + 1 / 2.0));

		SelectObject(hDC, hpenyellow);
		Polyline(hDC, rightplot, TWICENUM);			//plot the spRightBuffer. Since the buffer keeps coming 
		//hence we get a dynamic wave effect
		for (i = 0; i < HALFNUM + NUM; i++){			//right-shift the 2048 size array by a step of 512
			//to accomodate the next sample
			spRightBuffer[i] = spRightBuffer[i + HALFNUM];
		}
		break;
	}

	if (nrightcount != 3)
		nrightcount++;

	return;
}


