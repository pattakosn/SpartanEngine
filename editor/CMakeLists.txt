set(EDITOR_SRC
        Editor.cpp
        ImGui/ImGuiExtension.cpp
        ImGui/Implementation/imgui_impl_sdl2.cpp
        ImGui/Source/ImGuizmo/GraphEditor.cpp
        ImGui/Source/ImGuizmo/ImCurveEdit.cpp
        ImGui/Source/ImGuizmo/ImGradient.cpp
        ImGui/Source/ImGuizmo/ImGuizmo.cpp
        ImGui/Source/ImGuizmo/ImSequencer.cpp
        ImGui/Source/imgui.cpp
        ImGui/Source/imgui_demo.cpp
        ImGui/Source/imgui_draw.cpp
        ImGui/Source/imgui_freetype.cpp
        ImGui/Source/imgui_stdlib.cpp
        ImGui/Source/imgui_tables.cpp
        ImGui/Source/imgui_widgets.cpp
        Widgets/AssetBrowser.cpp
        Widgets/Console.cpp
        Widgets/Profiler.cpp
        Widgets/ProgressDialog.cpp
        Widgets/Properties.cpp
        Widgets/RenderOptions.cpp
        Widgets/ResourceViewer.cpp
        Widgets/ShaderEditor.cpp
        Widgets/TextureViewer.cpp
        Widgets/TitleBar.cpp
        Widgets/Viewport.cpp
        Widgets/Widget.cpp
        Widgets/WorldViewer.cpp
        WidgetsDeferred/ButtonColorPicker.cpp
        WidgetsDeferred/FileDialog.cpp
        WidgetsDeferred/IconLoader.cpp
        WidgetsDeferred/TextEditor.cpp
        main.cpp
)


add_executable(editor ${EDITOR_SRC} ${EDITOR_HEADERS})
target_link_libraries(editor runtime)
