#pragma once
#ifndef __SsdSimApp_h__
#define __SsdSimApp_h__

#define NOMINMAX
#include <Windows.h>

bool LaunchProcess(const char *pFile, const char *pArgs, SHELLEXECUTEINFO & ShExecInfo);

#endif