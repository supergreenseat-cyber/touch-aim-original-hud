#include "mods/service.hpp"
#include "mods/svc/hook.hpp"
#include "mods/svc/ui.h"
#include "dusk/settings.h"
#include "d/d_meter2_draw.h"

DEFINE_MOD();
IMPORT_SERVICE(HookService, svc_hook);
IMPORT_SERVICE(UiService, svc_ui);
DEFINE_HOOK_SYMBOL("dMeter2Draw_c::draw", void(dMeter2Draw_c*), HudDraw);

static UiStyleHandle style{};
static constexpr char css[] = R"RCSS(
body#root #button-a, body#root #button-b,
body#root #button-x, body#root #button-y, body#root #button-z,
body#root #trigger-l, body#root #trigger-r, body#root #skip,
body#root #control-stick, body#root #action-bar,
body#root #items, body#root #first-person, body#root #map,
body#root #collections, body#root #action-bar separator {
    filter: opacity(0); opacity: 0;
}
)RCSS";

// Experimental: temporarily change the setting only while drawing the original HUD.
// Config change subscribers may still run; verify on the target build before use.
static void draw_original_hud(ModContext*, void* args, void*, void*) {
    auto& flag = dusk::getSettings().game.enableTouchControls;
    const bool enabled = flag.getValue();
    if (!enabled) {
        HudDraw::g_orig(mods::arg<dMeter2Draw_c*>(args, 0));
        return;
    }
    struct Restore {
        decltype(flag)& value;
        ~Restore() { value.setValue(true); }
    } restore{flag};
    flag.setValue(false);
    HudDraw::g_orig(mods::arg<dMeter2Draw_c*>(args, 0));
}

extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError*) {
    auto result = svc_ui->register_styles(mod_ctx, UI_SCOPE_TOUCH_CONTROLS, css, &style);
    if (result != MOD_OK) return result;
    result = mods::hook::replace<HudDraw>(draw_original_hud);
    if (result != MOD_OK) {
        svc_ui->unregister_styles(mod_ctx, style);
        style = {};
    }
    return result;
}
MOD_EXPORT ModResult mod_update(ModError*) { return MOD_OK; }
MOD_EXPORT ModResult mod_shutdown(ModError*) { return MOD_OK; }
}
