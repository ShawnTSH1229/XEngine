#include "EditorUI.h"

#include <imgui.h>
#include <imgui_internal.h>

void XEditorUI::ShowEditorUI()
{
	bool bEditorMenuWindowOpen = true;
    bool bAssetWindowOpen = true;
    bool bGameEngineWindowOpen = true;
    bool bFileContentWindowOpen = true;
    bool bDetailWindowOpen = true;

	ShowEditorMenu(&bEditorMenuWindowOpen);
    
    //ShowEditorWorldObjectsWindow(&bAssetWindowOpen);
    ShowEditorGameWindow(&bGameEngineWindowOpen);
    //ShowEditorFileContentWindow(&bFileContentWindowOpen);
    //ShowEditorDetialWindow(&bDetailWindowOpen);
}

void XEditorUI::ShowEditorDetialWindow(bool* bOpen)
{
    ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_None;

    const ImGuiViewport* MainViewport = ImGui::GetMainViewport();

    if (!ImGui::Begin("Components Details", bOpen, WindowFlags))
    {
        ImGui::End();
        return;
    }
    ImGui::End();
}
void XEditorUI::ShowEditorWorldObjectsWindow(bool* bOpen)
{
    ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_None;

    const ImGuiViewport* MainViewport = ImGui::GetMainViewport();

    if (!ImGui::Begin("World Objects", bOpen, WindowFlags))
    {
        ImGui::End();
        return;
    }
    ImGui::End();

}

void XEditorUI::ShowEditorGameWindow(bool* bOpen)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar;

    const ImGuiViewport* MainViewport = ImGui::GetMainViewport();

    if (!ImGui::Begin("Game Engine", bOpen, WindowFlags))
    {
        ImGui::End();
        return;
    }

    ImGui::End();
}

void XEditorUI::ShowEditorMenu(bool* bOpen)
{
	ImGuiDockNodeFlags DockFlag = ImGuiDockNodeFlags_DockSpace;
	ImGuiWindowFlags   WindowFlag = 
		ImGuiWindowFlags_MenuBar				| ImGuiWindowFlags_NoTitleBar				| ImGuiWindowFlags_NoCollapse	|
		ImGuiWindowFlags_NoResize				| ImGuiWindowFlags_NoMove					| ImGuiWindowFlags_NoBackground |
		ImGuiConfigFlags_NoMouseCursorChange	| ImGuiWindowFlags_NoBringToFrontOnFocus									;

	const ImGuiViewport* MainViewPort = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(MainViewPort->WorkPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(WindowWidth, WindowHeight), ImGuiCond_Always);
	ImGui::SetNextWindowViewport(MainViewPort->ID);

	ImGui::Begin("Editor Menu", bOpen, WindowFlag);
	ImGuiID MainDockingID = ImGui::GetID("MainDocking");
    if (ImGui::DockBuilderGetNode(MainDockingID) == nullptr)
    {
        ImGui::DockBuilderRemoveNode(MainDockingID);

        ImGui::DockBuilderAddNode(MainDockingID, DockFlag);
        ImGui::DockBuilderSetNodePos(MainDockingID,
            ImVec2(MainViewPort->WorkPos.x, MainViewPort->WorkPos.y + 18.0f));
        ImGui::DockBuilderSetNodeSize(MainDockingID, ImVec2(WindowWidth, WindowHeight - 18.0f));

        ImGuiID Center = MainDockingID;
        ImGuiID Left;
        ImGuiID Right = ImGui::DockBuilderSplitNode(Center, ImGuiDir_Right, 0.25f, nullptr, &Left);

        ImGuiID LeftOther;
        ImGuiID LeftFileContent = ImGui::DockBuilderSplitNode(Left, ImGuiDir_Down, 0.30f, nullptr, &LeftOther);

        ImGuiID LeftGameEngine;
        ImGuiID LeftAsset =
            ImGui::DockBuilderSplitNode(LeftOther, ImGuiDir_Left, 0.30f, nullptr, &LeftGameEngine);

        ImGui::DockBuilderDockWindow("World Objects", LeftAsset);
        ImGui::DockBuilderDockWindow("Components Details", Right);
        ImGui::DockBuilderDockWindow("File Content", LeftFileContent);
        ImGui::DockBuilderDockWindow("Game Engine", LeftGameEngine);

        ImGui::DockBuilderFinish(MainDockingID);
    }

    ImGui::DockSpace(MainDockingID);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Menu"))
        {
            //if (ImGui::MenuItem("Reload Current Level"))
            //{
            //    WorldManager::getInstance().reloadCurrentLevel();
            //    onGObjectSelected(PILOT_INVALID_GOBJECT_ID);
            //}
            //if (ImGui::MenuItem("Save Current Level"))
            //{
            //    WorldManager::getInstance().saveCurrentLevel();
            //}
            if (ImGui::MenuItem("Exit"))
            {
                exit(0);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

void XEditorUI::ShowEditorFileContentWindow(bool* bOpen)
{
    ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_None;

    const ImGuiViewport* MainViewport = ImGui::GetMainViewport();

    if (!ImGui::Begin("File Content", bOpen, WindowFlags))
    {
        ImGui::End();
        return;
    }

    static ImGuiTableFlags Flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_NoBordersInBody;

    if (ImGui::BeginTable("File Content", 2, Flags))
    {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow();
       //Pilot::EditorFileService editor_file_service;
       //editor_file_service.buildEngineFileTree();
       //EditorFileNode* editor_root_node = editor_file_service.getEditorRootNode();
       //buildEditorFileAssstsUITree(editor_root_node);
        ImGui::EndTable();
    }

    // file image list

    ImGui::End();
}

void XEditorUI::OnTick()
{
	ShowEditorUI();
}
