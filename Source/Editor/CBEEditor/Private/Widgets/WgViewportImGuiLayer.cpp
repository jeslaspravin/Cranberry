/*!
 * \file WgViewportImGuiLayer.cpp
 *
 * \author Jeslas
 * \date August 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Widgets/WgViewportImGuiLayer.h"
#include "Widgets/ImGui/ImGuiLib/imgui.h"
#include "Widgets/ImGui/ImGuiLib/imgui_internal.h"
#include "Widgets/WidgetDrawContext.h"
#include "Widgets/WidgetWindow.h"
#include "WorldViewport.h"
#include "IApplicationModule.h"
#include "InputSystem/InputSystem.h"
#include "ApplicationInstance.h"

WgViewportImGuiLayer::WgViewportImGuiLayer()
{
    defaultCamera.cameraProjection = ECameraProjection::Perspective;
    defaultCamera.setClippingPlane(0.1f, 6000.f);

    //Vector3D camTranslation = Vector3D(0.f, 1.f, 0.0f).safeNormalized() * (500);
    Vector3D camTranslation; 
    camTranslation.z() += 200;

    defaultCamera.setTranslation(camTranslation);
    //defaultCamera.lookAt(Vector3D::ZERO);
}

void WgViewportImGuiLayer::draw(ImGuiDrawInterface *drawInterface)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    // NoMove since we draw the output image directly to viewport, and moving window will make output drawn below other windows
    // TODO(Jeslas): May be find if having separate ImGui draw list helps with this? or use UserCallback to render the scene output?
    bDrawingViewport = ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove);
    ImGui::PopStyleVar(2);
    if (bDrawingViewport)
    {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        viewportRegion.minBound.x = Math::max(0, int16(pos.x));
        viewportRegion.minBound.y = Math::max(0, int16(pos.y));
        viewportRegion.maxBound.x = Math::max(0, int16(pos.x + viewportSize.x));
        viewportRegion.maxBound.y = Math::max(0, int16(pos.y + viewportSize.y));

        ImGui::PushStyleColor(ImGuiCol_Text, ColorConst::RED);
        ImGui::PushStyleColor(ImGuiCol_Border, ColorConst::GREEN);
        if (ImGui::IsWindowHovered())
        {
            ImGui::Text("Viewport hovered");
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::GetIO().MouseDownDurationPrev[ImGuiMouseButton_Right] <= 0.25f)
            {
                ImGui::Text("Viewport right clicked");
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                ImGui::FocusWindow(ImGui::GetCurrentWindow());
            }
        }
        if (ImGui::IsWindowFocused())
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !ImGui::IsWindowHovered())
            {
                ImGui::FocusWindow(ImGui::GetCurrentContext()->HoveredWindow);
            }

            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
            {
                ImGui::FocusWindow(ImGui::GetCurrentWindow());
                // ImGui::Text("Viewport right dragged");
                navigateScene();
            }
        }
        ImGui::PopStyleColor(2);
    }
    ImGui::End();
}

bool WgViewportImGuiLayer::drawDirect(const DrawDirectParams &params)
{
    if (bDrawingViewport && viewportRegion.isValidAABB() && worldViewport)
    {
        SharedPtr<WgWindow> wndw = findWidgetParentWindow(shared_from_this());
        QuantShortBox2D drawRegion{ wndw->applyDpiScale(viewportRegion.minBound), wndw->applyDpiScale(viewportRegion.maxBound) };
        worldViewport->drawBackBuffer(drawRegion, params.rt, params.cmdBuffer, params.cmdList, params.graphicsInstance, params.graphicsHelper);
        params.inOutClearRt = false;
        return true;
    }
    return false;
}

void WgViewportImGuiLayer::drawOnImGui(WidgetDrawContext &context) {}

void WgViewportImGuiLayer::drawWidget(
    QuantShortBox2D clipBound, WidgetGeomId thisId, const WidgetGeomTree &geomTree, WidgetDrawContext &context
)
{
    if (bDrawingViewport && viewportRegion.isValidAABB())
    {
        if (worldViewport)
        {
            SharedPtr<WgWindow> wndw = findWidgetParentWindow(shared_from_this());
            Short2D viewportSize = viewportRegion.size();
            defaultCamera.setFOV((110.f * viewportSize.x) / (viewportSize.y * 1.78f), 90.f);
            worldViewport->startSceneRender(wndw->applyDpiScale(viewportSize), defaultCamera);
        }
        else
        {
            context.drawBox(viewportRegion, nullptr, viewportRegion, ColorConst::BLACK);
        }
    }
}

void WgViewportImGuiLayer::navigateScene()
{
    ApplicationInstance *application = IApplicationModule::get()->getApplication();
    Rotation cameraRotation = defaultCamera.rotation();
    Vector3D cameraTranslation = defaultCamera.translation();

    cameraRotation.yaw() += application->inputSystem->analogState(AnalogStates::RelMouseX)->currentValue * 0.25f;
    cameraRotation.pitch() += application->inputSystem->analogState(AnalogStates::RelMouseY)->currentValue * 0.25f;

    float camSpeedModifier = 1;
    if (application->inputSystem->isKeyPressed(Keys::LSHIFT))
    {
        camSpeedModifier = 2;
    }
    if (application->inputSystem->isKeyPressed(Keys::A))
    {
        cameraTranslation -= cameraRotation.rightVector() * ImGui::GetIO().DeltaTime * camSpeedModifier * 150.f;
    }
    if (application->inputSystem->isKeyPressed(Keys::D))
    {
        cameraTranslation += cameraRotation.rightVector() * ImGui::GetIO().DeltaTime * camSpeedModifier * 150.f;
    }
    if (application->inputSystem->isKeyPressed(Keys::W))
    {
        cameraTranslation += cameraRotation.fwdVector() * ImGui::GetIO().DeltaTime * camSpeedModifier * 150.f;
    }
    if (application->inputSystem->isKeyPressed(Keys::S))
    {
        cameraTranslation -= cameraRotation.fwdVector() * ImGui::GetIO().DeltaTime * camSpeedModifier * 150.f;
    }
    if (application->inputSystem->isKeyPressed(Keys::Q))
    {
        cameraTranslation -= Vector3D::UP * ImGui::GetIO().DeltaTime * camSpeedModifier * 150.f;
    }
    if (application->inputSystem->isKeyPressed(Keys::E))
    {
        cameraTranslation += Vector3D::UP * ImGui::GetIO().DeltaTime * camSpeedModifier * 150.f;
    }
    if (application->inputSystem->keyState(Keys::R)->keyWentUp)
    {
        cameraRotation = RotationMatrix::fromZX(Vector3D::UP, cameraRotation.fwdVector()).asRotation();
    }

    defaultCamera.setRotation(cameraRotation);
    defaultCamera.setTranslation(cameraTranslation);
}
