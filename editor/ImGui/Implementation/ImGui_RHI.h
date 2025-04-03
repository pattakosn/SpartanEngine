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

#pragma once

//= INCLUDES ===========================
#include <array>
#include "../Source/imgui.h"
#include "../../Widgets/TextureViewer.h"
#include "Core/Event.h"
#include "Rendering/Renderer_Buffers.h"
#include "Rendering/Renderer.h"
#include "Math/Rectangle.h"
#include "RHI/RHI_Device.h"
#include "RHI/RHI_Shader.h"
#include "RHI/RHI_Texture.h"
#include "RHI/RHI_SwapChain.h"
#include "RHI/RHI_BlendState.h"
#include "RHI/RHI_Queue.h"
#include "RHI/RHI_CommandList.h"
#include "RHI/RHI_Buffer.h"
#include "RHI/RHI_PipelineState.h"
#include "RHI/RHI_RasterizerState.h"
#include "RHI/RHI_DepthStencilState.h"
#include <Debugging.h>
SP_WARNINGS_OFF
#include <SDL3/SDL_video.h>
SP_WARNINGS_ON
//======================================

namespace ImGui::RHI
{
    //= NAMESPACES =========
    using namespace spartan;
    using namespace math;
    using namespace std;
    //======================

    namespace
    {
        const uint32_t buffer_count = 8;

        struct ViewportRhiResources
        {
            array<unique_ptr<RHI_Buffer>, buffer_count> index_buffers;
            array<unique_ptr<RHI_Buffer>, buffer_count> vertex_buffers;
            Pcb_Pass push_constant_buffer_pass;
            uint32_t buffer_index = 0;

            ViewportRhiResources() = default;
            ViewportRhiResources(const char* name, RHI_SwapChain* swapchain)
            {
                // allocate buffers
                for (uint32_t i = 0; i < buffer_count; i++)
                {
                    vertex_buffers[i] = make_unique<RHI_Buffer>(RHI_Buffer_Type::Vertex, sizeof(ImDrawVert), 50000, nullptr, true, name);
                    index_buffers[i]  = make_unique<RHI_Buffer>(RHI_Buffer_Type::Index, sizeof(ImDrawIdx), 100000, nullptr, true, name);
                }
            }
        };

        struct WindowData
        {
            shared_ptr<ViewportRhiResources> viewport_rhi_resources;
            shared_ptr<RHI_SwapChain>        swapchain;
        };

        // main window rhi resources
        ViewportRhiResources g_viewport_data;

        // shared rhi resources (between all windows)
        shared_ptr<RHI_Texture>           g_font_atlas;
        shared_ptr<RHI_DepthStencilState> g_depth_stencil_state;
        shared_ptr<RHI_RasterizerState>   g_rasterizer_state;
        shared_ptr<RHI_BlendState>        g_blend_state;
        shared_ptr<RHI_Shader>            g_shader_vertex;
        shared_ptr<RHI_Shader>            g_shader_pixel;
    }

    // forward declarations
    void initialize_platform_interface();

    void destroy_rhi_resources()
    {
        g_font_atlas          = nullptr;
        g_depth_stencil_state = nullptr;
        g_rasterizer_state    = nullptr;
        g_blend_state         = nullptr;
        g_shader_vertex       = nullptr;
        g_shader_pixel        = nullptr;

        for (auto& ptr : g_viewport_data.index_buffers)
        {
            ptr = nullptr;
        }

        for (auto& ptr : g_viewport_data.vertex_buffers)
        {
            ptr = nullptr;
        }
    }

    void Initialize()
    {
        // create required RHI objects
        {
            g_viewport_data       = ViewportRhiResources("imgui", Renderer::GetSwapChain());
            g_depth_stencil_state = make_shared<RHI_DepthStencilState>(false, false, RHI_Comparison_Function::Always);
            g_rasterizer_state    = make_shared<RHI_RasterizerState>(RHI_PolygonMode::Solid, true);

            g_blend_state = make_shared<RHI_BlendState>
            (
                true,
                RHI_Blend::Src_Alpha,     // source blend
                RHI_Blend::Inv_Src_Alpha, // destination blend
                RHI_Blend_Operation::Add, // blend op
                RHI_Blend::Inv_Src_Alpha, // source blend alpha
                RHI_Blend::Zero,          // destination blend alpha
                RHI_Blend_Operation::Add  // destination op alpha
            );

            // compile shaders
            {
                const string shader_path = ResourceCache::GetResourceDirectory(ResourceDirectory::Shaders) + "/imgui.hlsl";

                bool async = false;

                g_shader_vertex = make_shared<RHI_Shader>();
                g_shader_vertex->Compile(RHI_Shader_Type::Vertex, shader_path, async, RHI_Vertex_Type::Pos2dUvCol8);

                g_shader_pixel = make_shared<RHI_Shader>();
                g_shader_pixel->Compile(RHI_Shader_Type::Pixel, shader_path, async);
            }
        }

        // font atlas
        {
            unsigned char* pixels = nullptr;
            int atlas_width       = 0;
            int atlas_height      = 0;
            int bpp               = 0;
            ImGuiIO& io           = GetIO();
            io.Fonts->GetTexDataAsRGBA32(&pixels, &atlas_width, &atlas_height, &bpp);

            // copy pixel data
            vector<RHI_Texture_Slice> texture_data;
            vector<std::byte>& mip = texture_data.emplace_back().mips.emplace_back().bytes;
            const uint32_t size = atlas_width * atlas_height * bpp;
            mip.resize(size);
            mip.reserve(size);
            memcpy(&mip[0], reinterpret_cast<std::byte*>(pixels), size);

            // upload texture to graphics system
            g_font_atlas = make_shared<RHI_Texture>(RHI_Texture_Type::Type2D, atlas_width, atlas_height, 1, 1, RHI_Format::R8G8B8A8_Unorm, RHI_Texture_Srv, "imgui_font_atlas", texture_data);
            io.Fonts->TexID = reinterpret_cast<ImTextureID>(g_font_atlas.get());
        }

        // setup back-end capabilities flags
        ImGuiIO& io             = GetIO();
        io.BackendFlags        |= ImGuiBackendFlags_RendererHasViewports;
        io.BackendFlags        |= ImGuiBackendFlags_RendererHasVtxOffset;
        io.BackendRendererName  = "RHI";
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            initialize_platform_interface();
        }

        SP_SUBSCRIBE_TO_EVENT(EventType::RendererOnShutdown, SP_EVENT_HANDLER_STATIC(destroy_rhi_resources));
    }

    void shutdown()
    {
        DestroyPlatformWindows();
    }

    void render(ImDrawData* draw_data, WindowData* window_data = nullptr, const bool clear = true)
    {
        // get resources
        bool is_main_window                 = window_data == nullptr;
        ViewportRhiResources* rhi_resources = is_main_window ? &g_viewport_data : window_data->viewport_rhi_resources.get();
        RHI_SwapChain* swapchain            = is_main_window ? Renderer::GetSwapChain() : window_data->swapchain.get();
        uint32_t buffer_index               = rhi_resources->buffer_index;
        rhi_resources->buffer_index         = (rhi_resources->buffer_index + 1) % buffer_count;
        RHI_Buffer* vertex_buffer           = rhi_resources->vertex_buffers[buffer_index].get();
        RHI_Buffer* index_buffer            = rhi_resources->index_buffers[buffer_index].get();
        RHI_Queue* queue                    = RHI_Device::GetQueue(RHI_Queue_Type::Graphics);

        // child windows need to be handled here
        if (!is_main_window)
        {
            queue->NextCommandList();
            swapchain->AcquireNextImage();
            queue->GetCommandList()->Begin(queue);
        }

        // for the main window, if the renderer is not ready, return
        RHI_CommandList* cmd_list = queue->GetCommandList();
        if (is_main_window && cmd_list->GetState() != RHI_CommandListState::Recording)
            return;

        // update vertex and index buffers
        {
            // grow vertex buffer as needed
            if (vertex_buffer->GetElementCount() < static_cast<uint32_t>(draw_data->TotalVtxCount))
            {
                const uint32_t count                        = vertex_buffer->GetElementCount();
                const uint32_t count_new                    = draw_data->TotalVtxCount + 15000;
                rhi_resources->vertex_buffers[buffer_index] = make_unique<RHI_Buffer>(RHI_Buffer_Type::Vertex, sizeof(ImDrawVert), count_new, nullptr, true, vertex_buffer->GetObjectName().c_str());

                if (count != 0)
                {
                    SP_LOG_INFO("Vertex buffer has been re-allocated to fit %d vertices", count_new);
                }
            }

            // grow index buffer as needed
            if (index_buffer->GetElementCount() < static_cast<uint32_t>(draw_data->TotalIdxCount))
            {
                const uint32_t count                       = index_buffer->GetElementCount();
                const uint32_t count_new                   = draw_data->TotalIdxCount + 30000;
                rhi_resources->index_buffers[buffer_index] = make_unique<RHI_Buffer>(RHI_Buffer_Type::Index, sizeof(ImDrawIdx), count_new, nullptr, true, index_buffer->GetObjectName().c_str());

                if (count != 0)
                {
                    SP_LOG_INFO("Index buffer has been re-allocated to fit %d indices", count_new);
                }
            }

            // copy all imgui vertices into a single buffer
            ImDrawVert* vtx_dst = static_cast<ImDrawVert*>(vertex_buffer->GetMappedData());
            ImDrawIdx*  idx_dst = static_cast<ImDrawIdx*>(index_buffer->GetMappedData());
            if (vtx_dst && idx_dst)
            {
                for (auto i = 0; i < draw_data->CmdListsCount; i++)
                {
                    const ImDrawList* imgui_cmd_list = draw_data->CmdLists[i];

                    memcpy(vtx_dst, imgui_cmd_list->VtxBuffer.Data, imgui_cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
                    memcpy(idx_dst, imgui_cmd_list->IdxBuffer.Data, imgui_cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));

                    vtx_dst += imgui_cmd_list->VtxBuffer.Size;
                    idx_dst += imgui_cmd_list->IdxBuffer.Size;
                }
            }
        }

        // set pipeline state
        static RHI_PipelineState pso         = {};
        pso.name                             = "imgui";
        pso.shaders[RHI_Shader_Type::Vertex] = g_shader_vertex.get();
        pso.shaders[RHI_Shader_Type::Pixel]  = g_shader_pixel.get();
        pso.rasterizer_state                 = g_rasterizer_state.get();
        pso.blend_state                      = g_blend_state.get();
        pso.depth_stencil_state              = g_depth_stencil_state.get();
        pso.render_target_swapchain          = swapchain;
        pso.clear_color[0]                   = clear ? Color::standard_black : rhi_color_dont_care;

        // start the pass
        const char* name = is_main_window ? "imgui_window_main" : "imgui_window_child";
        bool gpu_timing  = is_main_window;
        cmd_list->BeginTimeblock(name, true, spartan::Debugging::IsGpuTimingEnabled() && gpu_timing);
        cmd_list->SetPipelineState(pso);
        cmd_list->SetBufferVertex(vertex_buffer);
        cmd_list->SetBufferIndex(index_buffer);
        cmd_list->SetCullMode(RHI_CullMode::None);

        // render
        {
            uint32_t global_vtx_offset = 0;
            uint32_t global_idx_offset = 0;
            for (uint32_t i = 0; i < static_cast<uint32_t>(draw_data->CmdListsCount); i++)
            {
                ImDrawList* cmd_list_imgui = draw_data->CmdLists[i];

                for (uint32_t cmd_i = 0; cmd_i < static_cast<uint32_t>(cmd_list_imgui->CmdBuffer.Size); cmd_i++)
                {
                    const ImDrawCmd* pcmd = &cmd_list_imgui->CmdBuffer[cmd_i];

                    if (pcmd->UserCallback != nullptr)
                    {
                        pcmd->UserCallback(cmd_list_imgui, pcmd);
                    }
                    else
                    {
                        // set scissor rectangle
                        {
                            math::Rectangle scissor_rect;
                            scissor_rect.left   = pcmd->ClipRect.x - draw_data->DisplayPos.x;
                            scissor_rect.top    = pcmd->ClipRect.y - draw_data->DisplayPos.y;
                            scissor_rect.right  = pcmd->ClipRect.z - draw_data->DisplayPos.x;
                            scissor_rect.bottom = pcmd->ClipRect.w - draw_data->DisplayPos.y;

                            cmd_list->SetScissorRectangle(scissor_rect);
                        }

                        // push pass/draw call constants
                        {
                            // set texture and update texture viewer parameters
                            {
                                bool m_channel_r           = false;
                                bool m_channel_g           = false;
                                bool m_channel_b           = false;
                                bool m_channel_a           = false;
                                bool m_gamma_correct       = false;
                                bool m_pack                = false;
                                bool m_boost               = false;
                                bool m_abs                 = false;
                                bool m_point_sampling      = false;
                                float mip_and_array_packed = 0.0f;
                                bool is_texture_visualised = false;
                                bool is_frame_texture      = false;

                                if (RHI_Texture* texture = reinterpret_cast<RHI_Texture*>(pcmd->TextureId))
                                {
                                    is_frame_texture = Renderer::GetRenderTarget(Renderer_RenderTarget::frame_output)->GetObjectId() == texture->GetObjectId();

                                    // during engine startup, some textures might be loading in different threads
                                    if (texture->GetResourceState() == ResourceState::PreparedForGpu)
                                    {
                                        // update texture viewer parameters
                                        is_texture_visualised = TextureViewer::GetVisualisedTextureId() == texture->GetObjectId();
                                        int mip_level         = TextureViewer::GetMipLevel();
                                        int array_level       = TextureViewer::GetArrayLevel();
                                        mip_and_array_packed  = static_cast<float>(mip_level) + static_cast<float>(array_level) * 0.03125f; // 1/32 = 0.03125

                                        if (array_level > 0)
                                        {
                                            cmd_list->SetTexture(Renderer_BindingsSrv::light_depth, texture);
                                        }
                                        else
                                        { 
                                            cmd_list->SetTexture(Renderer_BindingsSrv::tex, texture);
                                        }

                                        if (is_texture_visualised)
                                        {
                                            m_channel_r      = TextureViewer::GetVisualisationFlags() & Visualise_Channel_R;
                                            m_channel_g      = TextureViewer::GetVisualisationFlags() & Visualise_Channel_G;
                                            m_channel_b      = TextureViewer::GetVisualisationFlags() & Visualise_Channel_B;
                                            m_channel_a      = TextureViewer::GetVisualisationFlags() & Visualise_Channel_A;
                                            m_gamma_correct  = TextureViewer::GetVisualisationFlags() & Visualise_GammaCorrect;
                                            m_pack           = TextureViewer::GetVisualisationFlags() & Visualise_Pack;
                                            m_boost          = TextureViewer::GetVisualisationFlags() & Visualise_Boost;
                                            m_abs            = TextureViewer::GetVisualisationFlags() & Visualise_Abs;
                                            m_point_sampling = TextureViewer::GetVisualisationFlags() & Visualise_Sample_Point;
                                        }
                                    }
                                }

                                rhi_resources->push_constant_buffer_pass.set_f4_value(m_channel_r, m_channel_g, m_channel_b, m_channel_a);
                                rhi_resources->push_constant_buffer_pass.set_f3_value(m_gamma_correct, m_pack, m_boost);
                                rhi_resources->push_constant_buffer_pass.set_f3_value2(m_abs, m_point_sampling, mip_and_array_packed);
                                rhi_resources->push_constant_buffer_pass.set_is_transparent_and_material_index(is_texture_visualised, is_frame_texture ? 1 : 0);
                            }

                            // compute transform matrix
                            {
                                const float L = draw_data->DisplayPos.x;
                                const float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
                                const float T = draw_data->DisplayPos.y;
                                const float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;

                                rhi_resources->push_constant_buffer_pass.transform = Matrix
                                (
                                    2.0f / (R - L), 0.0f, 0.0f, (R + L) / (L - R),
                                    0.0f, 2.0f / (T - B), 0.0f, (T + B) / (B - T),
                                    0.0f, 0.0f, 0.5f, 0.5f,
                                    0.0f, 0.0f, 0.0f, 1.0f
                                );
                            }

                            cmd_list->PushConstants(0, sizeof(Pcb_Pass), &rhi_resources->push_constant_buffer_pass);
                        }

                        cmd_list->DrawIndexed(pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset);
                    }
                }

                global_idx_offset += static_cast<uint32_t>(cmd_list_imgui->IdxBuffer.Size);
                global_vtx_offset += static_cast<uint32_t>(cmd_list_imgui->VtxBuffer.Size);
            }
        }

        cmd_list->EndTimeblock();

        if (!is_main_window)
        {
            cmd_list->Submit(queue, swapchain->GetObjectId());
        }
    }

    void window_create(ImGuiViewport* viewport)
    {
        SP_ASSERT(viewport->PlatformHandle);

        // note: platformHandle is SDL_Window, PlatformHandleRaw is HWND
        SDL_Window* sdl_window = SDL_GetWindowFromID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(viewport->PlatformHandle)));

        WindowData* window = new WindowData();
        window->swapchain = make_shared<RHI_SwapChain>
        (
            sdl_window, 
            static_cast<uint32_t>(viewport->Size.x),
            static_cast<uint32_t>(viewport->Size.y),
            RHI_Present_Mode::Immediate,
            2,
            spartan::Display::GetHdr(),
            (string("swapchain_child_") + string(to_string(viewport->ID))).c_str()
        );

        window->viewport_rhi_resources = make_shared<ViewportRhiResources>("imgui_child_window", window->swapchain.get());
        viewport->RendererUserData     = window;
    }

    void window_destroy(ImGuiViewport* viewport)
    {
        if (WindowData* window = static_cast<WindowData*>(viewport->RendererUserData))
        {
            viewport->RendererUserData = nullptr;
            delete window;
        }
    }

    void window_resize(ImGuiViewport* viewport, const ImVec2 size)
    {
        static_cast<WindowData*>(viewport->RendererUserData)->swapchain->Resize(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));
    }

    void window_render(ImGuiViewport* viewport, void*)
    {
        const bool clear = !(viewport->Flags & ImGuiViewportFlags_NoRendererClear);
        render(viewport->DrawData, static_cast<WindowData*>(viewport->RendererUserData), clear);
    }

    void window_present(ImGuiViewport* viewport, void*)
    {
        WindowData* window = static_cast<WindowData*>(viewport->RendererUserData);
        window->swapchain->Present();
    }

    void initialize_platform_interface()
    {
        ImGuiPlatformIO& platform_io       = ImGui::GetPlatformIO();
        platform_io.Renderer_CreateWindow  = window_create;
        platform_io.Renderer_DestroyWindow = window_destroy;
        platform_io.Renderer_SetWindowSize = window_resize;
        platform_io.Renderer_RenderWindow  = window_render;
        platform_io.Renderer_SwapBuffers   = window_present;
    }
}
