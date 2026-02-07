#include <gtest/gtest.h>

#include <cstdint>
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include "mandelbrot_sender.hpp"
#include "types_sfml.hpp"

struct ReceiverState {
    FrameBuffer *value = nullptr;
    bool stoppedCalled = false;
    bool errorCalled = false;
};

///
///
///

struct TestReceiver {
    using receiver_concept = ex::receiver_t;
    ReceiverState *state{};

    void set_value(FrameBuffer *fb) noexcept {
        state->value = fb;
    }

    void set_error(std::exception_ptr) noexcept { 
        state->errorCalled = true; 
    }

    void set_stopped() noexcept { 
        state->stoppedCalled = true; 
    }

    auto get_env() const noexcept { 
        return ex::env<>{}; 
    }
};

///
///
///

class MandelbrotTestGroup : public testing::Test {
protected:
    RenderSettings settings{.width = 10, .height = 10, .max_iterations = 10, .escape_radius = 2.0};
    ViewPort viewport{-1.0, 1.0, -1.0, 1.0};
};

///
///
///

TEST_F(MandelbrotTestGroup, SenderReceiverConnect) {
    auto sender = mandelbrot::MakeComputeSender(settings, viewport);

    ReceiverState state{};
    auto op = ex::connect(std::move(sender), TestReceiver{&state});

    ex::start(op);
    /// fb is created inside sender
    EXPECT_FALSE(state.value == nullptr);
    EXPECT_FALSE(state.stoppedCalled);
    EXPECT_FALSE(state.errorCalled);
}

///
///
///

TEST_F(MandelbrotTestGroup, GetFirstResult) {
    exec::static_thread_pool pool{ 1 };
    auto sched = pool.get_scheduler();

    auto sender = mandelbrot::MakeComputeSender(settings, viewport) |
        ex::then([](FrameBuffer *fb) { return fb->rgba[0]; });

    auto result = ex::sync_wait(ex::starts_on(sched, std::move(sender)));

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(std::get<0>(*result) != 0);
}

///
///
///

TEST_F(MandelbrotTestGroup, CheckFullBuffer) {
    auto sender = mandelbrot::MakeComputeSender(settings, viewport);
    auto result = ex::sync_wait(std::move(sender));

    EXPECT_TRUE(result.has_value());

    auto &rgba = std::get<0>(*result)->rgba;
    EXPECT_EQ(400, rgba.size());

    bool st = true;
    for (int i = 3; i < rgba.size(); i += 4) {
        st &= rgba[i] == 0xff;
    }
    EXPECT_TRUE(st);
}

///
///
///