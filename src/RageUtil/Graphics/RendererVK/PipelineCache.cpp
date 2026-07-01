#include "PipelineCache.h"
#include "VkUtils.h"
#include "RageUtil/File/RageFile.h"

void
PipelineCache::Init()
{
	assert(m_Device != nullptr);
	assert(m_TextureLayout != nullptr);
	assert(m_DescriptorSetLayout != nullptr);

	vk::PipelineCacheCreateInfo createInfo{};

	std::vector<uint8_t> persistedData;
	RageFile file;
	if (file.Open(std::string(CacheName), RageFile::READ)) {
		persistedData.resize(file.GetFileSize());
		file.Read(&persistedData[0], file.GetFileSize());

		createInfo.setInitialData<uint8_t>(persistedData);
	}

	m_DriverCache = vk::raii::PipelineCache(*m_Device, createInfo);
}

void
PipelineCache::WriteToDisk()
{
	assert(m_DriverCache != nullptr);

	auto data = m_DriverCache.getData();

	RageFile file;
	if (!file.Open(std::string(CacheName), RageFile::WRITE)) {
		throw std::runtime_error(file.GetError());
	}

	file.Write(data.data(), data.size());
}

void
PipelineCache::ReloadPipelines()
{
	assert(m_Device != nullptr);
	m_Device->waitIdle();

	for (auto& pipeline : m_Pipelines) {
		CreateGraphicsPipeline(
		  pipeline.VertexShaderPath, pipeline.FragmentShaderPath, true);
	}
}

intptr_t
PipelineCache::CreateGraphicsPipeline(const std::string& vertexShaderPath,
									  const std::string& fragmentShaderPath,
									  bool reload)
{
	assert(m_Device != nullptr);
	assert(m_TextureLayout != nullptr);
	assert(m_DescriptorSetLayout != nullptr);

	if (!reload) {
		auto previousPipeline =
		  m_PipelineLookup.find({ vertexShaderPath, fragmentShaderPath });

		if (previousPipeline != m_PipelineLookup.end()) {
			return previousPipeline->second;
		} else {
			m_PipelineLookup[{ vertexShaderPath, fragmentShaderPath }] =
			  m_Pipelines.size();
		}
	}

	PipelineInfo info = {};
	info.VertexShaderPath = vertexShaderPath;
	info.FragmentShaderPath = fragmentShaderPath;

	auto fragmentShader =
	  LoadShaderFromFile(fragmentShaderPath, *m_Device, ShaderType_Fragment);
	auto vertexShader =
	  LoadShaderFromFile(vertexShaderPath, *m_Device, ShaderType_Vertex);

	vk::PipelineShaderStageCreateInfo vertexShaderStageInfo{};
	vertexShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertexShaderStageInfo.module = vertexShader,
	vertexShaderStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo fragmentShaderStageInfo{};
	fragmentShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragmentShaderStageInfo.module = fragmentShader,
	fragmentShaderStageInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo shaderStages[] = {
		vertexShaderStageInfo, fragmentShaderStageInfo
	};

	std::vector dynamicStates = { vk::DynamicState::eViewport,
								  vk::DynamicState::eScissor,
								  vk::DynamicState::eDepthWriteEnable,
								  vk::DynamicState::eDepthTestEnable,
								  vk::DynamicState::eDepthCompareOp,
								  vk::DynamicState::eColorBlendEnableEXT,
								  vk::DynamicState::eColorBlendEquationEXT,
								  vk::DynamicState::eColorWriteMaskEXT };

	vk::PipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.dynamicStateCount =
	  static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

	vk::PipelineViewportStateCreateInfo viewportState({}, 1, {}, 1);

	vk::PipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.depthClampEnable = vk::False;
	rasterizer.rasterizerDiscardEnable = vk::False;
	rasterizer.polygonMode = vk::PolygonMode::eFill;
	rasterizer.cullMode = vk::CullModeFlagBits::eBack;
	rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizer.depthBiasEnable = vk::False;
	rasterizer.depthBiasSlopeFactor = 1.0f;
	rasterizer.lineWidth = 1.0f;

	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
	multisampling.sampleShadingEnable = vk::False;

	std::array<vk::PushConstantRange, 1> pushConstants = {};
	pushConstants[0].size = sizeof(uint64_t) * 2;
	pushConstants[0].stageFlags =
	  vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;

	vk::DescriptorSetLayout layouts[] = { m_DescriptorSetLayout,
										  m_TextureLayout };

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.setSetLayouts(layouts);
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

	info.PipelineLayout =
	  vk::raii::PipelineLayout(*m_Device, pipelineLayoutInfo);

	vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.colorWriteMask =
	  vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	  vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	vk::PipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = vk::CompareOp::eLessOrEqual;

	vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &m_ImageFormat;
	pipelineRenderingCreateInfo.depthAttachmentFormat = m_DepthFormat;

	// we don't actually need any vertex info since we're reading stuffs from
	// the storage buffer
	vk::PipelineVertexInputStateCreateInfo vertexInfo = {};

	vk::GraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.pNext = &pipelineRenderingCreateInfo;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pVertexInputState = &vertexInfo;
	pipelineInfo.layout = info.PipelineLayout;
	pipelineInfo.renderPass = nullptr;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;

	info.GraphicsPipeline =
	  vk::raii::Pipeline(*m_Device, m_DriverCache, pipelineInfo);

	if (reload) {
		int index = -1;
		for (int i = 0; auto& pipeline : m_Pipelines) {
			if (pipeline.VertexShaderPath == vertexShaderPath &&
				pipeline.FragmentShaderPath == fragmentShaderPath) {
				index = i;
				break;
			}

			i++;
		}

		assert(index != -1 && "Only existing pipelines should be refreshed");

		m_Pipelines[index] = std::move(info);
		return index;
	}

	m_Pipelines.push_back(std::move(info));
	return static_cast<intptr_t>(m_Pipelines.size() - 1);
}
