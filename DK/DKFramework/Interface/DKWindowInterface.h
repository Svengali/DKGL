//
//  File: DKWindowInterface.h
//  Author: Hongtae Kim (tiff2766@gmail.com)
//
//  Copyright (c) 2004-2015 Hongtae Kim. All rights reserved.
//

#pragma once
#include "../DKRect.h"
#include "../DKVKey.h"
#include "DKBackendInterface.h"

////////////////////////////////////////////////////////////////////////////////
// DKWindowInterface
// An abstract class, interface for GUI window.
// You may need to subclass for your platform, If you have plan to use
// DKWindow.
////////////////////////////////////////////////////////////////////////////////

namespace DKGL
{
	class DKWindow;
	class DKWindowInterface : public DKBackendInterface
	{
	public:
		virtual bool Create(const DKString& title, const DKSize& size, const DKPoint& origin, int style) = 0;
		virtual bool CreateProxy(void* systemHandle) = 0;
		virtual bool IsProxy(void) const = 0;
		virtual void UpdateProxy(void) = 0;
		virtual void Destroy(void) = 0;
		virtual void* PlatformHandle(void) const = 0;
		virtual bool IsValid(void) const = 0;

		virtual void ShowMouse(int deviceId, bool bShow) = 0;
		virtual bool IsMouseVisible(int deviceId) const = 0;
		virtual void HoldMouse(int deviceId, bool bHold) = 0;
		virtual bool IsMouseHeld(int deviceId) const = 0;

		virtual void Show(void) = 0;
		virtual void Hide(void) = 0;
		virtual void Activate(void) = 0;
		virtual void Minimize(void) = 0;

		// Window's origin, size is based on system GUI coordinates.
		DKPoint Origin(void) const;
		virtual void SetOrigin(const DKPoint&) = 0;
		virtual void Resize(const DKSize&, const DKPoint* optionalOrigin) = 0;

		DKSize ContentSize(void) const;  // pixel based coords

		virtual double ContentScaleFactor(void) const = 0;  // logical coords by pixel ratio.

		virtual void SetTitle(const DKString& title) = 0;
		virtual DKString Title(void) const = 0;

		virtual void SetMousePosition(int deviceId, const DKPoint& pt) = 0;
		virtual DKPoint MousePosition(int deviceId) const = 0;

		virtual void EnableTextInput(int deviceId, bool bTextInput) = 0;

		static DKWindowInterface* CreateInterface(DKWindow*);
	};
}
