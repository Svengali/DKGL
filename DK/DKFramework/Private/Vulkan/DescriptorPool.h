//
//  File: DescriptorPool.h
//  Author: Hongtae Kim (tiff2766@gmail.com)
//
//  Copyright (c) 2016-2019 Hongtae Kim. All rights reserved.
//

#pragma once
#include "../GraphicsAPI.h"
#if DKGL_ENABLE_VULKAN
#include <vulkan/vulkan.h>
#include "Types.h"

namespace DKFramework::Private::Vulkan
{
    class GraphicsDevice;

    struct DescriptorPoolId
    {
        uint32_t mask = 0;
        uint32_t typeSize[VK_DESCRIPTOR_TYPE_RANGE_SIZE] = { 0 };

        DescriptorPoolId() = default;
        DescriptorPoolId(uint32_t poolSizeCount, const VkDescriptorPoolSize* poolSizes)
            : mask(0)
            , typeSize{ 0 }
        {
            for (uint32_t i = 0; i < poolSizeCount; ++i)
            {
                const VkDescriptorPoolSize& poolSize = poolSizes[i];
                uint32_t typeIndex = poolSize.type - VK_DESCRIPTOR_TYPE_BEGIN_RANGE;
                typeSize[typeIndex] += poolSize.descriptorCount;
            }
            uint32_t dpTypeKey = 0;
            for (uint32_t i = 0; i < VK_DESCRIPTOR_TYPE_RANGE_SIZE; ++i)
            {
                if (typeSize[i])
                    mask = mask | (1 << i);
            }
        }
        DescriptorPoolId(const DKShaderBindingSetLayout& layout)
            : mask(0)
            , typeSize{ 0 }
        {
            for (const DKShaderBinding& binding : layout.bindings)
            {
                VkDescriptorType type = DescriptorType(binding.type);
                uint32_t typeIndex = type - VK_DESCRIPTOR_TYPE_BEGIN_RANGE;
                typeSize[typeIndex] += binding.arrayLength;
            }
            uint32_t dpTypeKey = 0;
            for (uint32_t i = 0; i < VK_DESCRIPTOR_TYPE_RANGE_SIZE; ++i)
            {
                if (typeSize[i])
                    mask = mask | (1 << i);
            }
        }
        bool operator < (const DescriptorPoolId& rhs) const
        {
            if (this->mask == rhs.mask)
            {
                for (uint32_t i = 0; i < std::size(typeSize); ++i)
                {
                    if (this->typeSize[i] != rhs.typeSize[i])
                        return this->typeSize[i] < rhs.typeSize[i];
                }
            }
            return this->mask < rhs.mask;
        }
        bool operator > (const DescriptorPoolId& rhs) const
        {
            if (this->mask == rhs.mask)
            {
                for (uint32_t i = 0; i < std::size(typeSize); ++i)
                {
                    if (this->typeSize[i] != rhs.typeSize[i])
                        return this->typeSize[i] > rhs.typeSize[i];
                }
            }
            return this->mask > rhs.mask;
        }
        bool operator == (const DescriptorPoolId& rhs) const
        {
            if (this->mask == rhs.mask)
            {
                for (uint32_t i = 0; i < std::size(typeSize); ++i)
                {
                    if (this->typeSize[i] != rhs.typeSize[i])
                        return false;
                }
                return true;
            }
            return false;
        }
    };

    class DescriptorPool final
    {
    public:
        const DescriptorPoolId& poolId; // key for container(DescriptorPoolChain)
        const uint32_t maxSets;
        const VkDescriptorPoolCreateFlags createFlags;

        VkDescriptorPool pool;
        GraphicsDevice* device;
        uint32_t numAllocatedSets;


        DescriptorPool(GraphicsDevice*, VkDescriptorPool, const VkDescriptorPoolCreateInfo& ci, const DescriptorPoolId&);
        ~DescriptorPool();

        VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout);

        void ReleaseDescriptorSets(VkDescriptorSet*, size_t);

    };
}
#endif //#if DKGL_ENABLE_VULKAN
