#pragma once

#include "keyviz/input_event.h"

#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

#include <deque>

namespace keyviz {

class Renderer {
public:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool initialize(HWND hwnd);
    void resize(UINT width, UINT height) noexcept;
    void render(const std::deque<InputEvent>& history) noexcept;
    void shutdown() noexcept;

private:
    bool createDeviceResources() noexcept;
    void discardDeviceResources() noexcept;

    HWND hwnd_{};
    UINT width_{};
    UINT height_{};
    Microsoft::WRL::ComPtr<ID2D1Factory> factory_;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> target_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> panelBrush_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> accentBrush_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> textBrush_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> secondaryBrush_;
    Microsoft::WRL::ComPtr<IDWriteFactory> writeFactory_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> titleFormat_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> bodyFormat_;
};

} // namespace keyviz
