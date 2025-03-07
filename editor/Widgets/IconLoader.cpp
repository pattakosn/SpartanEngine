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

//= INCLUDES ======================
#include "IconLoader.h"
#include "Resource/ResourceCache.h"
#include "RHI/RHI_Texture.h"
#include "Core/ThreadPool.h"
#include "Event.h"
//=================================

//= NAMESPACES =========
using namespace std;
using namespace spartan;
//======================

namespace
{
    vector<shared_ptr<Icon>> icons;
    Icon no_icon;
    mutex icon_mutex;

    void destroy_rhi_resources()
    {
        icons.clear();
    }

    Icon* get_icon_by_type(IconType type)
    {
        for (auto& icon : icons)
        {
            if (icon->GetType() == type)
                return icon.get();
        }

        return &no_icon;
    }
}

Icon::Icon(IconType type, const string& file_path)
{
    m_type = type;

    // load texture
    ThreadPool::AddTask([this, file_path]()
    {
        m_texture = make_shared<RHI_Texture>(file_path);
    });
}

RHI_Texture* Icon::GetTexture() const
{
    if (m_texture && m_texture->GetResourceState() == ResourceState::PreparedForGpu)
    {
        return m_texture.get();
    }

    return nullptr;
}

void Icon::SetTexture(shared_ptr<RHI_Texture> texture)
{
    m_texture = texture;
}

string Icon::GetFilePath() const
{
    if (!m_texture)
        return "";

    return m_texture->GetResourceFilePath();
}

void IconLoader::Initialize()
{
    SP_SUBSCRIBE_TO_EVENT(EventType::RendererOnShutdown, SP_EVENT_HANDLER_STATIC(destroy_rhi_resources));

    const string data_dir = ResourceCache::GetDataDirectory() + "/";

    IconLoader::LoadFromFile(data_dir + "icons/component_audioListener.png"         , IconType::Component_AudioListener);
    IconLoader::LoadFromFile(data_dir + "icons/component_audioSource.png"           , IconType::Component_AudioSource);
    IconLoader::LoadFromFile(data_dir + "icons/component_camera.png"                , IconType::Component_Camera);
    IconLoader::LoadFromFile(data_dir + "icons/component_light.png"                 , IconType::Component_Light);
    IconLoader::LoadFromFile(data_dir + "icons/component_material.png"              , IconType::Component_Material);
    IconLoader::LoadFromFile(data_dir + "icons/component_material_removeTexture.png", IconType::Component_Material_RemoveTexture);
    IconLoader::LoadFromFile(data_dir + "icons/component_meshCollider.png"          , IconType::Component_MeshCollider);
    IconLoader::LoadFromFile(data_dir + "icons/component_renderable.png"            , IconType::Component_Renderable);
    IconLoader::LoadFromFile(data_dir + "icons/component_rigidBody.png"             , IconType::Component_PhysicsBody);
    IconLoader::LoadFromFile(data_dir + "icons/component_softBody.png"              , IconType::Component_SoftBody);
    IconLoader::LoadFromFile(data_dir + "icons/component_transform.png"             , IconType::Component_Transform);
    IconLoader::LoadFromFile(data_dir + "icons/component_terrain.png"               , IconType::Component_Terrain);
    IconLoader::LoadFromFile(data_dir + "icons/component_environment.png"           , IconType::Component_Environment);
    IconLoader::LoadFromFile(data_dir + "icons/console.png"                         , IconType::Console);
    IconLoader::LoadFromFile(data_dir + "icons/file.png"                            , IconType::Directory_File_Default);
    IconLoader::LoadFromFile(data_dir + "icons/folder.png"                          , IconType::Directory_Folder);
    IconLoader::LoadFromFile(data_dir + "icons/audio.png"                           , IconType::Directory_File_Audio);
    IconLoader::LoadFromFile(data_dir + "icons/model.png"                           , IconType::Directory_File_Model);
    IconLoader::LoadFromFile(data_dir + "icons/world.png"                           , IconType::Directory_File_World);
    IconLoader::LoadFromFile(data_dir + "icons/material.png"                        , IconType::Directory_File_Material);
    IconLoader::LoadFromFile(data_dir + "icons/shader.png"                          , IconType::Directory_File_Shader);
    IconLoader::LoadFromFile(data_dir + "icons/xml.png"                             , IconType::Directory_File_Xml);
    IconLoader::LoadFromFile(data_dir + "icons/dll.png"                             , IconType::Directory_File_Dll);
    IconLoader::LoadFromFile(data_dir + "icons/txt.png"                             , IconType::Directory_File_Txt);
    IconLoader::LoadFromFile(data_dir + "icons/ini.png"                             , IconType::Directory_File_Ini);
    IconLoader::LoadFromFile(data_dir + "icons/exe.png"                             , IconType::Directory_File_Exe);
    IconLoader::LoadFromFile(data_dir + "icons/font.png"                            , IconType::Directory_File_Font);
    IconLoader::LoadFromFile(data_dir + "icons/screenshot.png"                      , IconType::Screenshot);
    IconLoader::LoadFromFile(data_dir + "icons/settings.png"                        , IconType::Component_Options);
    IconLoader::LoadFromFile(data_dir + "icons/play.png"                            , IconType::Button_Play);
    IconLoader::LoadFromFile(data_dir + "icons/timer.png"                           , IconType::Button_Profiler);
    IconLoader::LoadFromFile(data_dir + "icons/resource_viewer.png"                 , IconType::Button_ResourceCache);
    IconLoader::LoadFromFile(data_dir + "icons/capture.png"                         , IconType::Button_RenderDoc);
    IconLoader::LoadFromFile(data_dir + "icons/code.png"                            , IconType::Button_Shader);
    IconLoader::LoadFromFile(data_dir + "icons/texture.png"                         , IconType::Directory_File_Texture);
    IconLoader::LoadFromFile(data_dir + "icons/window_minimise.png"                 , IconType::Window_Minimize);
    IconLoader::LoadFromFile(data_dir + "icons/window_maximise.png"                 , IconType::Window_Maximize);
    IconLoader::LoadFromFile(data_dir + "icons/window_close.png"                    , IconType::Window_Close);
}

RHI_Texture* IconLoader::GetTextureByType(IconType type)
{
    return LoadFromFile("", type)->GetTexture();
}

Icon* IconLoader::LoadFromFile(const string& file_path, IconType type /*Undefined*/)
{
    // check if the texture is already loaded, and return that
    bool search_by_type = type != IconType::Undefined;
    for (auto& icon : icons)
    {
        if (search_by_type)
        {
            if (icon->GetType() == type)
                return icon.get();
        }
        else if (icon->GetFilePath() == file_path)
        {
            return icon.get();
        }
    }

    // the texture is new so load it
    if (FileSystem::IsSupportedImageFile(file_path) || FileSystem::IsEngineTextureFile(file_path))
    {
        lock_guard<mutex> guard(icon_mutex);

        // add a new icon
        icons.push_back(make_shared<Icon>(type, file_path));

        // return it
        return icons.back().get();
    }

    return get_icon_by_type(IconType::Directory_File_Default);
}
