#include "Plugin/PrecompiledHeader.h"
#include "Plugin/InputGeomCache.h"
#include "Plugin/OutputGeomCache.h"
#include "Plugin/GeomCache.h"
#include "Plugin/nvcAPI.h"


nvcAPI nvc::InputGeomCache* nvcIGCCreate(const nvc::GeomCacheDesc *descs)
{
    return new nvc::InputGeomCache(descs);
}
nvcAPI void nvcIGCRelease(nvc::InputGeomCache *self)
{
    delete self;
}
nvcAPI void nvcIGCAddData(nvc::InputGeomCache * self, float time, const nvc::GeomCacheData * data)
{
    if (self) {
        self->addData(time, data);
    }
}

nvcAPI nvc::OutputGeomCache* nvcOGCCreate()
{
    return new nvc::OutputGeomCache();
}

nvcAPI void nvcOGCRelease(nvc::OutputGeomCache *self)
{
    delete self;
}

nvcAPI int nvcOGCGetVertexCount(nvc::OutputGeomCache *self)
{
    return self ? (int)self->points.size() : 0;
}

nvcAPI int nvcOGCGetIndexCount(nvc::OutputGeomCache *self)
{
    return self ? (int)self->indices.size() : 0;
}

nvcAPI int nvcOGCCopyIndices(nvc::OutputGeomCache *self, const nvc::GeomSubmesh *gsm, int *dst)
{
    if (self) {
        return self->copyIndices(*gsm, dst);
    }
    return false;
}

nvcAPI int nvcOGCCopyPoints(nvc::OutputGeomCache *self, const nvc::GeomMesh *gm, nvc::float3 *dst)
{
    if (self) {
        return self->copyPoints(*gm, dst);
    }
    return false;
}

nvcAPI int nvcOGCCopyNormals(nvc::OutputGeomCache *self, const nvc::GeomMesh *gm, nvc::float3 *dst)
{
    if (self) {
        return self->copyNormals(*gm, dst);
    }
    return false;
}

nvcAPI int nvcOGCCopyTangents(nvc::OutputGeomCache *self, const nvc::GeomMesh *gm, nvc::float4 *dst)
{
    if (self) {
        return self->copyTangents(*gm, dst);
    }
    return false;
}

nvcAPI int nvcOGCCopyUV0(nvc::OutputGeomCache *self, const nvc::GeomMesh *gm, nvc::float2 *dst)
{
    if (self) {
        return self->copyUV0(*gm, dst);
    }
    return false;
}

nvcAPI int nvcOGCCopyUV1(nvc::OutputGeomCache *self, const nvc::GeomMesh *gm, nvc::float2 *dst)
{
    if (self) {
        return self->copyUV1(*gm, dst);
    }
    return false;
}

nvcAPI int nvcOGCCopyColors(nvc::OutputGeomCache *self, const nvc::GeomMesh *gm, nvc::float4 *dst)
{
    if (self) {
        return self->copyColors(*gm, dst);
    }
    return false;
}


nvcAPI nvc::GeomCache* nvcGCCreate()
{
    return new nvc::GeomCache();
}

nvcAPI void nvcGCRelease(nvc::GeomCache *self)
{
    delete self;
}

nvcAPI int nvcGCOpen(nvc::GeomCache *self, const char *path)
{
    if (self) {
        return self->open(path);
    }
    return false;
}

nvcAPI void nvcGCClose(nvc::GeomCache *self)
{
    if (self) {
        self->close();
    }
}

nvcAPI void nvcGCSetCurrentTime(nvc::GeomCache *self, float time)
{
    if (self) {
        self->setCurrentFrame(time);
    }
}

nvcAPI void nvcGCGetCurrentCache(nvc::GeomCache *self, nvc::OutputGeomCache *ogc)
{
    if (self && ogc) {
        self->assignCurrentDataToMesh(*ogc);
    }
}
