// See: https://github.com/slint-ui/slint/blob/ce50ea806a9a1d512d30acab6f99c8a1d511505f/api/cpp/esp-idf/slint/include/slint-esp.h
#pragma once

#include "slint-platform.h"
#include <LovyanGFX.hpp>
#include <vector>
#include <span>
#include <optional>

/**
 * This data structure configures the Slint platform for use with LovyanGFX.
 */
template <typename PixelType = slint::platform::Rgb565Pixel>
struct SlintPlatformConfiguration
{
    /// The size of the screen in pixels.
    slint::PhysicalSize size;
    /// The pointer to the LovyanGFX instance.
    lgfx::LGFX_Device* gfx = nullptr;
    
    /// The buffer Slint will render into. It must have have the size of at least one frame.
    std::optional<std::span<PixelType>> buffer1 = {};
    /// If specified, this is a second buffer that will be used for double-buffering.
    std::optional<std::span<PixelType>> buffer2 = {};
    
    slint::platform::SoftwareRenderer::RenderingRotation rotation =
        slint::platform::SoftwareRenderer::RenderingRotation::NoRotation;
        
    bool byte_swap = false;
};

template <typename... Args>
SlintPlatformConfiguration(Args...) -> SlintPlatformConfiguration<>;

/**
 * Initialize the Slint platform for LovyanGFX.
 *
 * This must be called before any other call to the Slint library.
 */
void slint_esp_init(const SlintPlatformConfiguration<slint::platform::Rgb565Pixel> &config);
void slint_esp_init(const SlintPlatformConfiguration<slint::Rgb8Pixel> &config);
