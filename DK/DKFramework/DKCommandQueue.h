//
//  File: DKCommandQueue.h
//  Author: Hongtae Kim (tiff2766@gmail.com)
//
//  Copyright (c) 2015-2017 Hongtae Kim. All rights reserved.
//

#pragma once
#include "../DKFoundation.h"
#include "DKRenderPass.h"
#include "DKCommandBuffer.h"
#include "DKSwapChain.h"

namespace DKFramework
{
	class DKGraphicsDevice;
	class DKWindow;

	/// @brief GPU command queue
	/// every queues can perform blit commands by default
	class DKCommandQueue
	{
	public:
		enum TypeFlags : uint32_t
		{
			Graphics = 1,		// Graphics and Blit commands
			Compute = 1 << 1,	// Compute and Blit commands
		};

		virtual ~DKCommandQueue(void) {}

		virtual DKObject<DKCommandBuffer> CreateCommandBuffer(void) = 0;
		virtual DKObject<DKSwapChain> CreateSwapChain(DKWindow*) = 0;

		virtual uint32_t Type(void) const = 0;
		virtual DKGraphicsDevice* Device(void) = 0;
	};

	inline DKGraphicsDevice* DKCommandBuffer::Device(void)
	{
		return Queue()->Device();
	}
}
