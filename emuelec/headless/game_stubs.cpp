// Sustitutos para EmuELEC (etapa 1b, sin ventana): lo que src/common/game_info.cpp, keyboard.cpp y cheats.cpp dan
// en Android pero que dependen de ImGui, SDL3 y JNI. Define app::kGame (sin la interfaz ImGui ni los controles
// tactiles) y un PadHook vacio; el mando real lo pondra la etapa 3 con SDL2.
#include "app/app.h"
#include "common/options.h"
#include "core/input.h"

namespace game {
void InstallCaptureSignal();
void Init() { InstallCaptureSignal(); }
void PadHook(int, input::Pad&) {}
}  // namespace game

const app::GameInfo app::kGame = {
    .title = L"Pro Evolution Soccer 5",
    .short_title = L"PES5",
    .name = L"PES5",
    .log = "pes5",
    .settings = "settings.toml",
    .splash = L"splash",
    .missing = L"Put the game in the data folder: default.xbe with its files beside it.",
    .images = {{"default.xbe", "PES5"}},
    .options = {},
    .load = nullptr,
    .save = nullptr,
    .init = game::Init,
    .loaded = nullptr,
    .window = nullptr,
    .message = nullptr,
    .ui = nullptr,
    .pad = game::PadHook,
    .rumble = nullptr,
};
