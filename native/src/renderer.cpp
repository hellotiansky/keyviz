#include "keyviz/renderer.h"

#include <algorithm>
#include <iterator>

namespace keyviz {

bool Renderer::initialize(HWND hwnd) {
    hwnd_ = hwnd;

    D2D1_FACTORY_OPTIONS options{};
#ifdef _DEBUG
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
    HRESULT hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        __uuidof(ID2D1Factory),
        &options,
        reinterpret_cast<void**>(factory_.GetAddressOf()));
    if (FAILED(hr)) return false;

    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(writeFactory_.GetAddressOf()));
    if (FAILED(hr)) return false;

    hr = writeFactory_->CreateTextFormat(
        L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        30.0f, L"", &titleFormat_);
    if (FAILED(hr)) return false;

    hr = writeFactory_->CreateTextFormat(
        L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        13.0f, L"", &bodyFormat_);
    if (FAILED(hr)) return false;

    RECT rect{};
    GetClientRect(hwnd_, &rect);
    width_ = static_cast<UINT>(rect.right - rect.left);
    height_ = static_cast<UINT>(rect.bottom - rect.top);
    return createDeviceResources();
}

bool Renderer::createDeviceResources() noexcept {
    if (target_) return true;

    const D2D1_SIZE_U size = D2D1::SizeU(
        std::max<UINT>(1, width_), std::max<UINT>(1, height_));
    HRESULT hr = factory_->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN,
                              D2D1_ALPHA_MODE_IGNORE)),
        D2D1::HwndRenderTargetProperties(hwnd_, size),
        &target_);
    if (FAILED(hr)) return false;

    hr = target_->CreateSolidColorBrush(
        D2D1::ColorF(0.075f, 0.090f, 0.115f, 0.96f), &panelBrush_);
    if (FAILED(hr)) { discardDeviceResources(); return false; }
    hr = target_->CreateSolidColorBrush(
        D2D1::ColorF(0.28f, 0.66f, 1.0f, 1.0f), &accentBrush_);
    if (FAILED(hr)) { discardDeviceResources(); return false; }
    hr = target_->CreateSolidColorBrush(
        D2D1::ColorF(0.96f, 0.97f, 0.99f, 1.0f), &textBrush_);
    if (FAILED(hr)) { discardDeviceResources(); return false; }
    hr = target_->CreateSolidColorBrush(
        D2D1::ColorF(0.67f, 0.72f, 0.79f, 1.0f), &secondaryBrush_);
    if (FAILED(hr)) { discardDeviceResources(); return false; }
    return true;
}

void Renderer::resize(UINT width, UINT height) noexcept {
    width_ = width;
    height_ = height;
    if (target_) target_->Resize(D2D1::SizeU(
        std::max<UINT>(1, width), std::max<UINT>(1, height)));
}

void Renderer::render(const std::deque<InputEvent>& history) noexcept {
    if (!factory_ || !createDeviceResources()) return;

    target_->BeginDraw();
    target_->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

    const D2D1_SIZE_F size = target_->GetSize();
    const D2D1_RECT_F panel = D2D1::RectF(
        1.0f, 1.0f, size.width - 1.0f, size.height - 1.0f);
    target_->FillRoundedRectangle(
        D2D1::RoundedRect(panel, 16.0f, 16.0f), panelBrush_.Get());
    target_->DrawRoundedRectangle(
        D2D1::RoundedRect(panel, 16.0f, 16.0f), accentBrush_.Get(), 1.0f);

    const DWORD now = GetTickCount();
    float opacity = 1.0f;
    if (!history.empty()) {
        const DWORD age = now - history.front().timestamp;
        constexpr DWORD fadeDuration = 1100;
        opacity = age < fadeDuration
            ? std::clamp(1.0f - static_cast<float>(age) /
                static_cast<float>(fadeDuration), 0.0f, 1.0f)
            : 0.28f;
    }

    textBrush_->SetOpacity(opacity);
    accentBrush_->SetOpacity(opacity);
    const InputEvent* latestMeaningful = nullptr;
    const InputEvent* latestPointer = nullptr;
    for (const auto& event : history) {
        if (!latestPointer && event.type == InputEvent::Type::MouseMove)
            latestPointer = &event;
        if (event.type != InputEvent::Type::MouseMove &&
            event.type != InputEvent::Type::KeyUp &&
            event.type != InputEvent::Type::MouseUp) {
            latestMeaningful = &event;
            break;
        }
    }
    const std::wstring headline = latestMeaningful
        ? latestMeaningful->label
        : (latestPointer ? L"Mouse movement" : L"Waiting for keyboard or mouse input...");
    target_->DrawText(headline.c_str(),
        static_cast<UINT32>(headline.size()), titleFormat_.Get(),
        D2D1::RectF(22.0f, 13.0f, size.width - 20.0f, 57.0f),
        textBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);

    secondaryBrush_->SetOpacity(0.92f);
    std::wstring pointerText = L"Ctrl + Shift + K   |   Native settings";
    if (latestPointer) {
        pointerText = L"POINTER   X " + std::to_wstring(latestPointer->screenX) +
            L"   Y " + std::to_wstring(latestPointer->screenY);
    }
    target_->DrawText(pointerText.c_str(),
        static_cast<UINT32>(pointerText.size()), bodyFormat_.Get(),
        D2D1::RectF(22.0f, 69.0f, size.width - 20.0f, 94.0f),
        secondaryBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);

    std::wstring recent = L"RECENT   ";
    unsigned count = 0;
    for (const auto& event : history) {
        if (event.type != InputEvent::Type::KeyDown &&
            event.type != InputEvent::Type::MouseDown &&
            event.type != InputEvent::Type::Wheel) continue;
        if (count) recent += L"    /    ";
        recent += event.label;
        if (++count == 5) break;
    }
    target_->DrawText(recent.c_str(),
        static_cast<UINT32>(recent.size()), bodyFormat_.Get(),
        D2D1::RectF(22.0f, 111.0f, size.width - 20.0f, 140.0f),
        secondaryBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);

    // A compact pointer glyph reinforces that movement capture is active.
    if (latestPointer) {
        const float cx = size.width - 34.0f;
        const float cy = 82.0f;
        target_->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), 7.0f, 7.0f),
            accentBrush_.Get(), 2.0f);
        target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), 2.0f, 2.0f),
            accentBrush_.Get());
    }

    const HRESULT hr = target_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) discardDeviceResources();
}

void Renderer::discardDeviceResources() noexcept {
    secondaryBrush_.Reset();
    textBrush_.Reset();
    accentBrush_.Reset();
    panelBrush_.Reset();
    target_.Reset();
}

void Renderer::shutdown() noexcept {
    discardDeviceResources();
    titleFormat_.Reset();
    bodyFormat_.Reset();
    writeFactory_.Reset();
    factory_.Reset();
    hwnd_ = nullptr;
}

} // namespace keyviz
