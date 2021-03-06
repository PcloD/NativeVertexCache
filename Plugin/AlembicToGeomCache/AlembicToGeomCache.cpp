#include "Plugin/PrecompiledHeader.h"
#include "./AlembicToGeomCache.h"

namespace nvcabc {

MeshSegment::MeshSegment(ImportContext * ctx, aiPolyMesh * abc)
    : m_ctx(ctx)
    , m_mesh(abc)
{
    aiPolyMeshGetSummary(m_mesh, &m_summary);
}

void MeshSegment::onUpdateSample()
{
    aiSchemaSync(m_mesh);

    auto *sample = aiSchemaGetSample(m_mesh);
    aiPolyMeshGetSampleSummary(sample, &m_sample_summary);

    m_submeshes.resize(m_sample_summary.submesh_count);
    m_submesh_summaries.resize(m_sample_summary.submesh_count);
    aiPolyMeshGetSubmeshSummaries(sample, m_submesh_summaries.data());
}

void MeshSegment::getCounts(size_t& icount, size_t& vcount, size_t& smcount)
{
    icount = m_sample_summary.index_count;
    vcount = m_sample_summary.vertex_count;
    smcount = m_submeshes.size();
}

void MeshSegment::setOffsets(size_t ioffset, size_t voffset, size_t smoffset)
{
    index_offset = ioffset;
    vertex_offset = voffset;
    subm_offset = smoffset;
}

void MeshSegment::fillVertexBuffersBegin()
{
    const nvc::float2 zero2{ 0.0f, 0.0f };
    const nvc::float3 zero3{ 0.0f, 0.0f, 0.0f };
    const nvc::float4 zero4{ 0.0f, 0.0f, 0.0f, 0.0f };

    int vcount = m_sample_summary.vertex_count;
    int voffset = (int)vertex_offset;

    m_vdata.indices = &m_ctx->m_indices[index_offset];
    m_vdata.points = (abcV3*)&m_ctx->m_points[voffset];

    // velocities
    if (m_ctx->m_id_velocities != -1) {
        if (m_summary.has_velocities)
            m_vdata.velocities = (abcV3*)&m_ctx->m_velocities[voffset];
        else
            std::fill(&m_ctx->m_velocities[voffset], &m_ctx->m_velocities[voffset] + vcount, zero3);
    }

    // normals
    if (m_ctx->m_id_normals != -1) {
        if (m_summary.has_normals)
            m_vdata.normals = (abcV3*)&m_ctx->m_normals[voffset];
        else
            std::fill(&m_ctx->m_normals[voffset], &m_ctx->m_normals[voffset] + vcount, zero3);
    }

    // tangents
    if (m_ctx->m_id_tangents != -1) {
        if (m_summary.has_tangents)
            m_vdata.tangents = (abcV4*)&m_ctx->m_tangents[voffset];
        else
            std::fill(&m_ctx->m_tangents[voffset], &m_ctx->m_tangents[voffset] + vcount, zero4);
    }

    // uv0
    if (m_ctx->m_id_uv0 != -1) {
        if (m_summary.has_uv0)
            m_vdata.uv0 = (abcV2*)&m_ctx->m_uv0[voffset];
        else
            std::fill(&m_ctx->m_uv0[voffset], &m_ctx->m_uv0[voffset] + vcount, zero2);
    }

    // uv1
    if (m_ctx->m_id_uv1 != -1) {
        if (m_summary.has_uv1)
            m_vdata.uv1 = (abcV2*)&m_ctx->m_uv1[voffset];
        else
            std::fill(&m_ctx->m_uv1[voffset], &m_ctx->m_uv1[voffset] + vcount, zero2);
    }

    // colors
    if (m_ctx->m_id_colors != -1) {
        if (m_summary.has_colors)
            m_vdata.colors = (abcV4*)&m_ctx->m_colors[voffset];
        else
            std::fill(&m_ctx->m_colors[voffset], &m_ctx->m_colors[voffset] + vcount, zero4);
    }

    size_t sm_offset = 0;
    for (size_t smi = 0; smi < m_submeshes.size(); ++smi) {
        m_submeshes[smi].indices = m_vdata.indices + sm_offset;
        sm_offset += m_submesh_summaries[smi].index_count;
    }

    aiPolyMeshFillVertexBuffer(aiSchemaGetSample(m_mesh), &m_vdata, m_submeshes.data());
}

void MeshSegment::fillVertexBuffersEnd()
{
    aiSampleSync(aiSchemaGetSample(m_mesh));
}



ImportContext::ImportContext()
{
}

ImportContext::~ImportContext()
{
    clear();
}

void ImportContext::clear()
{
    if (m_igcconst) {
        nvcIGCReleaseConstantData(m_igcconst);
        m_igcconst = nullptr;
    }
    if (m_ctx) {
        aiContextDestroy(m_ctx);
        m_ctx = nullptr;
    }
}

bool ImportContext::open(const char * path_to_abc, const ImportOptions & opt)
{
    clear();

    aiContext* ctx = aiContextCreate(1);
    aiContextSetConfig(ctx, (const aiConfig*)&opt);
    if (!aiContextLoad(ctx, path_to_abc)) {
        aiContextDestroy(ctx);
        return false;
    }
    m_ctx = ctx;
    return true;
}

void ImportContext::gatherTimes()
{
    // unify all time samplings
    int num_timesamplings = aiContextGetTimeSamplingCount(m_ctx);
    for (int tsi = 1; tsi < num_timesamplings; ++tsi) {
        auto *ts = aiContextGetTimeSampling(m_ctx, tsi);
        auto num_samples = aiTimeSamplingGetSampleCount(ts);
        for (int si = 0; si < num_samples; ++si) {
            auto t = aiTimeSamplingGetTime(ts, si);
            m_timesamples.push_back(t);
        }
    }
    std::sort(m_timesamples.begin(), m_timesamples.end());
    m_timesamples.erase(
        std::unique(m_timesamples.begin(), m_timesamples.end()),
        m_timesamples.end());
}


void ImportContext::gatherMeshes(aiObject * obj)
{
    if (!obj)
        return;

    m_abc_nodes.push_back(obj);

    if (aiPolyMesh *poly = aiObjectAsPolyMesh(obj)) {
        m_segments.push_back(MeshSegment(this, poly));
        nvcIGCAddConstantDataString(m_igcconst, aiObjectGetFullName(obj));

        const auto& summary = m_segments.back().m_summary;
        if (m_id_points == -1) {
            m_id_points = (int)m_descs.size();
            m_descs.push_back({ nvcSEMANTIC_POINTS, nvc::DataFormat::Float3 });
        }
        if (summary.has_velocities && m_id_velocities == -1) {
            m_id_velocities = (int)m_descs.size();
            m_descs.push_back({ nvcSEMANTIC_VELOCITIES, nvc::DataFormat::Float3 });
        }
        if (summary.has_normals && m_id_normals == -1) {
            m_id_normals = (int)m_descs.size();
            m_descs.push_back({ nvcSEMANTIC_NORMALS, nvc::DataFormat::Float3 });
        }
        if (summary.has_tangents && m_id_tangents == -1) {
            m_id_tangents = (int)m_descs.size();
            m_descs.push_back({ nvcSEMANTIC_TANGENTS, nvc::DataFormat::Float4 });
        }
        if (summary.has_uv0 && m_id_uv0 == -1) {
            m_id_uv0 = (int)m_descs.size();
            m_descs.push_back({ nvcSEMANTIC_UV0, nvc::DataFormat::Float2 });
        }
        if (summary.has_uv1 && m_id_uv1 == -1) {
            m_id_uv1 = (int)m_descs.size();
            m_descs.push_back({ nvcSEMANTIC_UV1, nvc::DataFormat::Float2 });
        }
        if (summary.has_colors && m_id_colors == -1) {
            m_id_colors = (int)m_descs.size();
            m_descs.push_back({ nvcSEMANTIC_COLORS, nvc::DataFormat::Float4 });
        }
    }

    // process children
    int num_children = aiObjectGetNumChildren(obj);
    for (int ci = 0; ci < num_children; ++ci)
        gatherMeshes(aiObjectGetChild(obj, ci));
}

void ImportContext::gatherMeshes()
{
    m_igcconst = nvcIGCCreateConstantData();

    gatherMeshes(aiContextGetTopObject(m_ctx));
    m_descs.push_back(GEOM_CACHE_DESCRIPTOR_END);
}


void ImportContext::gatherSamples(double time, nvc::InputGeomCache *igc)
{
    aiContextUpdateSamples(m_ctx, time);

    m_geomeshes.clear();
    m_geosubmeshes.clear();

    size_t index_offset = 0, index_count = 0;
    size_t vertex_offset = 0, vertex_count = 0;
    size_t subm_offset = 0, subm_count = 0;
    for (auto& seg : m_segments) {
        seg.onUpdateSample();

        seg.setOffsets(index_offset, vertex_offset, subm_offset);
        seg.getCounts(index_count, vertex_count, subm_count);

        {
            GeomMesh gm;
            gm.submeshOffset = (uint32_t)m_geosubmeshes.size();
            gm.submeshCount = (uint32_t)seg.m_submesh_summaries.size();
            gm.vertexOffset = (uint32_t)vertex_offset;
            gm.vertexCount = (uint32_t)vertex_count;
            m_geomeshes.push_back(gm);
        }
        {
            GeomSubmesh gsm;
            gsm.indexOffset = (uint32_t)index_offset;
            for (auto& sm : seg.m_submesh_summaries) {
                gsm.indexCount = sm.index_count;
                gsm.topology = (nvc::Topology)sm.topology;
                m_geosubmeshes.push_back(gsm);

                gsm.indexOffset += sm.index_count;
            }
        }

        index_offset += index_count;
        vertex_offset += vertex_count;
        subm_offset += subm_count;
    }

    m_indices.resize(index_offset);

    m_data_pointers.resize(m_descs.size());
    {
        m_points.resize(vertex_offset);
        m_data_pointers[m_id_points] = m_points.data();
    }
    if (m_id_velocities != -1) {
        m_velocities.resize(vertex_offset);
        m_data_pointers[m_id_velocities] = m_velocities.data();
    }
    if (m_id_normals != -1) {
        m_normals.resize(vertex_offset);
        m_data_pointers[m_id_normals] = m_normals.data();
    }
    if (m_id_tangents != -1) {
        m_tangents.resize(vertex_offset);
        m_data_pointers[m_id_tangents] = m_tangents.data();
    }
    if (m_id_uv0 != -1) {
        m_uv0.resize(vertex_offset);
        m_data_pointers[m_id_uv0] = m_uv0.data();
    }
    if (m_id_uv1 != -1) {
        m_uv1.resize(vertex_offset);
        m_data_pointers[m_id_uv1] = m_uv1.data();
    }
    if (m_id_colors != -1) {
        m_colors.resize(vertex_offset);
        m_data_pointers[m_id_colors] = m_colors.data();
    }

    for (auto& seg : m_segments)
        seg.fillVertexBuffersBegin();

    for (auto& seg : m_segments)
        seg.fillVertexBuffersEnd();

    nvc::GeomCacheData odata;
    odata.indexCount = m_indices.size();
    odata.indices = m_indices.data();
    odata.vertexCount = m_points.size();
    odata.vertices = m_data_pointers.data();
    odata.meshCount = m_geomeshes.size();
    odata.meshes = m_geomeshes.data();
    odata.submeshCount = m_geosubmeshes.size();
    odata.submeshes = m_geosubmeshes.data();
    nvcIGCAddData(igc, (float)time, &odata);
}

InputGeomCache* ImportContext::gatherSamples()
{
    InputGeomCache *ret = nvcIGCCreate(m_descs.data(), m_igcconst);
    for (auto ts : m_timesamples) {
        gatherSamples(ts, ret);
    }
    return ret;
}

} // namespace nvcabc
