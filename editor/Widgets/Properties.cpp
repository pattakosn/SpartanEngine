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

//= INCLUDES ============================
#include "Properties.h"
#include "Window.h"
#include "../ImGui/ImGui_Extension.h"
#include "../ImGui/ImGui_Style.h"
#include "../ImGui/Source/imgui_stdlib.h"
#include "../Widgets/ButtonColorPicker.h"
#include "Core/Engine.h"
#include "World/Entity.h"
#include "World/Components/Renderable.h"
#include "World/Components/PhysicsBody.h"
#include "World/Components/Light.h"
#include "World/Components/AudioSource.h"
#include "World/Components/Terrain.h"
#include "World/Components/Camera.h"
//=======================================

//= NAMESPACES =========
using namespace std;
using namespace spartan;
using namespace math;
//======================

weak_ptr<Entity> Properties::m_inspected_entity;
weak_ptr<Material> Properties::m_inspected_material;

namespace
{
    #define column_pos_x 180.0f * spartan::Window::GetDpiScale()
    #define item_width   120.0f * spartan::Window::GetDpiScale()

    string context_menu_id;
    Component* copied_component = nullptr;

    void component_context_menu_options(const string& id, Component* component, const bool removable)
    {
        if (ImGui::BeginPopup(id.c_str()))
        {
            if (removable)
            {
                if (ImGui::MenuItem("Remove"))
                {
                    if (auto entity = Properties::m_inspected_entity.lock())
                    {
                        if (component)
                        {
                            entity->RemoveComponentById(component->GetObjectId());
                        }
                    }
                }
            }

            if (ImGui::MenuItem("Copy Attributes"))
            {
                copied_component = component;
            }

            if (ImGui::MenuItem("Paste Attributes"))
            {
                if (copied_component && copied_component->GetType() == component->GetType())
                {
                    component->SetAttributes(copied_component->GetAttributes());
                }
            }

            ImGui::EndPopup();
        }
    }

    bool component_begin(const string& name, const IconType icon_enum, Component* component_instance, bool options = true, const bool removable = true)
    {
        // collapsible contents
        ImGui::PushFont(Editor::font_bold);
        const bool collapsed = ImGuiSp::collapsing_header(name.c_str(), ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_DefaultOpen);
        ImGui::PopFont();

        // component Icon - top left
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();

        // component Options - top right
        if (options)
        {
            const float icon_width = 16.0f;
            const auto original_pen_y = ImGui::GetCursorPosY();

            ImGui::SetCursorPosY(original_pen_y + 5.0f);
            ImGuiSp::image(icon_enum, 15,ImGui::Style::color_accent_1);
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - icon_width + 1.0f); ImGui::SetCursorPosY(original_pen_y);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
            if (ImGuiSp::image_button(nullptr, IconType::Component_Options, icon_width, false))
            {
                context_menu_id = name;
                ImGui::OpenPopup(context_menu_id.c_str());
            }
            ImGui::PopStyleColor(1);

            if (component_instance)
            {
                if (context_menu_id == name)
                {
                    component_context_menu_options(context_menu_id, component_instance, removable);
                }
            }
        }

        return collapsed;
    }

    void component_end()
    {
        ImGui::Separator();
    }
}

Properties::Properties(Editor* editor) : Widget(editor)
{
    m_title          = "Properties";
    m_size_initial.x = 500; // min width

    m_colorPicker_light     = make_unique<ButtonColorPicker>("Light Color Picker");
    m_material_color_picker = make_unique<ButtonColorPicker>("Material Color Picker");
    m_colorPicker_camera    = make_unique<ButtonColorPicker>("Camera Color Picker");
}

void Properties::OnTickVisible()
{
    bool is_in_game_mode = spartan::Engine::IsFlagSet(spartan::EngineMode::Playing);
    ImGui::BeginDisabled(is_in_game_mode);
    {
        ImGui::PushItemWidth(item_width);
        {
            if (!m_inspected_entity.expired())
            {
                shared_ptr<Entity> entity_ptr = m_inspected_entity.lock();
                Renderable* renderable        = entity_ptr->GetComponent<Renderable>();
                Material* material            = renderable ? renderable->GetMaterial() : nullptr;

                ShowTransform(entity_ptr);
                ShowLight(entity_ptr->GetComponent<Light>());
                ShowCamera(entity_ptr->GetComponent<Camera>());
                ShowTerrain(entity_ptr->GetComponent<Terrain>());
                ShowAudioSource(entity_ptr->GetComponent<AudioSource>());
                ShowRenderable(renderable);
                ShowMaterial(material);
                ShowPhysicsBody(entity_ptr->GetComponent<PhysicsBody>());

                ShowAddComponentButton();
            }
            else if (!m_inspected_material.expired())
            {
                ShowMaterial(m_inspected_material.lock().get());
            }

            ImGui::PopItemWidth();
        }
    }
    ImGui::EndDisabled();
}

void Properties::Inspect(const shared_ptr<Entity> entity)
{
    m_inspected_entity = entity;

    // If we were previously inspecting a material, save the changes
    if (!m_inspected_material.expired())
    {
        m_inspected_material.lock()->SaveToFile(m_inspected_material.lock()->GetResourceFilePath());
    }
    m_inspected_material.reset();
}

void Properties::Inspect(const shared_ptr<Material> material)
{
    m_inspected_entity.reset();
    m_inspected_material = material;
}

void Properties::ShowTransform(shared_ptr<Entity> entity) const 
{
    if (component_begin("Transform", IconType::Component_Transform, nullptr, true, false))
    {
        //= REFLECT =====================================
        Vector3 position    = entity->GetPositionLocal();
        Quaternion rotation = entity->GetRotationLocal();
        Vector3 scale       = entity->GetScaleLocal();
        //===============================================

        // convert current rotation to Euler angles for display
        static Vector3 last_frame_euler = rotation.ToEulerAngles();
        Vector3 current_euler           = last_frame_euler;

        ImGui::AlignTextToFramePadding();
        ImGuiSp::vector3("Position (m)", position);
        ImGui::SameLine();
        ImGuiSp::vector3("Rotation (degrees)", current_euler);
        ImGui::SameLine();
        ImGuiSp::vector3("Scale", scale);

        // calculate the the rotation delta, convert it to a quaternion, and apply it, avoiding gimbal lock
        Vector3 delta_euler         = current_euler - last_frame_euler;
        last_frame_euler            = current_euler;
        Quaternion delta_quaternion = Quaternion::FromEulerAngles(delta_euler);
        rotation                    = delta_quaternion * rotation;
        rotation.Normalize();

        //= MAP ===========================
        entity->SetPositionLocal(position);
        entity->SetScaleLocal(scale);
        entity->SetRotationLocal(rotation);
        //=================================
    }
    component_end();
}

void Properties::ShowLight(spartan::Light* light) const
{
    if (!light)
        return;

    if (component_begin("Light", IconType::Component_Light, light))
    {
        //= REFLECT ============================================================================
        static vector<string> types = { "Directional", "Point", "Spot" };
        float intensity             = light->GetIntensityLumens();
        float temperature_kelvin    = light->GetTemperature();
        float angle                 = light->GetAngle() * math::rad_to_deg * 2.0f;
        bool shadows                = light->GetFlag(spartan::LightFlags::Shadows);
        bool shadows_transparent    = light->GetFlag(spartan::LightFlags::ShadowsTransparent);
        bool shadows_screen_space   = light->GetFlag(spartan::LightFlags::ShadowsScreenSpace);
        bool volumetric             = light->GetFlag(spartan::LightFlags::Volumetric);
        float range                 = light->GetRange();
        m_colorPicker_light->SetColor(light->GetColor());
        //======================================================================================

        // type
        ImGui::Text("Type");
        ImGui::SameLine(column_pos_x);
        uint32_t selection_index = static_cast<uint32_t>(light->GetLightType());
        if (ImGuiSp::combo_box("##LightType", types, &selection_index))
        {
            light->SetLightType(static_cast<LightType>(selection_index));
        }

        // temperature
        {
            ImGui::Text("Temperature");

            // color
            ImGui::SameLine(column_pos_x);
            m_colorPicker_light->Update();

            // kelvin
            ImGui::SameLine();
            ImGuiSp::draw_float_wrap("K", &temperature_kelvin, 0.3f, 1000.0f, 40000.0f);
            ImGuiSp::tooltip("Temperature expressed in Kelvin");
        }
        // intensity
        {
            static vector<string> intensity_types =
            {
                "Sky sunlight moon",
                "Sky sunlight morning evening",
                "Sky overcast day",
                "Sky twilight",
                "Bulb stadium",
                "Bulb 500 watt",
                "Bulb 150 watt",
                "Bulb 100 watt",
                "Bulb 60 watt",
                "Bulb 25 watt",
                "Bulb flashlight",
                "Black hole",
                "Custom"
            };

            ImGui::Text("Intensity");

            // light types
            ImGui::SameLine(column_pos_x);
            uint32_t intensity_type_index = static_cast<uint32_t>(light->GetIntensity());
            if (ImGuiSp::combo_box("##light_intensity_type", intensity_types, &intensity_type_index))
            {
                light->SetIntensity(static_cast<LightIntensity>(intensity_type_index));
                intensity = light->GetIntensityLumens();
            }
            ImGuiSp::tooltip("Common light types");

            // intensity
            ImGui::SameLine();
            ImGuiSp::draw_float_wrap(light->GetLightType() == spartan::LightType::Directional ? "lux" : "lm", &intensity, 10.0f, 0.0f, 120000.0f);
            ImGuiSp::tooltip("Intensity expressed in lux (directional) or lumens (point and spot)");
        }

        // shadows
        {
            ImGui::Text("Shadows");
            ImGui::SameLine(column_pos_x); ImGui::Checkbox("##light_shadows", &shadows);

            if (shadows)
            {
                // transparent shadows
                ImGui::Text("Transparent Shadows");
                ImGui::SameLine(column_pos_x); ImGui::Checkbox("##light_shadows_transparent", &shadows_transparent);
                ImGuiSp::tooltip("Allows transparent objects to cast colored translucent shadows");

                // transparent shadows
                ImGui::Text("Screen Space Shadows");
                ImGui::SameLine(column_pos_x); ImGui::Checkbox("##light_shadows_screen_space", &shadows_screen_space);
                ImGuiSp::tooltip("Screen space shadows from Days Gone - PS4");

                // volumetric
                ImGui::Text("Volumetric");
                ImGui::SameLine(column_pos_x); ImGui::Checkbox("##light_volumetric", &volumetric);
                ImGuiSp::tooltip("The shadow map is used to determine which parts of the \"air\" should be lit");
            }
        }

        // range
        if (light->GetLightType() != LightType::Directional)
        {
            ImGui::Text("Range");
            ImGui::SameLine(column_pos_x);
            ImGuiSp::draw_float_wrap("##lightRange", &range, 0.01f, 0.0f, 1000.0f);
        }

        // angle
        if (light->GetLightType() == LightType::Spot)
        {
            ImGui::Text("Angle");
            ImGui::SameLine(column_pos_x);
            ImGuiSp::draw_float_wrap("##lightAngle", &angle, 0.01f, 1.0f, 179.0f);
        }

        //= MAP ===================================================================================================================
        if (intensity != light->GetIntensityLumens())                     light->SetIntensity(intensity);
        if (angle != light->GetAngle() * math::rad_to_deg * 0.5f) light->SetAngle(angle * math::deg_to_rad * 0.5f);
        if (range != light->GetRange())                                   light->SetRange(range);
        if (m_colorPicker_light->GetColor() != light->GetColor())         light->SetColor(m_colorPicker_light->GetColor());
        if (temperature_kelvin != light->GetTemperature())                light->SetTemperature(temperature_kelvin);
        light->SetFlag(spartan::LightFlags::ShadowsTransparent, shadows_transparent);
        light->SetFlag(spartan::LightFlags::ShadowsScreenSpace, shadows_screen_space);
        light->SetFlag(spartan::LightFlags::Volumetric, volumetric);
        light->SetFlag(spartan::LightFlags::Shadows, shadows);
        //=========================================================================================================================
    }
    component_end();
}

void Properties::ShowRenderable(spartan::Renderable* renderable) const
{
    if (!renderable)
        return;

    if (component_begin("Renderable", IconType::Component_Renderable, renderable))
    {
        //= REFLECT =======================================================================
        string name_mesh              = renderable->GetMeshName();
        Material* material            = renderable->GetMaterial();
        uint32_t instance_count       = renderable->GetInstanceCount();
        uint32_t instance_group_count = renderable->GetInstanceGroupCount();
        string name_material          = material ? material->GetObjectName() : "N/A";
        bool cast_shadows             = renderable->HasFlag(RenderableFlags::CastsShadows);
        bool is_visible               = renderable->IsVisible();
        //=================================================================================

        // mesh
        ImGui::Text("Mesh");
        ImGui::SameLine(column_pos_x);
        ImGui::InputText("##renderable_mesh_name", &name_mesh, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);

        // geometry
        {
            // move to column_pos_x before starting the table
            ImGui::SetCursorPosX(column_pos_x);
            
            int lod_count = renderable->GetLodCount();
            if (ImGui::BeginTable("##geometry_table", lod_count + 1, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit))
            {
                // setup columns
                ImGui::TableSetupColumn("");  // first column for labels
                for (int i = 0; i < lod_count; i++)
                {
                    ImGui::TableSetupColumn(("LOD " + to_string(i + 1)).c_str());  // start numbering from 1
                }
        
                // header row
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("LODs");
                for (int i = 0; i < lod_count; i++)
                {
                    ImGui::TableSetColumnIndex(i + 1);
                    ImGui::Text(("LOD " + to_string(i + 1)).c_str());
                }
        
                // row 1: vertices
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Vertices");
                for (int i = 0; i < lod_count; i++)
                {
                    ImGui::TableSetColumnIndex(i + 1);
                    ImGui::Text("%d", renderable->GetVertexCount(i));
                }
        
                // row 2: indices
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Indices");
                for (int i = 0; i < lod_count; i++)
                {
                    ImGui::TableSetColumnIndex(i + 1);
                    ImGui::Text("%d", renderable->GetIndexCount(i));
                }
        
                ImGui::EndTable();
            }

            // we can print the lod index for each instanceb but it's not needed (so far)
            if (!renderable->HasInstancing())
            { 
                ImGui::Text("Lod Index");
                ImGui::SameLine(column_pos_x);
                ImGui::LabelText("##renderable_lod_index", to_string(renderable->GetLodIndex()).c_str(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
            }
        }

        // instancing
        if (instance_count != 0)
        {
            ImGui::Text("Instances");
            ImGui::SameLine(column_pos_x);
            ImGui::LabelText("##renderable_instance_count", to_string(instance_count).c_str(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);

            ImGui::Text("Instance Groups");
            ImGui::SameLine(column_pos_x);
            ImGui::LabelText("##renderable_instance_group_count", to_string(instance_group_count).c_str(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
        }

        // draw distance
        ImGui::Text("Draw Distance");
        ImGui::SameLine(column_pos_x);
        float draw_distance = renderable->GetMaxRenderDistance();
        ImGui::InputFloat("##renderable_draw_distance", &draw_distance, 1.0f, 10.0f, "%.0f");
        renderable->SetMaxRenderDistance(draw_distance);

        // material
        ImGui::Text("Material");
        ImGui::SameLine(column_pos_x);
        ImGui::InputText("##renderable_material", &name_material, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
        if (auto payload = ImGuiSp::receive_drag_drop_payload(ImGuiSp::DragPayloadType::Material))
        {
            renderable->SetMaterial(std::get<const char*>(payload->data));
        }

        ImGui::Text("Cast shadows");
        ImGui::SameLine(column_pos_x);
        ImGui::Checkbox("##renderable_cast_shadows", &cast_shadows);

        ImGui::Text("Visible");
        ImGui::SameLine(column_pos_x);
        ImGui::LabelText("##renderable_visible", is_visible ? "true" : "false", ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);

        //= MAP =========================================================
        renderable->SetFlag(RenderableFlags::CastsShadows, cast_shadows);
        //===============================================================
    }
    component_end();
}

void Properties::ShowPhysicsBody(PhysicsBody* body) const
{
    if (!body)
        return;

    const auto input_text_flags = ImGuiInputTextFlags_CharsDecimal;
    const float step            = 0.1f;
    const float step_fast       = 0.1f;
    const auto precision        = "%.3f";

    if (component_begin("PhysicsBody", IconType::Component_PhysicsBody, body))
    {
        //= REFLECT ==========================================================
        float mass             = body->GetMass();
        float friction         = body->GetFriction();
        float friction_rolling = body->GetFrictionRolling();
        float restitution      = body->GetRestitution();
        bool use_gravity       = body->GetUseGravity();
        bool is_kinematic      = body->GetIsKinematic();
        bool freeze_pos_x      = static_cast<bool>(body->GetPositionLock().x);
        bool freeze_pos_y      = static_cast<bool>(body->GetPositionLock().y);
        bool freeze_pos_z      = static_cast<bool>(body->GetPositionLock().z);
        bool freeze_rot_x      = static_cast<bool>(body->GetRotationLock().x);
        bool freeze_rot_y      = static_cast<bool>(body->GetRotationLock().y);
        bool freeze_rot_z      = static_cast<bool>(body->GetRotationLock().z);
        Vector3 center_of_mass = body->GetCenterOfMass();
        Vector3 bounding_box   = body->GetBoundingBox();
        //====================================================================

        // body type
        {
            static vector<string> shape_types =
            {
                "Rigid Body",
                "Vehicle"
            };

            ImGui::Text("Body Type");
            ImGui::SameLine(column_pos_x);
            uint32_t selection_index = static_cast<uint32_t>(body->GetBodyType());
            if (ImGuiSp::combo_box("##physics_body_type", shape_types, &selection_index))
            {
                body->SetBodyType(static_cast<PhysicsBodyType>(selection_index));
            }
        }

        // mass
        ImGui::Text("Mass (kg)");
        ImGui::SameLine(column_pos_x); ImGui::InputFloat("##physics_body_mass", &mass, step, step_fast, precision, input_text_flags);

        // friction
        ImGui::Text("Friction");
        ImGui::SameLine(column_pos_x); ImGui::InputFloat("##physics_body_friction", &friction, step, step_fast, precision, input_text_flags);

        // rolling friction
        ImGui::Text("Rolling Friction");
        ImGui::SameLine(column_pos_x); ImGui::InputFloat("##physics_body_rolling_friction", &friction_rolling, step, step_fast, precision, input_text_flags);

        // restitution
        ImGui::Text("Restitution");
        ImGui::SameLine(column_pos_x); ImGui::InputFloat("##physics_body_restitution", &restitution, step, step_fast, precision, input_text_flags);

        // use gravity
        ImGui::Text("Use Gravity");
        ImGui::SameLine(column_pos_x); ImGui::Checkbox("##physics_body_use_gravity", &use_gravity);

        // is kinematic
        ImGui::Text("Is Kinematic");
        ImGui::SameLine(column_pos_x); ImGui::Checkbox("##physics_body_is_kinematic", &is_kinematic);

        // freeze position
        ImGui::Text("Freeze Position");
        ImGui::SameLine(column_pos_x); ImGui::Text("X");
        ImGui::SameLine(); ImGui::Checkbox("##physics_body_pos_x", &freeze_pos_x);
        ImGui::SameLine(); ImGui::Text("Y");
        ImGui::SameLine(); ImGui::Checkbox("##physics_body_pos_y", &freeze_pos_y);
        ImGui::SameLine(); ImGui::Text("Z");
        ImGui::SameLine(); ImGui::Checkbox("##physics_body_pos_z", &freeze_pos_z);

        // freeze rotation
        ImGui::Text("Freeze Rotation");
        ImGui::SameLine(column_pos_x); ImGui::Text("X");
        ImGui::SameLine(); ImGui::Checkbox("##physics_body_rot_x", &freeze_rot_x);
        ImGui::SameLine(); ImGui::Text("Y");
        ImGui::SameLine(); ImGui::Checkbox("##physics_body_rot_y", &freeze_rot_y);
        ImGui::SameLine(); ImGui::Text("Z");
        ImGui::SameLine(); ImGui::Checkbox("##physics_body_rot_z", &freeze_rot_z);

        ImGui::Separator();

        // collision shape
        {
            static vector<string> shape_types =
            {
                "Box",
                "Sphere",
                "Static Plane",
                "Cylinder",
                "Capsule",
                "Cone",
                "Terrain",
                "Mesh"
            };

            ImGui::Text("Shape Type");
            ImGui::SameLine(column_pos_x);
            uint32_t selection_index = static_cast<uint32_t>(body->GetShapeType());
            if (ImGuiSp::combo_box("##physics_body_shape", shape_types, &selection_index))
            {
                body->SetShapeType(static_cast<PhysicsShape>(selection_index));
            }
        }

        // center
        ImGui::Text("Shape Center");
        ImGui::SameLine(column_pos_x); ImGui::PushID("physics_body_shape_center_x"); ImGui::InputFloat("X", &center_of_mass.x, step, step_fast, precision, input_text_flags); ImGui::PopID();
        ImGui::SameLine();             ImGui::PushID("physics_body_shape_center_y"); ImGui::InputFloat("Y", &center_of_mass.y, step, step_fast, precision, input_text_flags); ImGui::PopID();
        ImGui::SameLine();             ImGui::PushID("physics_body_shape_center_z"); ImGui::InputFloat("Z", &center_of_mass.z, step, step_fast, precision, input_text_flags); ImGui::PopID();

        // size
        ImGui::Text("Shape Size");
        ImGui::SameLine(column_pos_x); ImGui::PushID("physics_body_shape_size_x"); ImGui::InputFloat("X", &bounding_box.x, step, step_fast, precision, input_text_flags); ImGui::PopID();
        ImGui::SameLine();             ImGui::PushID("physics_body_shape_size_y"); ImGui::InputFloat("Y", &bounding_box.y, step, step_fast, precision, input_text_flags); ImGui::PopID();
        ImGui::SameLine();             ImGui::PushID("physics_body_shape_size_z"); ImGui::InputFloat("Z", &bounding_box.z, step, step_fast, precision, input_text_flags); ImGui::PopID();

        //= MAP ===============================================================================================================================================================================================
        if (mass != body->GetMass())                                      body->SetMass(mass);
        if (friction != body->GetFriction())                              body->SetFriction(friction);
        if (friction_rolling != body->GetFrictionRolling())               body->SetFrictionRolling(friction_rolling);
        if (restitution != body->GetRestitution())                        body->SetRestitution(restitution);
        if (use_gravity != body->GetUseGravity())                         body->SetUseGravity(use_gravity);
        if (is_kinematic != body->GetIsKinematic())                       body->SetIsKinematic(is_kinematic);
        if (freeze_pos_x != static_cast<bool>(body->GetPositionLock().x)) body->SetPositionLock(Vector3(static_cast<float>(freeze_pos_x), static_cast<float>(freeze_pos_y), static_cast<float>(freeze_pos_z)));
        if (freeze_pos_y != static_cast<bool>(body->GetPositionLock().y)) body->SetPositionLock(Vector3(static_cast<float>(freeze_pos_x), static_cast<float>(freeze_pos_y), static_cast<float>(freeze_pos_z)));
        if (freeze_pos_z != static_cast<bool>(body->GetPositionLock().z)) body->SetPositionLock(Vector3(static_cast<float>(freeze_pos_x), static_cast<float>(freeze_pos_y), static_cast<float>(freeze_pos_z)));
        if (freeze_rot_x != static_cast<bool>(body->GetRotationLock().x)) body->SetRotationLock(Vector3(static_cast<float>(freeze_rot_x), static_cast<float>(freeze_rot_y), static_cast<float>(freeze_rot_z)));
        if (freeze_rot_y != static_cast<bool>(body->GetRotationLock().y)) body->SetRotationLock(Vector3(static_cast<float>(freeze_rot_x), static_cast<float>(freeze_rot_y), static_cast<float>(freeze_rot_z)));
        if (freeze_rot_z != static_cast<bool>(body->GetRotationLock().z)) body->SetRotationLock(Vector3(static_cast<float>(freeze_rot_x), static_cast<float>(freeze_rot_y), static_cast<float>(freeze_rot_z)));
        if (center_of_mass != body->GetCenterOfMass())                    body->SetCenterOfMass(center_of_mass);
        if (bounding_box != body->GetBoundingBox())                       body->SetBoundingBox(bounding_box);
        //=====================================================================================================================================================================================================
    }
    component_end();
}

void Properties::ShowMaterial(Material* material) const
{
    if (!material)
        return;

    if (component_begin("Material", IconType::Component_Material, nullptr, false))
    {
        //= REFLECT ================================================
        math::Vector2 tiling = Vector2(
            material->GetProperty(MaterialProperty::TextureTilingX),
            material->GetProperty(MaterialProperty::TextureTilingY)
        );

        math::Vector2 offset = Vector2(
            material->GetProperty(MaterialProperty::TextureOffsetX),
            material->GetProperty(MaterialProperty::TextureOffsetY)
        );

        m_material_color_picker->SetColor(Color(
            material->GetProperty(MaterialProperty::ColorR),
            material->GetProperty(MaterialProperty::ColorG),
            material->GetProperty(MaterialProperty::ColorB),
            material->GetProperty(MaterialProperty::ColorA)
        ));
        //==========================================================

        // name
        ImGui::NewLine();
        ImGui::Text("Name");
        ImGui::SameLine(column_pos_x);
        ImGui::Text("%s", material->GetObjectName().c_str());

        // optimized
        bool optimized = material->GetProperty(MaterialProperty::Optimized) != 0.0f;
        {
            ImGui::Text("Optimized");
            ImGui::SameLine(column_pos_x);
            ImGui::Text(optimized ? "Yes" : "No");
            ImGuiSp::tooltip("Optimized materials can't be modified");
        }

        // texture slots
        {
            const auto show_property = [this, &material, &optimized](const char* name, const char* tooltip, const MaterialTextureType mat_tex, const MaterialProperty mat_property)
            {
                bool show_texture  = mat_tex      != MaterialTextureType::Max;
                bool show_modifier = mat_property != MaterialProperty::Max;
        
                // name
                if (name)
                {
                    ImGui::Text("%s", name);
        
                    if (tooltip)
                    {
                        ImGuiSp::tooltip(tooltip);
                    }
        
                    if (show_texture || show_modifier)
                    {
                        ImGui::SameLine(column_pos_x);
                    }
                }
        
                // texture
                ImGui::BeginDisabled(optimized);
                if (show_texture)
                {
                    // for the current texture type (mat_tex), show all its slots
                    for (uint32_t slot = 0; slot < material->GetUsedSlotCount(); ++slot)
                    {
                        MaterialTextureType texture_type = static_cast<MaterialTextureType>(mat_tex);
                        
                        // create a lambda that captures both the type and slot for the setter
                        auto setter = [material, texture_type, slot](spartan::RHI_Texture* texture) 
                        { 
                            material->SetTexture(texture_type, texture, slot);
                        };
                
                        // slots are shown side by side for each type
                        if (slot > 0)
                        {
                            ImGui::SameLine();
                        }
                
                        // get the texture for this slot and show it in the UI
                        RHI_Texture* texture = material->GetTexture(texture_type, slot);
                        ImGuiSp::image_slot(texture, setter);
                    }
                
                    if (show_modifier)
                    {
                        ImGui::SameLine();
                    }
                }
                ImGui::EndDisabled();
        
                // modifier/multiplier
                if (show_modifier)
                {
                    if (mat_property == MaterialProperty::ColorA)
                    {
                        m_material_color_picker->Update();
                    }
                    else
                    { 
                        float value = material->GetProperty(mat_property);

                        if (mat_property != MaterialProperty::Metalness)
                        {
                            float min = 0.0f;
                            float max = 1.0f;

                            if (mat_property == MaterialProperty::Ior)
                            {
                                min = 1.0f;
                                max = 2.4f; // diamond
                            }

                            // this custom slider already has a unique id
                            ImGuiSp::draw_float_wrap("", &value, 0.004f, min, max);
                        }
                        else
                        {
                            bool is_metallic = value != 0.0f;
                            ImGui::PushID(static_cast<int>(ImGui::GetCursorPosX() + ImGui::GetCursorPosY()));
                            ImGui::Checkbox("", &is_metallic);
                            ImGui::PopID();
                            value = is_metallic ? 1.0f : 0.0f;
                        }

                        material->SetProperty(mat_property, value);
                    }
                }
            };
        
            // properties with textures
            show_property("Color",                "Surface color",                                                                     MaterialTextureType::Color,     MaterialProperty::ColorA);
            show_property("Roughness",            "Specifies microfacet roughness of the surface for diffuse and specular reflection", MaterialTextureType::Roughness, MaterialProperty::Roughness);
            show_property("Metalness",            "Blends between a non-metallic and metallic material model",                         MaterialTextureType::Metalness, MaterialProperty::Metalness);
            show_property("Normal",               "Controls the normals of the base layers",                                           MaterialTextureType::Normal,    MaterialProperty::Normal);
            show_property("Height",               "Perceived depth for parallax mapping",                                              MaterialTextureType::Height,    MaterialProperty::Height);
            show_property("Occlusion",            "Amount of light loss, can be complementary to SSAO",                                MaterialTextureType::Occlusion, MaterialProperty::Max);
            show_property("Emission",             "Light emission from the surface, works nice with bloom",                            MaterialTextureType::Emission,  MaterialProperty::Max);
            show_property("Alpha mask",           "Discards pixels",                                                                   MaterialTextureType::AlphaMask, MaterialProperty::Max);
            show_property("Clearcoat",            "Extra white specular layer on top of others",                                       MaterialTextureType::Max,       MaterialProperty::Clearcoat);
            show_property("Clearcoat roughness",  "Roughness of clearcoat specular",                                                   MaterialTextureType::Max,       MaterialProperty::Clearcoat_Roughness);
            show_property("Anisotropic",          "Amount of anisotropy for specular reflection",                                      MaterialTextureType::Max,       MaterialProperty::Anisotropic);
            show_property("Anisotropic rotation", "Rotates the direction of anisotropy, with 1.0 going full circle",                   MaterialTextureType::Max,       MaterialProperty::AnisotropicRotation);
            show_property("Sheen",                "Amount of soft velvet like reflection near edges",                                  MaterialTextureType::Max,       MaterialProperty::Sheen);
            show_property("Subsurface scattering","Amount of translucency",                                                            MaterialTextureType::Max,       MaterialProperty::SubsurfaceScattering);
        }
        
        // index of refraction
        {
            static vector<string> ior_types =
            {
                "Air",
                "Water",
                "Eyes",
                "Glass",
                "Sapphire",
                "Diamond"
            };
        
            ImGui::Text("IOR");
            ImGui::SameLine(column_pos_x);
            uint32_t ior_index = static_cast<uint32_t>(Material::IorToEnum(material->GetProperty(MaterialProperty::Ior)));
            if (ImGuiSp::combo_box("##material_ior", ior_types, &ior_index))
            {
                material->SetProperty(MaterialProperty::Ior, static_cast<float>(Material::EnumToIor(static_cast<MaterialIor>(ior_index))));
            }
        }
        
        // uv
        {
            // tiling
            ImGui::Text("Tiling");
            ImGui::SameLine(column_pos_x); ImGui::Text("X");
            ImGui::SameLine(); ImGui::InputFloat("##matTilingX", &tiling.x, 0.01f, 0.1f, "%.2f", ImGuiInputTextFlags_CharsDecimal);
            ImGui::SameLine(); ImGui::Text("Y");
            ImGui::SameLine(); ImGui::InputFloat("##matTilingY", &tiling.y, 0.01f, 0.1f, "%.2f", ImGuiInputTextFlags_CharsDecimal);
        
            // offset
            ImGui::Text("Offset");
            ImGui::SameLine(column_pos_x); ImGui::Text("X");
            ImGui::SameLine(); ImGui::InputFloat("##matOffsetX", &offset.x, 0.01f, 0.1f, "%.2f", ImGuiInputTextFlags_CharsDecimal);
            ImGui::SameLine(); ImGui::Text("Y");
            ImGui::SameLine(); ImGui::InputFloat("##matOffsetY", &offset.y, 0.01f, 0.1f, "%.2f", ImGuiInputTextFlags_CharsDecimal);
        }

        // rendering
        {
            // cull mode
            {
                static vector<string> cull_modes =
                {
                    "Back",
                    "Front",
                    "None"
                };
        
                ImGui::Text("Culling");
                ImGui::SameLine(column_pos_x);
                uint32_t cull_mode_index = static_cast<uint32_t>(material->GetProperty(MaterialProperty::CullMode));
                if (ImGuiSp::combo_box("##mat_cull_mode", cull_modes, &cull_mode_index))
                {
                    material->SetProperty(MaterialProperty::CullMode, static_cast<float>(cull_mode_index));
                }
            }

            // tessellation
            bool tessellation = material->GetProperty(MaterialProperty::Tessellation) != 0.0f;
            ImGui::Checkbox("Tessellation", &tessellation);
            material->SetProperty(MaterialProperty::Tessellation, tessellation ? 1.0f : 0.0f);

            // wind animation
            bool wind_animation = material->GetProperty(MaterialProperty::IsTree) != 0.0f;
            ImGui::Checkbox("Wind animation", &wind_animation);
            material->SetProperty(MaterialProperty::IsTree, wind_animation ? 1.0f : 0.0f);
        }

        //= MAP ===============================================================================
        material->SetProperty(MaterialProperty::TextureTilingX, tiling.x);
        material->SetProperty(MaterialProperty::TextureTilingY, tiling.y);
        material->SetProperty(MaterialProperty::TextureOffsetX, offset.x);
        material->SetProperty(MaterialProperty::TextureOffsetY, offset.y);
        material->SetProperty(MaterialProperty::ColorR, m_material_color_picker->GetColor().r);
        material->SetProperty(MaterialProperty::ColorG, m_material_color_picker->GetColor().g);
        material->SetProperty(MaterialProperty::ColorB, m_material_color_picker->GetColor().b);
        material->SetProperty(MaterialProperty::ColorA, m_material_color_picker->GetColor().a);
        //=====================================================================================
    }

    component_end();
}

void Properties::ShowCamera(Camera* camera) const
{
    if (!camera)
        return;

    if (component_begin("Camera", IconType::Component_Camera, camera))
    {
        //= REFLECT ======================================================================
        static vector<string> projection_types = { "Perspective", "Orthographic" };
        float aperture                         = camera->GetAperture();
        float shutter_speed                    = camera->GetShutterSpeed();
        float iso                              = camera->GetIso();
        float fov                              = camera->GetFovHorizontalDeg();
        float near_plane                       = camera->GetNearPlane();
        float far_plane                        = camera->GetFarPlane();
        bool first_person_control_enabled      = camera->GetFlag(CameraFlags::CanBeControlled);
        //================================================================================

        const auto input_text_flags = ImGuiInputTextFlags_CharsDecimal;

        // Background
        ImGui::Text("Background");
        ImGui::SameLine(column_pos_x); m_colorPicker_camera->Update();

        // Projection
        ImGui::Text("Projection");
        ImGui::SameLine(column_pos_x);
        uint32_t selection_index = static_cast<uint32_t>(camera->GetProjectionType());
        if (ImGuiSp::combo_box("##cameraProjection", projection_types, &selection_index))
        {
            camera->SetProjection(static_cast<ProjectionType>(selection_index));
        }

        // Aperture
        ImGui::SetCursorPosX(column_pos_x);
        ImGuiSp::draw_float_wrap("Aperture (f-stop)", &aperture, 0.01f, 0.01f, 150.0f);
        ImGuiSp::tooltip("Aperture value in f-stop, controls the amount of light, depth of field and chromatic aberration");

        // Shutter speed
        ImGui::SetCursorPosX(column_pos_x);
        ImGuiSp::draw_float_wrap("Shutter Speed (sec)", &shutter_speed, 0.0001f, 0.0f, 1.0f, "%.4f");
        ImGuiSp::tooltip("Length of time for which the camera shutter is open, controls the amount of motion blur");

        // ISO
        ImGui::SetCursorPosX(column_pos_x);
        ImGuiSp::draw_float_wrap("ISO", &iso, 0.1f, 0.0f, 2000.0f);
        ImGuiSp::tooltip("Sensitivity to light, controls camera noise");

        // Field of View
        ImGui::SetCursorPosX(column_pos_x);
        ImGuiSp::draw_float_wrap("Field of View", &fov, 0.1f, 1.0f, 179.0f);

        // Clipping Planes
        ImGui::Text("Clipping Planes");
        ImGui::SameLine(column_pos_x);      ImGui::InputFloat("Near", &near_plane, 0.01f, 0.01f, "%.2f", input_text_flags);
        ImGui::SetCursorPosX(column_pos_x); ImGui::InputFloat("Far", &far_plane, 0.01f, 0.01f, "%.2f", input_text_flags);

        // FPS Control
        ImGui::Text("First Person Control");
        ImGui::SameLine(column_pos_x); ImGui::Checkbox("##camera_first_person_control", &first_person_control_enabled);
        ImGuiSp::tooltip("Enables first person control while holding down the right mouse button (or when a controller is connected)");
 
        //= MAP =======================================================================================================================================================
        if (aperture != camera->GetAperture())                             camera->SetAperture(aperture);
        if (shutter_speed != camera->GetShutterSpeed())                    camera->SetShutterSpeed(shutter_speed);
        if (iso != camera->GetIso())                                       camera->SetIso(iso);
        if (fov != camera->GetFovHorizontalDeg())                          camera->SetFovHorizontalDeg(fov);
        if (near_plane != camera->GetNearPlane())                          camera->SetNearPlane(near_plane);
        if (far_plane != camera->GetFarPlane())                            camera->SetFarPlane(far_plane);
        if (first_person_control_enabled != camera->GetFlag(CameraFlags::CanBeControlled)) camera->SetFlag(CameraFlags::CanBeControlled, first_person_control_enabled);
        //=============================================================================================================================================================
    }
    component_end();
}

void Properties::ShowTerrain(Terrain* terrain) const
{
    if (!terrain)
        return;

    if (component_begin("Terrain", IconType::Component_Terrain, terrain))
    {
        //= REFLECT =====================
        float min_y = terrain->GetMinY();
        float max_y = terrain->GetMaxY();
        //===============================

        const float cursor_y = ImGui::GetCursorPosY();

        ImGui::BeginGroup();
        {
            ImGui::Text("Height Map");

            ImGuiSp::image_slot(terrain->GetHeightMap(), [&terrain](RHI_Texture* texture)
            {
                terrain->SetHeightMap(texture);
            });

            if (ImGuiSp::button("Generate", ImVec2(82.0f * spartan::Window::GetDpiScale(), 0)))
            {
                spartan::ThreadPool::AddTask([terrain]()
                {
                    terrain->Generate();
                });
            }
        }
        ImGui::EndGroup();

        // Min, max
        ImGui::SameLine();
        ImGui::SetCursorPosY(cursor_y);
        ImGui::BeginGroup();
        {
            ImGui::InputFloat("Min Y", &min_y);
            ImGui::InputFloat("Max Y", &max_y);
        }
        ImGui::EndGroup();

        // Stats
        ImGui::BeginGroup();
        {
            ImGui::Text("Area: %.1f km^2",    terrain->GetArea());
            ImGui::Text("Height samples: %lu", terrain->GetHeightSampleCount());
            ImGui::Text("Vertices: %d",       terrain->GetVertexCount());
            ImGui::Text("Indices: %d ",       terrain->GetIndexCount());
        }
        ImGui::EndGroup();

        //= MAP =================================================
        if (min_y != terrain->GetMinY()) terrain->SetMinY(min_y);
        if (max_y != terrain->GetMaxY()) terrain->SetMaxY(max_y);
        //=======================================================
    }
    component_end();
}

void Properties::ShowAudioSource(spartan::AudioSource* audio_source) const
{
    if (!audio_source)
        return;

    if (component_begin("Audio Source", IconType::Component_AudioSource, audio_source))
    {
        //= REFLECT ==============================================
        string audio_clip_name = audio_source->GetAudioClipName();
        bool mute              = audio_source->GetMute();
        bool play_on_start     = audio_source->GetPlayOnStart();
        bool loop              = audio_source->GetLoop();
        bool is_3d             = audio_source->GetIs3d();
        float volume           = audio_source->GetVolume();
        float pitch            = audio_source->GetPitch();
        //========================================================

        // Audio clip
        ImGui::Text("Audio Clip");
        ImGui::SameLine(column_pos_x);
        ImGui::InputText("##audioSourceAudioClip", &audio_clip_name, ImGuiInputTextFlags_ReadOnly);
        if (auto payload = ImGuiSp::receive_drag_drop_payload(ImGuiSp::DragPayloadType::Audio))
        {
            audio_source->SetAudioClip(std::get<const char*>(payload->data));
        }

        // play on start
        ImGui::Text("Play on Start");
        ImGui::SameLine(column_pos_x); ImGui::Checkbox("##audioSourcePlayOnStart", &play_on_start);

        // mute
        ImGui::Text("Mute");
        ImGui::SameLine(column_pos_x); ImGui::Checkbox("##audioSourceMute", &mute);

        // loop
        ImGui::Text("Loop");
        ImGui::SameLine(column_pos_x); ImGui::Checkbox("##audioSourceLoop", &loop);

        // Pitch
        ImGui::Text("Pitch");
        ImGui::SameLine(column_pos_x); ImGui::SliderFloat("##audioSourcePitch", &pitch, 0.0f, 3.0f);

        // loop
        ImGui::Text("3D");
        ImGui::SameLine(column_pos_x); ImGui::Checkbox("##audioSourceIs3D", &is_3d);

        // volume
        ImGui::Text("Volume");
        ImGui::SameLine(column_pos_x); ImGui::SliderFloat("##audioSourceVolume", &volume, 0.0f, 1.0f);

        //= MAP =========================================================================================
        if (mute != audio_source->GetMute())                 audio_source->SetMute(mute);
        if (play_on_start != audio_source->GetPlayOnStart()) audio_source->SetPlayOnStart(play_on_start);
        if (loop != audio_source->GetLoop())                 audio_source->SetLoop(loop);
        if (is_3d != audio_source->GetIs3d())                audio_source->SetIs3d(is_3d);
        if (volume != audio_source->GetVolume())             audio_source->SetVolume(volume);
        if (pitch != audio_source->GetPitch())               audio_source->SetPitch(pitch);
        //===============================================================================================
    }
    component_end();
}

void Properties::ShowAddComponentButton() const
{
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.5f - 50);
    if (ImGuiSp::button("Add Component"))
    {
        ImGui::OpenPopup("##ComponentContextMenu_Add");
    }
    ComponentContextMenu_Add();
}

void Properties::ComponentContextMenu_Add() const
{
    if (ImGui::BeginPopup("##ComponentContextMenu_Add"))
    {
        if (auto entity = m_inspected_entity.lock())
        {
            if (ImGui::MenuItem("Camera"))
            {
                entity->AddComponent<Camera>();
            }

            if (ImGui::MenuItem("Renderable"))
            {
                entity->AddComponent<Renderable>();
            }

            if (ImGui::MenuItem("Terrain"))
            {
                entity->AddComponent<Terrain>();
            }

            if (ImGui::BeginMenu("Light"))
            {
                if (ImGui::MenuItem("Directional"))
                {
                    entity->AddComponent<Light>()->SetLightType(LightType::Directional);
                }
                else if (ImGui::MenuItem("Point"))
                {
                    entity->AddComponent<Light>()->SetLightType(LightType::Point);
                }
                else if (ImGui::MenuItem("Spot"))
                {
                    entity->AddComponent<Light>()->SetLightType(LightType::Spot);
                }

                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Physics Body"))
            {
                entity->AddComponent<PhysicsBody>();
            }

            if (ImGui::BeginMenu("Audio"))
            {
                if (ImGui::MenuItem("Audio Source"))
                {
                    entity->AddComponent<AudioSource>();
                }

                ImGui::EndMenu();
            }
        }

        ImGui::EndPopup();
    }
}
