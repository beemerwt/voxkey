#pragma once

#include <functional>
#include <string>

namespace voxkey {

int run_hotkey_loop(
    const std::string& keysym_name,
    const std::function<void()>& on_press,
    const std::function<void()>& on_release);

}  // namespace voxkey
