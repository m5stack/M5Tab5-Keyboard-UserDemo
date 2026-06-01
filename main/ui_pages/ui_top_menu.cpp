/*
 * SPDX-FileCopyrightText: 2025-2026 M5Stack
 *
 * SPDX-License-Identifier: MIT
 */

#include "ui_top_menu.h"
#include <string.h>

LV_FONT_DECLARE(lv_font_montserrat_14);

// --- Menu Creation ---

struct MenuContext;

typedef struct {
    struct MenuContext* ctx;
    int index;
} menu_btn_data_t;

#define MENU_BTN_MAX 5
#define MENU_BTN_W   100

struct MenuContext {
    lv_obj_t* menu_cont;
    lv_obj_t* indicator;
    lv_obj_t* buttons[MENU_BTN_MAX];  // Stores button objects 0-5
    void (*switch_callback)(int);
    int current_scope;
    int click_counter;
    int last_clicked_index;
    menu_btn_data_t btn_data[MENU_BTN_MAX];
};

static void layoutM5Tab5Menu(MenuContext* ctx)
{
    if (!ctx || !ctx->menu_cont) return;

    int btn_w                      = MENU_BTN_W;
    int visible_count              = 0;
    int x_pos                      = 0;
    int first_visible_index        = -1;
    bool current_selection_visible = false;

    // Define Render Order: Normal, String, HID, REGISTRY, Factory
    // This ensures that when Factory (0) is revealed, it appears at the end
    // preventing the other items from shifting their relative order.
    const int render_order[] = {1, 2, 3, 4, 0};

    // Iterate through all potential buttons in custom order
    for (int k = 0; k < MENU_BTN_MAX; k++) {
        int i         = render_order[k];
        lv_obj_t* btn = ctx->buttons[i];
        if (!btn) continue;

        // Check if this button is enabled in the current scope
        // Scope bit 0 = Index 0 (Factory), bit 1 = Index 1 (Normal), etc.
        bool enabled = (ctx->current_scope & (1 << i));

        if (enabled) {
            lv_obj_clear_flag(btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_x(btn, x_pos);
            lv_obj_set_width(btn, btn_w);  // Ensure width is set
            x_pos += btn_w;
            visible_count++;

            if (first_visible_index == -1) first_visible_index = i;

            // Check if indicator is currently here
            if (ctx->last_clicked_index == i) {
                current_selection_visible = true;
                // Update indicator position target
                lv_obj_set_x(ctx->indicator, lv_obj_get_x(btn));
            }
        } else {
            lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Resize container and re-align to maintain right alignment
    lv_obj_set_width(ctx->menu_cont, visible_count * btn_w);
    lv_obj_align(ctx->menu_cont, LV_ALIGN_TOP_RIGHT, -30, 15);

    // 同步 indicator 宽度，确保与按钮宽度一致（避免动画偏移）
    if (ctx->indicator) {
        lv_obj_set_width(ctx->indicator, btn_w);
    }

    // If current selection is hidden, select the first visible one
    if (!current_selection_visible && first_visible_index != -1) {
        ctx->last_clicked_index = first_visible_index;
        lv_obj_t* btn           = ctx->buttons[first_visible_index];
        if (btn) {
            lv_obj_set_x(ctx->indicator, lv_obj_get_x(btn));
            // Trigger callback to default page
            if (ctx->switch_callback) {
                ctx->switch_callback(first_visible_index);
            }
        }
    }

    // Update indicator color based on initial selection (or updated selection)
    lv_color_t color = lv_color_hex(0x007AFF);  // Default Blue
    switch (ctx->last_clicked_index) {
        case 0:
            color = lv_color_hex(0x808080);
            break;  // Factory: Gray
        case 1:
            color = lv_color_hex(0x007AFF);
            break;  // Normal: Blue
        case 2:
            color = lv_color_hex(0xFF69B4);
            break;  // String: Pink (HotPink)
        case 3:
            color = lv_color_hex(0x00C000);
            break;  // HID: Green (Dimmed)
        case 4:
            color = lv_color_hex(0x9B59B6);
            break;  // REGISTRY: Purple
    }
    lv_obj_set_style_bg_color(ctx->indicator, color, 0);
}

static void menu_btn_event_cb(lv_event_t* e)
{
    lv_obj_t* btn         = (lv_obj_t*)lv_event_get_target(e);
    menu_btn_data_t* data = (menu_btn_data_t*)lv_event_get_user_data(e);

    if (data && data->ctx && btn) {
        MenuContext* ctx = data->ctx;

        // 1. Check for Unlock (God Mode)
        // If clicking the same button repeatedly
        if (ctx->last_clicked_index == data->index) {
            ctx->click_counter++;
            // ESP_LOGD("Menu", "Click count: %d", ctx->click_counter);
            if (ctx->click_counter >= 10 && ctx->current_scope != M5TAB5_MENU_SCOPE_ALL) {
                // ESP_LOGI("Menu", "Unlocking ALL menus!");
                ctx->current_scope = M5TAB5_MENU_SCOPE_ALL;
                layoutM5Tab5Menu(ctx);
                ctx->click_counter = 0;
            }
        } else {
            ctx->click_counter = 1;
        }

        ctx->last_clicked_index = data->index;

        // 2. Move Indicator & Change Color
        // We need to move it to the button's current position (which might have changed in layout)
        if (ctx->indicator) {
            lv_obj_set_x(ctx->indicator, lv_obj_get_x(btn));

            // Color Logic
            lv_color_t color = lv_color_hex(0x007AFF);  // Default Blue
            switch (data->index) {
                case 0:
                    color = lv_color_hex(0x808080);
                    break;  // Factory: Gray
                case 1:
                    color = lv_color_hex(0x007AFF);
                    break;  // Normal: Blue
                case 2:
                    color = lv_color_hex(0xFF69B4);
                    break;  // String: Pink
                case 3:
                    color = lv_color_hex(0x00C000);
                    break;  // HID: Green (Dimmed)
                case 4:
                    color = lv_color_hex(0x9B59B6);
                    break;  // REGISTRY: Purple
            }
            lv_obj_set_style_bg_color(ctx->indicator, color, 0);
        }

        // 3. Call Callback
        if (ctx->switch_callback) {
            ctx->switch_callback(data->index);
        }
    }
}

lv_obj_t* createM5Tab5Menu(lv_obj_t* parent, void (*switch_callback)(int), int scope_mask)
{
    // 0. Create Context
    MenuContext* ctx = new MenuContext();
    memset(ctx, 0, sizeof(MenuContext));
    ctx->switch_callback    = switch_callback;
    ctx->current_scope      = scope_mask;
    ctx->last_clicked_index = -1;  // Will be resolved to first visible item by layout

    // 1. Create Container (The Track)
    lv_obj_t* menu_cont = lv_obj_create(parent);
    ctx->menu_cont      = menu_cont;

    int btn_w = MENU_BTN_W;  // Width per button
    int btn_h = 50;

    // Initial size will be set by layout, but set height here
    lv_obj_set_height(menu_cont, btn_h);
    lv_obj_align(menu_cont, LV_ALIGN_TOP_RIGHT, -30, 15);

    // Style: Pill shape, Dark background
    lv_obj_set_style_radius(menu_cont, btn_h / 2, 0);  // Full pill radius
    lv_obj_set_style_bg_color(menu_cont, lv_color_hex(0x303030), 0);
    lv_obj_set_style_bg_opa(menu_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(menu_cont, 0, 0);
    lv_obj_set_style_pad_all(menu_cont, 0, 0);
    lv_obj_clear_flag(menu_cont, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Create Indicator (The Slider/Pill)
    lv_obj_t* indicator = lv_obj_create(menu_cont);
    ctx->indicator      = indicator;
    lv_obj_set_size(indicator, btn_w, btn_h);
    lv_obj_set_style_radius(indicator, btn_h / 2, 0);
    lv_obj_set_style_bg_color(indicator, lv_color_hex(0x007AFF), 0);  // Blue active color
    lv_obj_set_style_bg_opa(indicator, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(indicator, 0, 0);

    lv_obj_add_flag(indicator, LV_OBJ_FLAG_FLOATING);
    lv_obj_add_flag(indicator, LV_OBJ_FLAG_IGNORE_LAYOUT);

    // Check LVGL version for Animation Transition
#if LVGL_VERSION_MAJOR >= 8
    static const lv_style_prop_t props[] = {LV_STYLE_X, (lv_style_prop_t)0};
    static lv_style_transition_dsc_t trans;
    // Initialize once
    if (trans.props == NULL) {
        lv_style_transition_dsc_init(&trans, props, lv_anim_path_ease_out, 250, 0, NULL);
    }
    lv_obj_set_style_transition(indicator, &trans, 0);
#endif

    const char* menu_titles[] = {"FACTORY", "Normal", "String", "HID", "REGISTRY"};
    int btn_count             = MENU_BTN_MAX;

    for (int i = 0; i < btn_count; i++) {
        // Transparent Button Area
#if LVGL_VERSION_MAJOR >= 9
        lv_obj_t* btn = lv_button_create(menu_cont);
#else
        lv_obj_t* btn = lv_btn_create(menu_cont);
#endif
        ctx->buttons[i] = btn;
        lv_obj_set_size(btn, btn_w, btn_h);
        // Position will be set by layoutM5Tab5Menu

        // Transparent style
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_radius(btn, btn_h / 2, 0);

        // Label
        lv_obj_t* l = lv_label_create(btn);
        lv_label_set_text(l, menu_titles[i]);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(l);

        // Setup button data
        ctx->btn_data[i].ctx   = ctx;
        ctx->btn_data[i].index = i;

        // Event
        lv_obj_add_event_cb(btn, menu_btn_event_cb, LV_EVENT_CLICKED, &ctx->btn_data[i]);
    }

    layoutM5Tab5Menu(ctx);

    return menu_cont;
}
