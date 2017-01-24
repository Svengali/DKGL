//
//  File: GraphicsDevice.mm
//  Platform: OS X, iOS
//  Author: Hongtae Kim (tiff2766@gmail.com)
//
//  Copyright (c) 2015-2017 Hongtae Kim. All rights reserved.
//

#include "../GraphicsAPI.h"
#if DKGL_USE_METAL
#include <stdexcept>
#include "GraphicsDevice.h"
#include "CommandQueue.h"
#include "../../DKPropertySet.h"

namespace DKFramework
{
	namespace Private
	{
		namespace Metal
		{
			DKGraphicsDeviceInterface* CreateInterface(void)
			{
				return DKRawPtrNew<GraphicsDevice>();
			}
		}
	}
}

using namespace DKFramework;
using namespace DKFramework::Private::Metal;


GraphicsDevice::GraphicsDevice(void)
{
	@autoreleasepool {
#if !TARGET_OS_IPHONE
		NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();

		NSString* preferredDeviceName = @"";
		// get preferred device.
		if (DKPropertySet::SystemConfig().HasValue(preferredDeviceNameKey))
		{
			if (DKPropertySet::SystemConfig().Value(preferredDeviceNameKey).ValueType() == DKVariant::TypeString)
			{
				DKString prefDevName = DKPropertySet::SystemConfig().Value(preferredDeviceNameKey).String();
				if (prefDevName.Length() > 0)
					preferredDeviceName = [NSString stringWithUTF8String:(const char*)DKStringU8(prefDevName)];
			}
		}

		// save device list into system config.
		DKVariant deviceList = DKVariant::TypeArray;
		uint32_t deviceIndex = 0;
		for (id<MTLDevice> dev in devices)
		{
			bool pref = false;
			if (this->device == nil && preferredDeviceName)
			{
				if ([preferredDeviceName caseInsensitiveCompare:dev.name] == NSOrderedSame)
				{
					this->device = [dev retain];
					pref = true;
				}
			}

			DKString deviceName = DKString(dev.name.UTF8String);
			deviceList.Array().Add(deviceName);
			DKLog("METAL: Device[%u]: \"%s\"%s", deviceIndex, dev.name.UTF8String, pref? " (Preferred)" : "");
			deviceIndex++;
		}
		DKPropertySet::SystemConfig().SetValue(graphicsDeviceListKey, deviceList);
#endif	//if !TARGET_OS_IPHONE
		
		if (this->device == nil)
			device = MTLCreateSystemDefaultDevice();
	}

	if (this->device == nil)
	{
		throw std::runtime_error("No metal device.");
	}
}

GraphicsDevice::~GraphicsDevice(void)
{
	[device autorelease];
}

DKString GraphicsDevice::DeviceName(void) const
{
	return DKString( device.name.UTF8String );
}

DKObject<DKCommandQueue> GraphicsDevice::CreateCommandQueue(DKGraphicsDevice* ctxt)
{
	DKObject<CommandQueue> queue = DKOBJECT_NEW CommandQueue();
	queue->queue = [device newCommandQueue];
	queue->device = ctxt;
	return queue.SafeCast<DKCommandQueue>();
}

#endif //#if DKGL_USE_METAL
