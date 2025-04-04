/*
Copyright(c) 2016-2025 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//= INCLUDES ================================
#include "pch.h"
#include "Mesh.h"
#include "../RHI/RHI_Buffer.h"
#include "../World/Entity.h"
#include "../IO/FileStream.h"
#include "../Resource/Import/ModelImporter.h"
#include "../Geometry/GeometryProcessing.h"
//===========================================

//= NAMESPACES ================
using namespace std;
using namespace spartan::math;
//=============================

namespace spartan
{
    Mesh::Mesh() : IResource(ResourceType::Mesh)
    {
        m_flags = GetDefaultFlags();
    }

    Mesh::~Mesh()
    {
        m_index_buffer  = nullptr;
        m_vertex_buffer = nullptr;
    }

    void Mesh::Clear()
    {
        m_indices.clear();
        m_indices.shrink_to_fit();

        m_vertices.clear();
        m_vertices.shrink_to_fit();
    }

    void Mesh::LoadFromFile(const string& file_path)
    {
        const Stopwatch timer;

        if (file_path.empty() || FileSystem::IsDirectory(file_path))
        {
            SP_LOG_WARNING("Invalid file path");
            return;
        }

        // load engine format
        if (FileSystem::GetExtensionFromFilePath(file_path) == EXTENSION_MODEL)
        {
            // deserialize
            auto file = make_unique<FileStream>(file_path, FileStream_Read);
            if (!file->IsOpen())
                return;

            SetResourceFilePath(file->ReadAs<string>());
            file->Read(&m_indices);
            file->Read(&m_vertices);

            CreateGpuBuffers();
        }
        // load foreign format
        else
        {
            SetResourceFilePath(file_path);
            ModelImporter::Load(this, file_path);
        }

        // compute memory usage
        {
            if (m_vertex_buffer && m_index_buffer)
            {
                m_object_size = m_vertex_buffer->GetObjectSize();
                m_object_size += m_index_buffer->GetObjectSize();
            }
        }

        SP_LOG_INFO("Loading \"%s\" took %d ms", FileSystem::GetFileNameFromFilePath(file_path).c_str(), static_cast<int>(timer.GetElapsedTimeMs()));
    }

    void Mesh::SaveToFile(const string& file_path)
    {
        auto file = make_unique<FileStream>(file_path, FileStream_Write);
        if (!file->IsOpen())
            return;

        file->Write(GetResourceFilePath());
        file->Write(m_indices);
        file->Write(m_vertices);

        file->Close();
    }

    uint32_t Mesh::GetMemoryUsage() const
    {
        uint32_t size  = 0;
        size          += uint32_t(m_indices.size()  * sizeof(uint32_t));
        size          += uint32_t(m_vertices.size() * sizeof(RHI_Vertex_PosTexNorTan));

        return size;
    }

    void Mesh::GetGeometry(uint32_t sub_mesh_index, vector<uint32_t>* indices, vector<RHI_Vertex_PosTexNorTan>* vertices)
    {
        SP_ASSERT_MSG(indices != nullptr || vertices != nullptr, "Indices and vertices vectors can't both be null");

        const MeshLod& lod = GetSubMesh(sub_mesh_index).lods[0];

        if (indices)
        {
            SP_ASSERT_MSG(lod.index_count != 0, "Index count can't be 0");

            const auto index_first = m_indices.begin() + lod.index_offset;
            const auto index_last  = m_indices.begin() + lod.index_offset + lod.index_count;
            *indices               = vector<uint32_t>(index_first, index_last);
        }

        if (vertices)
        {
            SP_ASSERT_MSG(lod.vertex_count != 0, "Index count can't be 0");

            const auto vertex_first = m_vertices.begin() + lod.vertex_offset;
            const auto vertex_last  = m_vertices.begin() + lod.vertex_offset + lod.vertex_count;
            *vertices               = vector<RHI_Vertex_PosTexNorTan>(vertex_first, vertex_last);
        }
    }

    void Mesh::AddLod(vector<RHI_Vertex_PosTexNorTan>& vertices, vector<uint32_t>& indices, const uint32_t sub_mesh_index)
    {
        lock_guard lock(m_mutex);
        SP_ASSERT(sub_mesh_index < m_sub_meshes.size());

        // build lod
        MeshLod lod;
        lod.vertex_offset = static_cast<uint32_t>(m_vertices.size());
        lod.vertex_count  = static_cast<uint32_t>(vertices.size());
        lod.index_offset  = static_cast<uint32_t>(m_indices.size());
        lod.index_count   = static_cast<uint32_t>(indices.size());
        lod.aabb          = BoundingBox(vertices.data(), static_cast<uint32_t>(vertices.size()));
    
        // append geometry to mesh buffers
        m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
        m_indices.insert(m_indices.end(), indices.begin(), indices.end());

        // add lod to the specified sub-mesh
        m_sub_meshes[sub_mesh_index].lods.push_back(lod);
    }

    void Mesh::AddGeometry(vector<RHI_Vertex_PosTexNorTan>& vertices, vector<uint32_t>& indices, const bool generate_lods, uint32_t* sub_mesh_index)
    {
        // create a sub-mesh
        SubMesh sub_mesh;
        uint32_t current_sub_mesh_index = static_cast<uint32_t>(m_sub_meshes.size());
        m_sub_meshes.push_back(sub_mesh); // add it to the list so addlod() can access it
    
        // lod 0: original geometry
        {
            // optimize original geometry if flagged
            if (m_flags & static_cast<uint32_t>(MeshFlags::PostProcessOptimize))
            {
                geometry_processing::optimize(vertices, indices);
            }

            // add the original geometry as lod 0
            AddLod(vertices, indices, current_sub_mesh_index);
        }
    
        // generate additional lods if requested
        if (generate_lods && !(m_flags & static_cast<uint32_t>(MeshFlags::PostProcessDontGenerateLods)))
        {
            // store the original index count
            size_t original_index_count = indices.size();
    
            // start with the original geometry for lod 1 onwards
            vector<RHI_Vertex_PosTexNorTan> prev_vertices = vertices;
            vector<uint32_t> prev_indices                 = indices;
    
            for (uint32_t lod_level = 1; lod_level < mesh_lod_count; lod_level++)
            {
                // use the previous lod's geometry for simplification
                vector<RHI_Vertex_PosTexNorTan> lod_vertices = prev_vertices;
                vector<uint32_t> lod_indices                 = prev_indices;
    
                // only simplify if the geometry is complex enough, this prevents collapsing into nothing
                if (lod_indices.size() > 64)
                {
                    // compute target index count based on original index count
                    float t = static_cast<float>(lod_level) / static_cast<float>(mesh_lod_count);
                    if (m_lod_dropoff == MeshLodDropoff::Exponential)
                    {
                        t = pow(t, 2.0f);
                    }
                    float target_fraction     = 1.0f - t;
                    size_t target_index_count = max(static_cast<size_t>(3), static_cast<size_t>(original_index_count * target_fraction));
    
                    // simplify geometry
                    geometry_processing::simplify(lod_indices, lod_vertices, target_index_count);
    
                    // add the simplified geometry as a new lod
                    AddLod(lod_vertices, lod_indices, current_sub_mesh_index);
    
                    // update previous geometry for the next iteration
                    prev_vertices = move(lod_vertices);
                    prev_indices  = move(lod_indices);
                }
                else
                {
                    // if too simple to simplify further, stop generating lods
                    break;
                }
            }
        }
    
        // return the sub-mesh index if requested
        if (sub_mesh_index)
        {
            *sub_mesh_index = current_sub_mesh_index;
        }
    }

    uint32_t Mesh::GetVertexCount() const
    {
        return static_cast<uint32_t>(m_vertices.size());
    }

    uint32_t Mesh::GetIndexCount() const
    {
        return static_cast<uint32_t>(m_indices.size());
    }

    uint32_t Mesh::GetDefaultFlags()
    {
        return
            static_cast<uint32_t>(MeshFlags::ImportRemoveRedundantData) |
            static_cast<uint32_t>(MeshFlags::PostProcessNormalizeScale) |
            static_cast<uint32_t>(MeshFlags::PostProcessOptimize);
    }

    void Mesh::CreateGpuBuffers()
    {
        m_vertex_buffer = make_shared<RHI_Buffer>(RHI_Buffer_Type::Vertex,
            sizeof(m_vertices[0]),
            static_cast<uint32_t>(m_vertices.size()),
            static_cast<void*>(&m_vertices[0]),
            false,
            (string("mesh_vertex_buffer_") + m_object_name).c_str()
        );

        m_index_buffer = make_shared<RHI_Buffer>(RHI_Buffer_Type::Index,
            sizeof(m_indices[0]),
            static_cast<uint32_t>(m_indices.size()),
            static_cast<void*>(&m_indices[0]),
            false,
            (string("mesh_index_buffer_") + m_object_name).c_str()
        );

        // normalize scale
        if (m_flags & static_cast<uint32_t>(MeshFlags::PostProcessNormalizeScale))
        {
            if (shared_ptr<Entity> entity = m_root_entity.lock())
            {
                BoundingBox bounding_box(m_vertices.data(), static_cast<uint32_t>(m_vertices.size()));
                float scale_offset     = bounding_box.GetExtents().Length();
                float normalized_scale = 1.0f / scale_offset;
                entity->SetScale(normalized_scale);
            }
        }
    }
}
