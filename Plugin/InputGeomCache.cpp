//! PrecompiledHeader Include.
#include "Plugin/PrecompiledHeader.h"

//! Header Include.
#include "InputGeomCache.h"

namespace nvc {

const char* InputGeomCache::VERTEX_ID_SEMANTIC = "VertexId";
const char* InputGeomCache::MESH_ID_SEMANTIC = "MeshId";

InputGeomCache::InputGeomCache(const GeomCacheDesc *desc)
{
	memcpy(m_Descriptor, desc, sizeof(GeomCacheDesc) * getAttributeCount(desc));
}

InputGeomCache::~InputGeomCache()
{
	const size_t attributeCount = getAttributeCount(m_Descriptor);

	auto it = m_Data.begin();
	const auto itEnd = m_Data.end();
	while (it != itEnd)
	{
		freeGeomCacheData(it->second, attributeCount);
		++it;
	}

	m_Data.clear();
}

void InputGeomCache::addData(float time, const GeomCacheData* data)
{
	const auto it = std::find_if(m_Data.begin(), m_Data.end(),
						[time] (const FrameDataType& d)
						{
							return d.first == time;
						});

	if (it == m_Data.end())
	{
		GeomCacheData cacheData = {};
		cacheData.indexCount = data->indexCount;
		cacheData.vertexCount = data->vertexCount;

		if (data->indices)
		{
			const size_t dataSize = sizeof(int) * data->indexCount;

			void *indices = malloc(dataSize);
			if (indices != nullptr)
			{
				memcpy(indices, data->indices, dataSize);
			}

			cacheData.indices = indices;
		}

		if (data->vertices)
		{
			const size_t attributeCount = getAttributeCount(m_Descriptor);
			cacheData.vertices = new const void*[attributeCount];

			for (size_t iAttribute = 0; iAttribute < attributeCount; ++iAttribute)
			{
				const size_t dataSize = getSizeOfDataFormat(m_Descriptor[iAttribute].format) * data->vertexCount;

				void *vertexData = malloc(dataSize);
				if (vertexData != nullptr)
				{
					memcpy(vertexData, data->vertices[iAttribute], dataSize);
				}

				cacheData.vertices[iAttribute] = vertexData;
			}
		}

		// Insert sorted.
		const auto dataToInsert = std::make_pair(time, cacheData);
		const auto itInsert = std::lower_bound(m_Data.begin(), m_Data.end(), dataToInsert,
										[](const FrameDataType& lhs, const FrameDataType& rhs)
										{
											return lhs.first < rhs.first;
										});
		m_Data.insert(itInsert, dataToInsert);
	}
}

void InputGeomCache::getDesc(GeomCacheDesc* desc) const
{
	memcpy(desc, m_Descriptor, sizeof(GeomCacheDesc) * getAttributeCount(desc));
}

void InputGeomCache::getData(size_t frameIndex, float& frameTime, GeomCacheData* data) const
{
	if (frameIndex < getDataCount())
	{
		auto& frameData = m_Data.at(frameIndex);

		frameTime = frameData.first;
		data->indices = frameData.second.indices;
		data->indexCount = frameData.second.indexCount;
		data->vertices = frameData.second.vertices;
		data->vertexCount = frameData.second.vertexCount;
	}
}

size_t InputGeomCache::getDataCount() const
{
	return m_Data.size();
}

int InputGeomCache::getVertexIdIndex() const
{
	const size_t itemCount = getAttributeCount(m_Descriptor);
	for (size_t iItem = 0; iItem < itemCount; ++iItem)
	{
		if (_stricmp(m_Descriptor[iItem].semantic, VERTEX_ID_SEMANTIC) != 0)
		{
			return static_cast<int>(iItem);
		}
	}

	return -1;
}

bool InputGeomCache::hasVertexId() const
{
	return getVertexIdIndex() > 0;
}

int InputGeomCache::getMeshIdIndex() const
{
	const size_t itemCount = getAttributeCount(m_Descriptor);
	for (size_t iItem = 0; iItem < itemCount; ++iItem)
	{
		if (_stricmp(m_Descriptor[iItem].semantic, MESH_ID_SEMANTIC) != 0)
		{
			return static_cast<int>(iItem);
		}
	}

	return -1;
}

bool InputGeomCache::hasMeshId() const
{
	return getMeshIdIndex() > 0;
}

} // namespace nvc
