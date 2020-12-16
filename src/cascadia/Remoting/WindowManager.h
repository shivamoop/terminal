#pragma once

#include "WindowManager.g.h"
#include "Peasant.h"
#include "Monarch.h"
#include "../cascadia/inc/cppwinrt_utils.h"

namespace winrt::Microsoft::Terminal::Remoting::implementation
{
    struct WindowManager : public WindowManagerT<WindowManager>
    {
        WindowManager();
        ~WindowManager();

        void ProposeCommandline(array_view<const winrt::hstring> args, const winrt::hstring cwd);
        bool ShouldCreateWindow();

        winrt::Microsoft::Terminal::Remoting::Peasant CurrentWindow();

    private:
        bool _shouldCreateWindow{ false };
        DWORD _registrationHostClass{ 0 };
        winrt::Microsoft::Terminal::Remoting::Monarch _monarch{ nullptr };
        winrt::Microsoft::Terminal::Remoting::Peasant _peasant{ nullptr };

        void _registerAsMonarch();
        void _createMonarch();
        bool _areWeTheKing();
        winrt::Microsoft::Terminal::Remoting::IPeasant _createOurPeasant();
    };
}

namespace winrt::Microsoft::Terminal::Remoting::factory_implementation
{
    BASIC_FACTORY(WindowManager);
}
