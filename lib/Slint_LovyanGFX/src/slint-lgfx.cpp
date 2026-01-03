// See: https://github.com/slint-ui/slint/blob/ce50ea806a9a1d512d30acab6f99c8a1d511505f/api/cpp/esp-idf/slint/src/slint-esp.cpp
#include <deque>
#include <mutex>
#include "slint-lgfx.h"
#include "slint-platform.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "slint_platform";

using RepaintBufferType = slint::platform::SoftwareRenderer::RepaintBufferType;

class LgfxWindowAdapter : public slint::platform::WindowAdapter
{
public:
    slint::platform::SoftwareRenderer m_renderer;
    bool needs_redraw = true;
    const slint::PhysicalSize m_size;

    explicit LgfxWindowAdapter(RepaintBufferType buffer_type, slint::PhysicalSize size)
        : m_renderer(buffer_type), m_size(size)
    {
    }

    slint::platform::AbstractRenderer &renderer() override { return m_renderer; }

    slint::PhysicalSize size() override { return m_size; }

    void request_redraw() override { needs_redraw = true; }
};

template <typename PixelType>
struct LgfxPlatform : public slint::platform::Platform
{
    LgfxPlatform(const SlintPlatformConfiguration<PixelType> &config)
        : size(config.size),
          gfx(config.gfx),
          buffer1(config.buffer1),
          buffer2(config.buffer2),
          byte_swap(config.byte_swap),
          rotation(config.rotation)
    {
        task = xTaskGetCurrentTaskHandle();
    }

    std::unique_ptr<slint::platform::WindowAdapter> create_window_adapter() override;

    std::chrono::milliseconds duration_since_start() override;
    void run_event_loop() override;
    void quit_event_loop() override;
    void run_in_event_loop(Task) override;

private:
    slint::PhysicalSize size;
    lgfx::LGFX_Device* gfx;
    std::optional<std::span<PixelType>> buffer1;
    std::optional<std::span<PixelType>> buffer2;
    bool byte_swap;
    slint::platform::SoftwareRenderer::RenderingRotation rotation;
    class LgfxWindowAdapter *m_window = nullptr;

    static TaskHandle_t task;
    std::mutex queue_mutex;
    std::deque<slint::platform::Platform::Task> queue;
    bool quit = false;
};

template <typename PixelType>
std::unique_ptr<slint::platform::WindowAdapter> LgfxPlatform<PixelType>::create_window_adapter()
{
    if (m_window != nullptr)
    {
        ESP_LOGI(TAG, "FATAL: create_window_adapter called multiple times");
        return nullptr;
    }

    auto buffer_type =
        buffer2 ? RepaintBufferType::SwappedBuffers : RepaintBufferType::ReusedBuffer;
    auto window = std::make_unique<LgfxWindowAdapter>(buffer_type, size);
    m_window = window.get();
    m_window->m_renderer.set_rendering_rotation(rotation);
    return window;
}

template <typename PixelType>
std::chrono::milliseconds LgfxPlatform<PixelType>::duration_since_start()
{
    auto ticks = xTaskGetTickCount();
    return std::chrono::milliseconds(pdTICKS_TO_MS(ticks));
}

namespace
{
    void byte_swap_color(slint::platform::Rgb565Pixel *pixel)
    {
        auto px = reinterpret_cast<uint16_t *>(pixel);
        *px = (*px << 8) | (*px >> 8);
    }
    void byte_swap_color(slint::Rgb8Pixel *pixel)
    {
        std::swap(pixel->r, pixel->b);
    }
}

template <typename PixelType>
void LgfxPlatform<PixelType>::run_event_loop()
{
    TickType_t max_ticks_to_wait = pdMS_TO_TICKS(10);

    float last_touch_x = 0;
    float last_touch_y = 0;
    bool touch_down = false;

    while (true)
    {
        slint::platform::update_timers_and_animations();

        std::optional<slint::platform::Platform::Task> event;
        {
            std::unique_lock lock(queue_mutex);
            if (queue.empty())
            {
                if (quit)
                {
                    quit = false;
                    break;
                }
            }
            else
            {
                event = std::move(queue.front());
                queue.pop_front();
            }
        }
        if (event)
        {
            std::move(*event).run();
            event.reset();
            continue;
        }

        if (m_window)
        {
            int32_t touch_x = 0, touch_y = 0;
            bool touched = false;
            if (gfx) {
                 touched = gfx->getTouch(&touch_x, &touch_y);
            }

            if (touched)
            {
                auto scale_factor = m_window->window().scale_factor();
                last_touch_x = float(touch_x) / scale_factor;
                last_touch_y = float(touch_y) / scale_factor;
                
                m_window->window().dispatch_pointer_move_event(
                    slint::LogicalPosition({last_touch_x, last_touch_y}));
                
                if (!touch_down)
                {
                    m_window->window().dispatch_pointer_press_event(
                        slint::LogicalPosition({last_touch_x, last_touch_y}),
                        slint::PointerEventButton::Left);
                }
                touch_down = true;
            }
            else if (touch_down)
            {
                m_window->window().dispatch_pointer_release_event(
                    slint::LogicalPosition({last_touch_x, last_touch_y}),
                    slint::PointerEventButton::Left);
                m_window->window().dispatch_pointer_exit_event();
                touch_down = false;
            }

            if (std::exchange(m_window->needs_redraw, false))
            {
                using slint::platform::SoftwareRenderer;
                auto rotated = rotation == SoftwareRenderer::RenderingRotation::Rotate90 || rotation == SoftwareRenderer::RenderingRotation::Rotate270;
                auto stride = rotated ? size.height : size.width;
                
                if (gfx) gfx->startWrite();

                if (buffer1)
                {
                    auto region = m_window->m_renderer.render(buffer1.value(), stride);

                    if (byte_swap)
                    {
                        for (auto [o, s] : region.rectangles())
                        {
                            for (int y = o.y; y < o.y + s.height; y++)
                            {
                                for (int x = o.x; x < o.x + s.width; x++)
                                {
                                    byte_swap_color(&buffer1.value()[y * stride + x]);
                                }
                            }
                        }
                    }

                    if (buffer2)
                    {
                        for (auto [o, s] : region.rectangles())
                        {
                             if (gfx) gfx->pushImage(o.x, o.y, s.width, s.height, (const uint16_t*)(buffer1->data() + o.y * stride + o.x));
                        }
                        
                        std::swap(buffer1, buffer2);
                    }
                    else
                    {
                        for (auto [o, s] : region.rectangles())
                        {
                            if (gfx) gfx->pushImage(o.x, o.y, s.width, s.height, (const uint16_t*)(buffer1->data() + o.y * stride + o.x));
                        }
                    }
                }
                else
                {
                    // Line by line
                    using Uniq = std::unique_ptr<PixelType, void (*)(void *)>;
                    auto alloc = [&]
                    {
                        void *ptr = heap_caps_malloc(stride * sizeof(PixelType),
                                                     MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
                        if (!ptr)
                        {
                            ESP_LOGE(TAG, "malloc failed to allocate line buffer");
                            abort();
                        }
                        return Uniq(reinterpret_cast<PixelType *>(ptr), heap_caps_free);
                    };
                    Uniq lb[1] = {alloc()};
                    
                    m_window->m_renderer.render_by_line<PixelType>(
                        [this, &lb](std::size_t line_y, std::size_t line_start,
                                          std::size_t line_end, auto &&render_fn)
                        {
                            std::span<PixelType> view{lb[0].get(), line_end - line_start};
                            render_fn(view);
                            if (byte_swap)
                            {
                                std::for_each(view.begin(), view.end(),
                                              [](auto &rgbpix)
                                              { byte_swap_color(&rgbpix); });
                            }
                            
                            if (gfx) gfx->pushImage(line_start, line_y, line_end - line_start, 1, (const uint16_t*)view.data());
                        });
                }
                
                if (gfx) gfx->endWrite();
            }

            if (m_window->window().has_active_animations())
            {
                continue;
            }
        }

        TickType_t ticks_to_wait = max_ticks_to_wait;
        if (auto wait_time = slint::platform::duration_until_next_timer_update())
        {
            ticks_to_wait = std::min(ticks_to_wait, pdMS_TO_TICKS(wait_time->count()));
        }

        ulTaskNotifyTake(/*reset to zero*/ pdTRUE, ticks_to_wait);
    }

    vTaskDelete(NULL);
}

template <typename PixelType>
void LgfxPlatform<PixelType>::quit_event_loop()
{
    {
        const std::unique_lock lock(queue_mutex);
        quit = true;
    }
    vTaskNotifyGiveFromISR(task, nullptr);
}

template <typename PixelType>
void LgfxPlatform<PixelType>::run_in_event_loop(slint::platform::Platform::Task event)
{
    {
        const std::unique_lock lock(queue_mutex);
        queue.push_back(std::move(event));
    }
    vTaskNotifyGiveFromISR(task, nullptr);
}

template <typename PixelType>
TaskHandle_t LgfxPlatform<PixelType>::task = {};

void slint_esp_init(const SlintPlatformConfiguration<slint::platform::Rgb565Pixel> &config)
{
    slint::platform::set_platform(
        std::make_unique<LgfxPlatform<slint::platform::Rgb565Pixel>>(config));
}

void slint_esp_init(const SlintPlatformConfiguration<slint::Rgb8Pixel> &config)
{
    slint::platform::set_platform(std::make_unique<LgfxPlatform<slint::Rgb8Pixel>>(config));
}
