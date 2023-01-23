// #include "VulkanDevice.h"

// DescriptorSetLayout::DescriptorSetLayout(VulkanDevice* device, const DescriptorSetLayoutDesc& desc)
//     : m_Device(std::move(device))
// {
//     u32 maxBindingIndex = 0;
//     for (const auto& inputBinding : desc.bindings)
//     {
//         maxBindingIndex = std::max(maxBindingIndex, inputBinding.index);
//     }
//     assert(maxBindingIndex <= GPUConstants::MaxShaderBindingIndex);

//     m_SetIndex = desc.setIndex;
//     m_Bindless = desc.bindless;
//     m_Dynamic = desc.dynamic;

//     m_Bindings.resize(desc.bindings.size());
//     m_BindingDataIndices.resize(GPUConstants::MaxShaderBindingIndex);

//     const bool skipBindlessBindings = m_Device->UseBindless() && desc.bindless;

//     for (u32 i = 0; i < desc.bindings.size(); i++)
//     {
//         const auto& inputBinding = desc.bindings[i];
//         assert(inputBinding.index != u32(-1));

//         // Need to push binding(even if this binding is not needed/bindless) to properly index DescriptorBinding
//         array
//         // from m_IndexToBinding.
//         m_Bindings[i] = inputBinding;
//         m_BindingDataIndices[inputBinding.index] = i;

//         // Skip bindings for bindless images.
//         if (desc.setIndex == 0 && skipBindlessBindings
//             && (inputBinding.type == vk::DescriptorType::eCombinedImageSampler
//                 || inputBinding.type == vk::DescriptorType::eStorageImage))
//         {
//             continue;
//         }

//         auto vulkanBinding = vk::DescriptorSetLayoutBinding()
//                                  .setBinding(inputBinding.index)
//                                  .setDescriptorType(inputBinding.type)
//                                  .setDescriptorCount(inputBinding.count)
//                                  .setStageFlags(inputBinding.shaderStageFlags);

//         // Always use dynamic uniform buffers?
//         if (vulkanBinding.descriptorType == vk::DescriptorType::eUniformBuffer)
//         {
//             vulkanBinding.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic);
//         }

//         m_VulkanBindings.push_back(vulkanBinding);
//     }

//     auto descriptorSetLayoutInfo = vk::DescriptorSetLayoutCreateInfo().setBindings(m_VulkanBindings);

//     if (desc.bindless && m_Device->UseBindless())
//     {
//         descriptorSetLayoutInfo.setFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool);

//         const auto bindlessFlags = vk::DescriptorBindingFlagBits::ePartiallyBound
//                                    | vk::DescriptorBindingFlagBits::eUpdateAfterBind
//                                    | vk::DescriptorBindingFlagBits::eVariableDescriptorCount;
//         std::vector<vk::DescriptorBindingFlags> bindingFlags(m_VulkanBindings.size(), bindlessFlags);

//         auto bindingFlagsInfo = vk::DescriptorSetLayoutBindingFlagsCreateInfo().setBindingFlags(bindingFlags);
//         descriptorSetLayoutInfo.setPNext(&bindingFlagsInfo);

//         auto vkRes = m_Device->GetVulkanDevice().createDescriptorSetLayout(
//             &descriptorSetLayoutInfo, m_Device->GetVulkanAllocationCallbacks(), &m_VulkanDescriptorSetLayout);
//         VK_CHECK(vkRes);
//     }
//     else
//     {
//         auto vkRes = m_Device->GetVulkanDevice().createDescriptorSetLayout(
//             &descriptorSetLayoutInfo, m_Device->GetVulkanAllocationCallbacks(), &m_VulkanDescriptorSetLayout);
//         VK_CHECK(vkRes)
//     }
// }

// DescriptorSetLayout::~DescriptorSetLayout()
// {
//     if (m_VulkanDescriptorSetLayout)
//     {
//         m_Device->GetDeferredDeletionQueue().EnqueueResource(DeferredDeletionQueue::Type::DescriptorSetLayout,
//                                                              m_VulkanDescriptorSetLayout);
//     }
// }

// DescriptorSetLayoutRef VulkanDevice::CreateDescriptorSetLayout(const DescriptorSetLayoutDesc& desc)
// {
//     auto descriptorSetLayoutRef = DescriptorSetLayoutRef::Create(new DescriptorSetLayout(this, desc));
//     if (!descriptorSetLayoutRef->GetVulkanHandle())
//     {
//         descriptorSetLayoutRef = nullptr;
//     }
//     return descriptorSetLayoutRef;
// }

// static void FillVulkanWriteDescriptorSets(const DescriptorSetLayout* descriptorSetLayout,
//                                           vk::DescriptorSet vulkanDescriptorSet,
//                                           std::vector<vk::WriteDescriptorSet>& vulkanWriteDescriptorSets,
//                                           std::vector<vk::DescriptorBufferInfo>& vulkanBufferDescriptors,
//                                           std::vector<vk::DescriptorImageInfo>& vulkanImageDescriptors,
//                                           vk::Sampler vulkanDefaultSampler,
//                                           const DescriptorSetResourcesArray& descriptorSetResources,
//                                           const ShaderBindingIndicesArray& shaderBindings)
// {
//     assert(descriptorSetResources.size() == shaderBindings.size());

//     const u32 numResources = descriptorSetResources.size();
//     const bool skipBindlessBindings = descriptorSetLayout->IsBindless();
//     const auto& bindingDataIndices = descriptorSetLayout->GetBindingDataIndices();
//     const auto& bindingData = descriptorSetLayout->GetBindings();

//     // Since image descriptors need to be initialized first before assigning to the write descriptors and we still do
//     // not know how many images will be used, set a maximum image descriptors value and reserve memory for the image
//     // descritpors.
//     static constexpr u32 maxSampledImagesPerShader = 1028;
//     vulkanImageDescriptors.reserve(maxSampledImagesPerShader);
//     // u32 currentImageCount = 0;

//     vulkanBufferDescriptors.reserve(numResources);
//     vulkanWriteDescriptorSets.reserve(numResources);

//     u32 usedResources = 0;

//     for (u32 i = 0; i < numResources; i++)
//     {
//         u32 shaderBindingIndex = shaderBindings[i];
//         u32 bindingDataIndex = descriptorSetLayout->GetBindingDataIndices()[shaderBindingIndex];
//         const DescriptorBinding& binding = descriptorSetLayout->GetBindings()[bindingDataIndex];

//         assert(binding.count > 0);

//         // Skip bindings for bindless images.
//         if ((descriptorSetLayout->GetSetIndex() == 0) && skipBindlessBindings
//             && (binding.type == vk::DescriptorType::eCombinedImageSampler
//                 || binding.type == vk::DescriptorType::eStorageImage))
//         {
//             continue;
//         }

//         u32 currentIndex = usedResources++;

//         auto resource = descriptorSetResources[i];

//         auto& descriptorWrite = vulkanWriteDescriptorSets.emplace_back();
//         descriptorWrite.setDstSet(vulkanDescriptorSet)
//             .setDstBinding(binding.index)
//             .setDstArrayElement(0)
//             .setDescriptorCount(binding.count)
//             .setDescriptorType(binding.type);

//         switch (binding.type)
//         {
//         case vk::DescriptorType::eCombinedImageSampler: {
//             if (binding.count == 1)
//             {
//                 assert(std::holds_alternative<GPUResource*>(resource.data));
//                 auto texture = static_cast<Texture*>(get<GPUResource*>(resource.data));

//                 auto& imageDescriptor = vulkanImageDescriptors.emplace_back();
//                 imageDescriptor.setSampler(vulkanDefaultSampler)
//                     .setImageView(texture->GetVulkanView())
//                     .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

//                 // imageDescriptor.setImageLayout(vk::ImageLayout::eReadOnlyOptimal);

//                 if (texture->GetSampler() != nullptr)
//                 {
//                     imageDescriptor.setSampler(texture->GetSampler()->GetVulkanHandle());
//                 }

//                 if (resource.sampler != nullptr)
//                 {
//                     imageDescriptor.setSampler(resource.sampler->GetVulkanHandle());
//                 }

//                 descriptorWrite.setImageInfo(imageDescriptor);
//                 // currentImageCount++;
//             }
//             else
//             {
//                 assert(std::holds_alternative<std::vector<GPUResource*>>(resource.data));
//                 const auto& resourceArray = get<std::vector<GPUResource*>>(resource.data);

//                 for (const auto resourceArrayElement : resourceArray)
//                 {
//                     auto texture = static_cast<Texture*>(resourceArrayElement);

//                     auto& imageDescriptor = vulkanImageDescriptors.emplace_back();
//                     imageDescriptor.setSampler(vulkanDefaultSampler)
//                         .setImageView(texture->GetVulkanView())
//                         .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

//                     if (texture->GetSampler() != nullptr)
//                     {
//                         imageDescriptor.setSampler(texture->GetSampler()->GetVulkanHandle());
//                     }

//                     if (resource.sampler != nullptr)
//                     {
//                         imageDescriptor.setSampler(resource.sampler->GetVulkanHandle());
//                     }
//                 }
//             }

//             break;
//         }
//         case vk::DescriptorType::eStorageImage: {
//             auto resource = descriptorSetResources[i];
//             assert(std::holds_alternative<GPUResource*>(resource.data));
//             auto texture = static_cast<Texture*>(get<GPUResource*>(resource.data));

//             auto& imageDescriptor = vulkanImageDescriptors.emplace_back();
//             imageDescriptor.setSampler(nullptr)
//                 .setImageLayout(vk::ImageLayout::eGeneral)
//                 .setImageView(texture->GetVulkanView());

//             descriptorWrite.setImageInfo(imageDescriptor);

//             // currentImageCount++;

//             break;
//         }
//         case vk::DescriptorType::eUniformBuffer: {
//             auto resource = descriptorSetResources[i];
//             assert(std::holds_alternative<GPUResource*>(resource.data));
//             auto buffer = static_cast<Buffer*>(get<GPUResource*>(resource.data));

//             if (buffer->GetResourceUsage() == ResourceUsageType::Dynamic)
//             {
//                 descriptorWrite.setDescriptorType(vk::DescriptorType::eUniformBufferDynamic);
//             }

//             auto& bufferDescriptor = vulkanBufferDescriptors.emplace_back();
//             bufferDescriptor.setOffset(0).setRange(buffer->GetSize());

//             // Bind parent buffer for dynamic resources.
//             if (buffer->GetParentBuffer() != nullptr)
//             {
//                 assert(!buffer->GetVulkanHandle());
//                 bufferDescriptor.setBuffer(buffer->GetParentBuffer()->GetVulkanHandle());
//             }
//             else
//             {
//                 bufferDescriptor.setBuffer(buffer->GetVulkanHandle());
//             }

//             descriptorWrite.setBufferInfo(bufferDescriptor);

//             break;
//         }
//         case vk::DescriptorType::eStorageBuffer: {
//             auto resource = descriptorSetResources[i];
//             assert(std::holds_alternative<GPUResource*>(resource.data));
//             auto buffer = static_cast<Buffer*>(get<GPUResource*>(resource.data));

//             auto& bufferDescriptor = vulkanBufferDescriptors.emplace_back();
//             bufferDescriptor.setOffset(0).setRange(buffer->GetSize());

//             // Bind parent buffer for dynamic resources.
//             if (buffer->GetParentBuffer() != nullptr)
//             {
//                 assert(!buffer->GetVulkanHandle());
//                 bufferDescriptor.setBuffer(buffer->GetParentBuffer()->GetVulkanHandle());
//             }
//             else
//             {
//                 bufferDescriptor.setBuffer(buffer->GetVulkanHandle());
//             }

//             descriptorWrite.setBufferInfo(bufferDescriptor);

//             break;
//         }

//         default: {
//             LOG_ERROR("FillWriteDescriptorSet: Descriptor type not supported!");
//             break;
//         }
//         }
//     }
// }

// DescriptorSet::DescriptorSet(VulkanDevice* device, const DescriptorSetDesc& desc) : m_Device(std::move(device))
// {
//     assert(desc.descriptorSetLayout);

//     const auto vulkanDevice = m_Device->GetVulkanDevice();
//     const auto setLayout = desc.descriptorSetLayout;
//     const auto vulkanDescriptorSetLayout = setLayout->GetVulkanHandle();

//     // setLayout->IsBindless() => m_Device->UseBindless().
//     assert(!setLayout->IsBindless() || m_Device->UseBindless());
//     const bool isBindless = setLayout->IsBindless();

//     auto descriptorSetAllocateInfo = vk::DescriptorSetAllocateInfo()
//                                          .setDescriptorPool(isBindless ? m_Device->GetVulkanBindlessDescriptorPool()
//                                                                        : m_Device->GetVulkanGlobalDescriptorPool())
//                                          .setSetLayouts(vulkanDescriptorSetLayout);

//     if (isBindless)
//     {
//         const u32 maxBinding = GPUConstants::MaxBindlessResources - 1;
//         auto descriptorCountInfo
//             = vk::DescriptorSetVariableDescriptorCountAllocateInfo().setDescriptorCounts(maxBinding);

//         descriptorSetAllocateInfo.pNext = &descriptorCountInfo;

//         VK_CHECK(vulkanDevice.allocateDescriptorSets(&descriptorSetAllocateInfo, &m_VulkanDescriptorSet));
//     }
//     else
//     {
//         VK_CHECK(vulkanDevice.allocateDescriptorSets(&descriptorSetAllocateInfo, &m_VulkanDescriptorSet));
//     }

//     std::vector<vk::WriteDescriptorSet> vulkanWriteDescriptorSets;
//     std::vector<vk::DescriptorBufferInfo> vulkanBufferDescriptors;
//     std::vector<vk::DescriptorImageInfo> vulkanImageDescriptors;

//     // XXX TODO: Create default sampler.

//     FillVulkanWriteDescriptorSets(desc.descriptorSetLayout, m_VulkanDescriptorSet, vulkanWriteDescriptorSets,
//                                   vulkanBufferDescriptors, vulkanImageDescriptors, nullptr, desc.resources,
//                                   desc.shaderBindings);

//     vulkanDevice.updateDescriptorSets(desc.resources.size(), vulkanWriteDescriptorSets.data(), 0, nullptr);

//     m_DescriptorSetResources = desc.resources;
//     m_Desc = desc;
//     m_DescriptorSetResources = desc.resources;
//     m_ShaderBindingIndices = desc.shaderBindings;
//     m_Layout = DescriptorSetLayoutRef::Create(setLayout);
// }