//
//  File: ComputeCommandEncoder.h
//  Author: Hongtae Kim (tiff2766@gmail.com)
//
//  Copyright (c) 2016-2017 Hongtae Kim. All rights reserved.
//

#pragma once
#include "../GraphicsAPI.h"
#if DKGL_ENABLE_VULKAN
#include <vulkan/vulkan.h>

#include "../../DKComputeCommandEncoder.h"

#include "CommandBuffer.h"

namespace DKFramework
{
	namespace Private
	{
		namespace Vulkan
		{
			class ComputeCommandEncoder : public DKComputeCommandEncoder
			{
			public:
				ComputeCommandEncoder(VkCommandBuffer, CommandBuffer*);
				~ComputeCommandEncoder(void);

				void EndEncoding(void) override;
				bool IsCompleted(void) const override { return false; }
				DKCommandBuffer* Buffer(void) override;

				VkCommandBuffer encodingBuffer;
				DKObject<CommandBuffer> commandBuffer;
			};
		}
	}
}
#endif //#if DKGL_ENABLE_VULKAN
