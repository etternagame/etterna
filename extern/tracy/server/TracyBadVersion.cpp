#include <assert.h>

#include "IconsFontAwesome5.h"
#include "TracyBadVersion.hpp"
#include "TracyImGui.hpp"

namespace tracy
{

namespace detail
{

void BadVersionImpl( BadVersionState& badVer )
{
    assert( badVer.state != BadVersionState::Ok );

    switch( badVer.state )
    {
    case BadVersionState::BadFile:
        ImGui::OpenPopup( "Bad file" );
        break;
    case BadVersionState::ReadError:
        ImGui::OpenPopup( "File read error" );
        break;
    case BadVersionState::UnsupportedVersion:
        ImGui::OpenPopup( "Unsupported file version" );
        break;
    case BadVersionState::LegacyVersion:
        ImGui::OpenPopup( "Legacy file version" );
        break;
    default:
        assert( false );
        break;
    }
    if( ImGui::BeginPopupModal( "Bad file", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
    {
        TextCentered( ICON_FA_EXCLAMATION_TRIANGLE );
        ImGui::Text( "The file you are trying to open is not a tracy dump." );
        ImGui::Separator();
        if( ImGui::Button( "Oops" ) )
        {
            ImGui::CloseCurrentPopup();
            badVer.state = BadVersionState::Ok;
        }
        ImGui::EndPopup();
    }
    if( ImGui::BeginPopupModal( "File read error", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
    {
        TextCentered( ICON_FA_EXCLAMATION_TRIANGLE );
        ImGui::Text( "The file you are trying to open cannot be mapped to memory." );
        ImGui::Separator();
        if( ImGui::Button( "OK" ) )
        {
            ImGui::CloseCurrentPopup();
            badVer.state = BadVersionState::Ok;
        }
        ImGui::EndPopup();
    }
    if( ImGui::BeginPopupModal( "Unsupported file version", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
    {
        TextCentered( ICON_FA_CLOUD_DOWNLOAD_ALT );
        ImGui::Text( "The file you are trying to open is unsupported.\nYou should update to tracy %i.%i.%i or newer and try again.", badVer.version >> 16, ( badVer.version >> 8 ) & 0xFF, badVer.version & 0xFF );
        ImGui::Separator();
        if( ImGui::Button( "I understand" ) )
        {
            ImGui::CloseCurrentPopup();
            badVer.state = BadVersionState::Ok;
        }
        ImGui::EndPopup();
    }
    if( ImGui::BeginPopupModal( "Legacy file version", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
    {
        TextCentered( ICON_FA_GHOST );
        ImGui::Text( "You are trying to open a file which was created by legacy version %i.%i.%i.\nUse the update utility from an older version of the profiler to convert the file to a supported version.", badVer.version >> 16, ( badVer.version >> 8 ) & 0xFF, badVer.version & 0xFF );
        ImGui::Separator();
        if( ImGui::Button( "Maybe I don't need it" ) )
        {
            ImGui::CloseCurrentPopup();
            badVer.state = BadVersionState::Ok;
        }
        ImGui::EndPopup();
    }
}

}

}
