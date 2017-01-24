//
//  File: CommandQueue.mm
//  Platform: OS X, iOS
//  Author: Hongtae Kim (tiff2766@gmail.com)
//
//  Copyright (c) 2015-2017 Hongtae Kim. All rights reserved.
//

#include "../GraphicsAPI.h"
#if DKGL_USE_METAL
#include <TargetConditionals.h>

#include "CommandQueue.h"
#include "CommandBuffer.h"
#include "SwapChain.h"

using namespace DKFramework;
using namespace DKFramework::Private::Metal;

CommandQueue::~CommandQueue(void)
{
	[queue autorelease];
}

DKObject<DKCommandBuffer> CommandQueue::CreateCommandBuffer(void)
{
	DKObject<CommandBuffer> buffer = DKOBJECT_NEW CommandBuffer();
	buffer->buffer = [[queue commandBuffer] retain];
	buffer->queue = this;
	return buffer.SafeCast<DKCommandBuffer>();
}

DKObject<DKSwapChain> CommandQueue::CreateSwapChain(DKWindow* window)
{
	DKObject<SwapChain> swapChain = DKOBJECT_NEW SwapChain(this, window);
	if (swapChain->Setup())
	{

		return swapChain.SafeCast<DKSwapChain>();
	}
	return NULL;
}

#endif //#if DKGL_USE_METAL
