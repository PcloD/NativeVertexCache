#pragma once

#include "Plugin/OutputGeomCache.h"
#include "Plugin/GeomCacheData.h"
#include "Plugin/Compression/IDecompressor.h"
#include "Plugin/Stream/FileStream.h"

namespace nvc {

class GeomCache final
{

public:
	GeomCache();
	~GeomCache();

	bool open(const char* nvcFilename);
	bool close();
	bool good() const;

	// Caching
	//  I                                I
	// [X][o][o][o][o][o][o][][][][][][][X][o][o][o][o]][o][o][][][][][][]
	// |-------------------|  skip =>   |-------------------|

	//  I           I                          I
	// [X][o][o][o][o][o][o][][][][][][][][][][X][o][o][o][o]][o][o][][][][][][]
	//              |-------------------|  <= skip
	void preload(float currentTime, float range);

	// Playback.
	void setCurrentFrame(float currentTime);
	// + function to get geometry data to render.

	bool assignCurrentDataToMesh(OutputGeomCache& mesh);


	//// Sampling.
	//template<typename TDataType>
	//TDataType Sample<TDataType>(float time, const char* semantic);

	//...
	GeomCache(const GeomCache&) = delete;
	GeomCache(GeomCache&&) = delete;
	GeomCache& operator=(const GeomCache&) = delete;
	GeomCache& operator=(GeomCache&&) = delete;

protected:
	std::unique_ptr<IDecompressor> m_Decompressor {};
	std::unique_ptr<FileStream> m_InputFileStream {};
	const GeomCacheDesc* m_GeomCacheDescs {};
	size_t m_GeomCacheDescsNum = 0;
//	GeomCacheDesc m_GeomCacheDesc[GEOM_CACHE_MAX_DESCRIPTOR_COUNT + 1] {};

	int m_DescIndex_indices = -1;
	int m_DescIndex_points = -1;
	int m_DescIndex_normals = -1;
	int m_DescIndex_tangents = -1;
	int m_DescIndex_uvs = -1;
	int m_DescIndex_colors = -1;

	float m_CurrentTime = 0.0f;
	size_t m_CurrentFrame = 0;
};

} // namespace nvc
