#pragma once

#include "mandelbrot_fractal_utils.hpp"
#include "types_sfml.hpp"
#include <print>

#include <stdexec/execution.hpp>

using namespace std::chrono_literals;
namespace ex = stdexec;

namespace mandelbrot {

static auto MakeComputeSender(RenderSettings settings, ViewPort viewport) {
    static AvrTimeCounter time_counter;
    static FrameBuffer fb = FrameBuffer::Make(settings.width, settings.height);

    return ex::just() | ex::then([=]{
        time_counter.Start();

        for (auto x = 0; x < fb.width; ++x) {
            for (auto y = 0; y < fb.height; ++y) {
                auto complex = Pixel2DToComplex(x, y, viewport, fb.width, fb.height);
                auto iterations = CalculateIterationsForPoint(
                    complex, 
                    settings.max_iterations, 
                    settings.escape_radius
                );
                auto color = IterationsToColor(iterations, settings.max_iterations);
                auto idx = (x + y * fb.width) * 4;
                fb.rgba[idx + 0] = color.r;
                fb.rgba[idx + 1] = color.g;
                fb.rgba[idx + 2] = color.b;
                fb.rgba[idx + 3] = 0xff;
            }
        }

        return &fb;
    }) |
    ex::then([](FrameBuffer *fb) {
        time_counter.End();
        if (time_counter.Count() % 10 == 0) {
            std::println("\nAverage compute time: {} ms over {} frames", time_counter.GetAvr(),
                        time_counter.Count());
        }
        return fb;
    });
}


}  // namespace mandelbrot