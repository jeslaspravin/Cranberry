/*!
 * \file WgEditorImGuiLayer.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Widgets/WgEditorImGuiLayer.h"
#include "IApplicationModule.h"
#include "ApplicationInstance.h"
#include "Widgets/ImGui/ImGuiManager.h"
#include "Widgets/ImGui/ImGuiLib/imgui.h"
#include "Widgets/ImGui/ImGuiLib/imgui_internal.h"
#include "Types/Platform/Threading/CoPaT/FAAArrayQueue.hpp"

void WgEditorImGuiLayer::draw(ImGuiDrawInterface *drawInterface)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    if (bShowDemo)
    {
        ImGui::ShowDemoWindow(&bShowDemo);
    }

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.0f);

    bool bShowEditor = true;
    const AChar *edWindowName = "CBEdWindow";
    ImGuiWindowFlags edWindowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
                                     | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus;

    bool bBegunEdWindow = ImGui::Begin(edWindowName, &bShowEditor, edWindowFlags);
    ImGui::PopStyleVar(2);
    if (bBegunEdWindow)
    {
        addMenubar(drawInterface);

        const ImGuiID dockNodeId = ImGui::GetID(edWindowName);
        if (!ImGui::DockBuilderGetNode(dockNodeId))
        {
            // Reset current docking state
            ImGui::DockBuilderRemoveNode(dockNodeId);
            ImGui::DockBuilderAddNode(dockNodeId, ImGuiDockNodeFlags_None);
            ImGui::DockBuilderSetNodeSize(dockNodeId, ImGui::GetMainViewport()->Size);

            ImGuiID dockViewportId;
            ImGuiID dockDetailsId;
            ImGuiID dockWorldId = ImGui::DockBuilderSplitNode(dockNodeId, ImGuiDir_Right, 0.4f, nullptr, &dockViewportId);
            dockWorldId = ImGui::DockBuilderSplitNode(dockWorldId, ImGuiDir_Right, 0.5f, nullptr, &dockDetailsId);
            ImGuiID dockContentId = ImGui::DockBuilderSplitNode(dockViewportId, ImGuiDir_Down, 0.25f, nullptr, &dockViewportId);

            // Dock windows
            ImGui::DockBuilderDockWindow("World", dockWorldId);
            ImGui::DockBuilderDockWindow("Details", dockDetailsId);
            ImGui::DockBuilderDockWindow("Console", dockContentId);
            ImGui::DockBuilderDockWindow("Contents", dockContentId);
            ImGui::DockBuilderDockWindow("Viewport", dockViewportId);

            // I want contents to be before console however contents will be rendered after console to be active at start
            ImGui::FindWindowSettings(ImHashStr("Contents"))->DockOrder = 0;
            ImGui::FindWindowSettings(ImHashStr("Console"))->DockOrder = 1;

            ImGui::DockBuilderFinish(dockViewportId);
        }

        // Have to make dock space to not draw a window bg behind docked windows
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

        ImGui::DockSpace(dockNodeId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(1);
    }
    ImGui::End();

    jobSystemJobsStats();
    aboutWindow();
}

DelegateHandle WgEditorImGuiLayer::addMenuDrawExtender(const TChar *menuName, ImGuiDrawInterfaceCallback::SingleCastDelegateType &&callback)
{
    return menuExtenders[TCHAR_TO_UTF8(menuName)].bind(std::forward<decltype(callback)>(callback));
}

void WgEditorImGuiLayer::removeMenuExtender(const TChar *menuName, DelegateHandle handle)
{
    auto itr = menuExtenders.find(TCHAR_TO_UTF8(menuName));
    if (itr != menuExtenders.end())
    {
        itr->second.unbind(handle);
        if (!itr->second)
        {
            menuExtenders.erase(itr);
        }
    }
}

void WgEditorImGuiLayer::addMenubar(ImGuiDrawInterface *drawInterface)
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
            {
                IApplicationModule::get()->getApplication()->exitNextFrame();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Developer"))
        {
            ImGui::MenuItem("CoPaT stats", nullptr, &bShowJobQueueStats);
            ImGui::MenuItem("Show ImGUI demo", nullptr, &bShowDemo);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            ImGui::MenuItem("Show About", nullptr, &bShowAbout);

            ImGui::EndMenu();
        }

        for (const std::pair<const std::string, ImGuiDrawInterfaceCallback> &callback : menuExtenders)
        {
            if (ImGui::BeginMenu(callback.first.c_str()))
            {
                callback.second.invoke(drawInterface);

                ImGui::EndMenu();
            }
        }

        ImGui::EndMenuBar();
    }
}

void WgEditorImGuiLayer::aboutWindow()
{
    if (bShowAbout)
    {
        if (ImGui::Begin("About", &bShowAbout, ImGuiManager::SIMPLE_READONLY_WINDOWFLAGS))
        {
            ApplicationInstance *appInstance = IApplicationModule::get()->getApplication();
            static const AChar *appNameText = TCHAR_TO_UTF8(appInstance->getAppName().getChar());
            static const AChar *engineNameText = "Cranberry Engine";
            static const AChar *cpyRightText = "Copyright \xC2\xA9 Jeslas Pravin, 2023";

            int32 major, minor, patch;
            appInstance->getVersion(major, minor, patch);
            static const std::string versionText = TCHAR_TO_UTF8(STR_FORMAT(TCHAR("Version {}.{}.{}"), major, minor, patch).getChar());

            static const ImVec2 totalSize = ImGui::CalcTextSize(cpyRightText);
            static const ImVec2 appNameTextSize = ImGui::CalcTextSize(appNameText);
            static const ImVec2 engineNameTextSize = ImGui::CalcTextSize(engineNameText);
            static const ImVec2 versionTextSize = ImGui::CalcTextSize(versionText.c_str());
            ImGui::SetCursorPosX(totalSize.x * 0.5f - appNameTextSize.x * 0.5f);
            ImGui::Text(appNameText);
            ImGui::SetCursorPosX(totalSize.x * 0.5f - versionTextSize.x * 0.5f);
            ImGui::Text(versionText.c_str());

            ImGui::Separator();
            ImGui::SetCursorPosX(totalSize.x * 0.5f - engineNameTextSize.x * 0.5f);
            ImGui::Text(engineNameText);
            ImGui::Text(cpyRightText);
            ImGui::Separator();
            ImGui::Text("Email : pravinjeslas@gmail.com");
            ImGui::Text("Web   : https://jeslaspravin.com");
        }
        ImGui::End();
    }
}

void WgEditorImGuiLayer::jobSystemJobsStats()
{
    if (bShowJobQueueStats)
    {
        if (ImGui::Begin("CoPaT Stats", &bShowJobQueueStats, ImGuiManager::SIMPLE_READONLY_WINDOWFLAGS))
        {
#if COPAT_ENABLE_QUEUE_ALLOC_TRACKING
            uint64 nodeSize = sizeof(copat::FAAArrayQueueNode<void>);
            DO_ONCE(nodeSize = ~0ull);
            ImGui::Text("Queue node bytes %llu", nodeSize);
            ImGui::Text("Active nodes %llu", copat::getNodeAllocsTracker().activeAllocs.load(std::memory_order::relaxed));
            ImGui::Text("In delete Queue %llu", copat::getNodeAllocsTracker().inDeleteQAllocs.load(std::memory_order::relaxed));
            ImGui::Text("Deleted count %llu", copat::getNodeAllocsTracker().deletedCount.load(std::memory_order::relaxed));

            ImGui::Text("Total reuses %llu", copat::getNodeAllocsTracker().reuseCount.load(std::memory_order::relaxed));
            ImGui::Text("Total new %llu", copat::getNodeAllocsTracker().newAllocsCount.load(std::memory_order::relaxed));
#else
            ImGui::Text("Job system stats are not compiled");
            ImGui::Text("Enable ");
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, LinearColorConst::PALE_BLUE);
            ImGui::Text("COPAT_ENABLE_QUEUE_ALLOC_TRACKING");
            ImGui::PopStyleColor();
#endif
        }
        ImGui::End();
    }
}
