/*
 * SPDX-FileCopyrightText: 2024-2025
 * SPDX-License-Identifier: Apache-2.0
 *
 * LVGL Display Port for ESP-IDF
 * Supports: Default (I2C/SPI/I8080), DSI (MIPI-DSI), RGB interfaces
 */

#include <string.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_idf_version.h"

// ESP32-P4 specific includes (PPA + MIPI-DSI)
#if CONFIG_IDF_TARGET_ESP32P4
#include "esp_cache.h"
#include "esp_lcd_mipi_dsi.h"
#include "driver/ppa.h"
#define lvgl_PORT_PPA_SUPPORTED 1
#define lvgl_PORT_DSI_SUPPORTED 1
#else
#define lvgl_PORT_PPA_SUPPORTED 0
#define lvgl_PORT_DSI_SUPPORTED 0
#endif

// ESP32-S3 RGB support
#if CONFIG_IDF_TARGET_ESP32S3 && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "esp_lcd_panel_rgb.h"
#define lvgl_PORT_RGB_SUPPORTED 1
#else
#define lvgl_PORT_RGB_SUPPORTED 0
#endif

#include "lvgl_port.h"
#include "lvgl_port_disp.h"

static const char *TAG = "lvgl_disp";

/*******************************************************************************
 * Types definitions
 ******************************************************************************/
typedef enum {
    LVGL_DISP_TYPE_OTHER = 0,
    LVGL_DISP_TYPE_DSI,
    LVGL_DISP_TYPE_RGB,
} lvgl_disp_type_t;

typedef struct {
    lvgl_disp_type_t disp_type;
    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_panel_handle_t panel_handle;
    lv_display_t *disp_drv;
    lv_display_rotation_t current_rotation;
    SemaphoreHandle_t trans_sem;

#if lvgl_PORT_PPA_SUPPORTED
    // PPA rotation context (ESP32-P4 only)
    ppa_client_handle_t ppa_client;
    void *ppa_out_buf[2];
    uint8_t ppa_buf_idx;
    size_t ppa_buf_size;
#endif

    // Display info
    uint32_t hres;
    uint32_t vres;
    lv_color_format_t color_format;

    struct {
        unsigned int swap_bytes : 1;
        unsigned int sw_rotate : 1;
        unsigned int full_refresh : 1;
        unsigned int direct_mode : 1;
        unsigned int avoid_tearing : 1;
        unsigned int use_ppa : 1;
    } flags;
} lvgl_disp_ctx_t;

/*******************************************************************************
 * Function declarations
 ******************************************************************************/
static void disp_flush_callback(lv_display_t *drv, const lv_area_t *area, uint8_t *color_map);
static void disp_resolution_changed_cb(lv_event_t *e);

#if lvgl_PORT_PPA_SUPPORTED
/*******************************************************************************
 * PPA Rotation Helper (ESP32-P4 only)
 ******************************************************************************/
static esp_err_t ppa_rotate_init(lvgl_disp_ctx_t *ctx, size_t buffer_size)
{
    ppa_client_config_t ppa_cfg = {};
    ppa_cfg.oper_type           = PPA_OPERATION_SRM;
    ESP_RETURN_ON_ERROR(ppa_register_client(&ppa_cfg, &ctx->ppa_client), TAG, "PPA client register failed");

    // We use DSI frame buffers directly, so no allocation here.
    // Just set the buffer size for reference.
    ctx->ppa_buf_size = buffer_size;
    ctx->ppa_buf_idx  = 0;

    return ESP_OK;
}

static void ppa_rotate_deinit(lvgl_disp_ctx_t *ctx)
{
    if (ctx->ppa_client) {
        ppa_unregister_client(ctx->ppa_client);
        ctx->ppa_client = NULL;
    }
    // We don't own the buffers (managed by DSI driver), so don't free them.
    ctx->ppa_out_buf[0] = NULL;
    ctx->ppa_out_buf[1] = NULL;
}

static esp_err_t ppa_do_rotate(lvgl_disp_ctx_t *ctx, const void *in_buf, lv_area_t *area,
                               lv_display_rotation_t rotation, void **out_buf)
{
    if (!ctx->ppa_client || !ctx->ppa_out_buf[0]) {
        return ESP_ERR_INVALID_STATE;
    }

    int32_t in_w            = lv_area_get_width(area);
    int32_t in_h            = lv_area_get_height(area);
    uint8_t bytes_per_pixel = (ctx->color_format == LV_COLOR_FORMAT_RGB565) ? 2 : 3;

    // Flush Input Buffer Cache to RAM (Important for SPIRAM)
    // PPA Hardware reads from RAM, but CPU might have written to Cache
    size_t in_size = in_w * in_h * bytes_per_pixel;
    // Note: We flush the input buffer range. If in_buf is from heap_caps_aligned_alloc (LVGL buffer), this is correct.
    esp_cache_msync((void *)in_buf, in_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);

    // Toggle output buffer index
    ctx->ppa_buf_idx      = !ctx->ppa_buf_idx;
    void *current_out_buf = ctx->ppa_out_buf[ctx->ppa_buf_idx];

    // Determine output dimensions based on rotation
    int32_t out_w, out_h;
    ppa_srm_rotation_angle_t ppa_rotation;

    switch (rotation) {
        case LV_DISPLAY_ROTATION_90:
            out_w        = in_h;
            out_h        = in_w;
            ppa_rotation = PPA_SRM_ROTATION_ANGLE_90;
            break;
        case LV_DISPLAY_ROTATION_180:
            out_w        = in_w;
            out_h        = in_h;
            ppa_rotation = PPA_SRM_ROTATION_ANGLE_180;
            break;
        case LV_DISPLAY_ROTATION_270:
            out_w        = in_h;
            out_h        = in_w;
            ppa_rotation = PPA_SRM_ROTATION_ANGLE_270;
            break;
        default:
            *out_buf = (void *)in_buf;
            return ESP_OK;
    }

    // Configure PPA SRM operation
    ppa_srm_oper_config_t srm_cfg = {};
    srm_cfg.in.buffer             = in_buf;
    srm_cfg.in.pic_w              = (uint32_t)in_w;
    srm_cfg.in.pic_h              = (uint32_t)in_h;
    srm_cfg.in.block_w            = (uint32_t)in_w;
    srm_cfg.in.block_h            = (uint32_t)in_h;
    srm_cfg.in.block_offset_x     = 0;
    srm_cfg.in.block_offset_y     = 0;
    srm_cfg.in.srm_cm             = (bytes_per_pixel == 2) ? PPA_SRM_COLOR_MODE_RGB565 : PPA_SRM_COLOR_MODE_RGB888;

    srm_cfg.out.buffer         = current_out_buf;
    srm_cfg.out.buffer_size    = ctx->ppa_buf_size;
    srm_cfg.out.pic_w          = (uint32_t)out_w;
    srm_cfg.out.pic_h          = (uint32_t)out_h;
    srm_cfg.out.block_offset_x = 0;
    srm_cfg.out.block_offset_y = 0;
    srm_cfg.out.srm_cm         = (bytes_per_pixel == 2) ? PPA_SRM_COLOR_MODE_RGB565 : PPA_SRM_COLOR_MODE_RGB888;

    srm_cfg.rotation_angle = ppa_rotation;
    srm_cfg.scale_x        = 1.0f;
    srm_cfg.scale_y        = 1.0f;
    srm_cfg.rgb_swap       = 0;
    srm_cfg.byte_swap      = ctx->flags.swap_bytes ? true : false;
    srm_cfg.mode           = PPA_TRANS_MODE_BLOCKING;

    ESP_RETURN_ON_ERROR(ppa_do_scale_rotate_mirror(ctx->ppa_client, &srm_cfg), TAG, "PPA SRM failed");

    // Sync cache
    esp_cache_msync(current_out_buf, ctx->ppa_buf_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);

    // Transform area coordinates for rotated output
    int32_t orig_x1 = area->x1;

    int32_t orig_y1 = area->y1;
    int32_t hres    = ctx->hres;
    int32_t vres    = ctx->vres;

    switch (rotation) {
        case LV_DISPLAY_ROTATION_90:
            area->x1 = vres - orig_y1 - in_h;
            area->y1 = orig_x1;
            area->x2 = area->x1 + out_w - 1;
            area->y2 = area->y1 + out_h - 1;
            break;
        case LV_DISPLAY_ROTATION_180:
            area->x1 = hres - orig_x1 - in_w;
            area->y1 = vres - orig_y1 - in_h;
            area->x2 = area->x1 + out_w - 1;
            area->y2 = area->y1 + out_h - 1;
            break;
        case LV_DISPLAY_ROTATION_270:
            area->x1 = orig_y1;
            area->y1 = hres - orig_x1 - in_w;
            area->x2 = area->x1 + out_w - 1;
            area->y2 = area->y1 + out_h - 1;
            break;
        default:
            break;
    }

    *out_buf = current_out_buf;
    return ESP_OK;
}
#endif  // lvgl_PORT_PPA_SUPPORTED

#if lvgl_PORT_DSI_SUPPORTED
/*******************************************************************************
 * DPI panel callbacks (ESP32-P4 only)
 ******************************************************************************/
static bool dpi_panel_trans_done_cb(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_disp_flush_ready(disp);
    return false;
}

static bool dpi_panel_refresh_done_cb(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t *edata,
                                      void *user_ctx)
{
    BaseType_t need_yield = pdFALSE;
    lv_display_t *disp    = (lv_display_t *)user_ctx;
    lvgl_disp_ctx_t *ctx  = (lvgl_disp_ctx_t *)lv_display_get_driver_data(disp);

    if (ctx && ctx->trans_sem) {
        xSemaphoreGiveFromISR(ctx->trans_sem, &need_yield);
    }

    return (need_yield == pdTRUE);
}
#endif  // lvgl_PORT_DSI_SUPPORTED

/*******************************************************************************
 * Display flush callback
 ******************************************************************************/
static void disp_flush_callback(lv_display_t *drv, const lv_area_t *area, uint8_t *color_map)
{
    lvgl_disp_ctx_t *ctx = (lvgl_disp_ctx_t *)lv_display_get_driver_data(drv);
    assert(ctx != NULL);

    lv_area_t draw_area = *area;
    void *draw_buf      = color_map;

    bool is_last = lv_disp_flush_is_last(drv);
    ESP_LOGD(TAG, "Flush CB: area(%ld,%ld)-(%ld,%ld), rot=%d, ppa=%d, tear=%d, last=%d", area->x1, area->y1, area->x2,
             area->y2, ctx->current_rotation, ctx->flags.use_ppa, ctx->flags.avoid_tearing, is_last);

#if lvgl_PORT_DSI_SUPPORTED
    // For DSI with avoid_tearing (direct mode / full refresh)
    // DSI panels typically don't support partial updates, must refresh entire screen
    if (ctx->disp_type == LVGL_DISP_TYPE_DSI && ctx->flags.avoid_tearing) {
        if (is_last) {
            // Physical screen dimensions (always 720x1280 for this panel)
            int32_t phys_w = ctx->hres;  // 720
            int32_t phys_h = ctx->vres;  // 1280

#if lvgl_PORT_PPA_SUPPORTED
            // For PPA rotation: rotate LVGL's buffer (1280x720) to physical screen (720x1280)
            if (ctx->flags.use_ppa && ctx->flags.sw_rotate && ctx->current_rotation != LV_DISPLAY_ROTATION_0) {
                // LVGL buffer is in rotated resolution (1280x720 for 90/270 rotation)
                int32_t lvgl_w, lvgl_h;
                if (ctx->current_rotation == LV_DISPLAY_ROTATION_90 ||
                    ctx->current_rotation == LV_DISPLAY_ROTATION_270) {
                    lvgl_w = ctx->vres;  // 1280 (LVGL's horizontal)
                    lvgl_h = ctx->hres;  // 720 (LVGL's vertical)
                } else {
                    lvgl_w = ctx->hres;
                    lvgl_h = ctx->vres;
                }

                // Create area for entire LVGL buffer
                lv_area_t full_area = {.x1 = 0, .y1 = 0, .x2 = lvgl_w - 1, .y2 = lvgl_h - 1};

                ESP_LOGD(TAG, "PPA rotate: in=%ldx%ld, out=%ldx%ld, rot=%d", lvgl_w, lvgl_h, phys_w, phys_h,
                         ctx->current_rotation);

                esp_err_t ret = ppa_do_rotate(ctx, color_map, &full_area, ctx->current_rotation, &draw_buf);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "PPA rotation failed: %s", esp_err_to_name(ret));
                    lv_disp_flush_ready(drv);
                    return;
                }
            }
#endif
            ESP_LOGD(TAG, "DSI flush: draw_bitmap(0,0,%ld,%ld), buf=%p", phys_w, phys_h, draw_buf);
            esp_lcd_panel_draw_bitmap(ctx->panel_handle, 0, 0, phys_w, phys_h, draw_buf);

            // Wait for vsync
            if (ctx->trans_sem) {
                xSemaphoreTake(ctx->trans_sem, 0);
                xSemaphoreTake(ctx->trans_sem, portMAX_DELAY);
            }
        }
        lv_disp_flush_ready(drv);
        return;
    }
#endif

#if lvgl_PORT_PPA_SUPPORTED
    // Handle PPA rotation for non-DSI displays or DSI without avoid_tearing
    if (ctx->flags.use_ppa && ctx->flags.sw_rotate && ctx->current_rotation != LV_DISPLAY_ROTATION_0) {
        esp_err_t ret = ppa_do_rotate(ctx, color_map, &draw_area, ctx->current_rotation, &draw_buf);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "PPA rotation failed: %s", esp_err_to_name(ret));
            lv_disp_flush_ready(drv);
            return;
        }
    } else
#endif
        if (ctx->flags.swap_bytes) {
        // Swap bytes for non-PPA path
        size_t len = lv_area_get_size(area);
        lv_draw_sw_rgb565_swap(color_map, len);
    }

    // Partial update mode (default path) or DSI without avoid_tearing
    esp_lcd_panel_draw_bitmap(ctx->panel_handle, draw_area.x1, draw_area.y1, draw_area.x2 + 1, draw_area.y2 + 1,
                              draw_buf);

    // For non-DSI displays or DSI without avoid_tearing,
    // flush_ready is called by panel IO callback or here for DSI
#if lvgl_PORT_DSI_SUPPORTED
    if (ctx->disp_type == LVGL_DISP_TYPE_DSI) {
        // DSI without avoid_tearing: wait for trans_done callback
        // The callback will call lv_disp_flush_ready
    } else
#endif
    {
        // For displays without IO callback registered, call flush_ready here
        if (ctx->io_handle == NULL) {
            lv_disp_flush_ready(drv);
        }
        // Otherwise, the IO callback will call lv_disp_flush_ready
    }
}

/*******************************************************************************
 * Resolution changed callback
 ******************************************************************************/
static void disp_resolution_changed_cb(lv_event_t *e)
{
    lv_display_t *disp   = (lv_display_t *)lv_event_get_target(e);
    lvgl_disp_ctx_t *ctx = (lvgl_disp_ctx_t *)lv_display_get_driver_data(disp);
    if (ctx) {
        ctx->current_rotation = lv_display_get_rotation(disp);
    }
}

/*******************************************************************************
 * Private: Common display initialization
 ******************************************************************************/
static lv_display_t *add_disp_common(const lvgl_disp_cfg_t *cfg, lvgl_disp_ctx_t *ctx)
{
    lv_color_t *buf1 = NULL;
    lv_color_t *buf2 = NULL;

    assert(cfg != NULL);
    assert(cfg->panel_handle != NULL);
    assert(cfg->buffer_size > 0);
    assert(cfg->hres > 0 && cfg->vres > 0);

    // Determine color format
    lv_color_format_t color_format = (cfg->color_format != 0) ? cfg->color_format : LV_COLOR_FORMAT_RGB565;
    uint8_t color_bytes            = lv_color_format_get_size(color_format);

    // Store display info
    ctx->hres               = cfg->hres;
    ctx->vres               = cfg->vres;
    ctx->color_format       = color_format;
    ctx->io_handle          = cfg->io_handle;
    ctx->panel_handle       = cfg->panel_handle;
    ctx->flags.swap_bytes   = cfg->flags.swap_bytes;
    ctx->flags.sw_rotate    = cfg->flags.sw_rotate;
    ctx->flags.full_refresh = cfg->flags.full_refresh;
    ctx->flags.direct_mode  = cfg->flags.direct_mode;
    // Note: ctx->current_rotation should be set by caller before calling this function
    // For DSI displays, it's set in lvgl_port_add_disp_dsi()
    // For other displays, it defaults to 0 (from calloc)

    // Allocate buffers
    uint32_t buff_caps = MALLOC_CAP_DEFAULT;
    if (cfg->flags.buff_dma) {
        buff_caps = MALLOC_CAP_DMA;
    }
    if (cfg->flags.buff_spiram) {
        buff_caps = MALLOC_CAP_SPIRAM;
    }

    uint32_t buffer_size = cfg->buffer_size;

#if lvgl_PORT_DSI_SUPPORTED
    // For avoid_tearing mode with rotation, we need separate draw buffers
    // because DPI frame buffer layout (720x1280) doesn't match LVGL's rotated view (1280x720)
    if (ctx->flags.avoid_tearing && ctx->disp_type == LVGL_DISP_TYPE_DSI) {
        // If using PPA rotation, allocate separate buffers for LVGL to draw into
        // The rotated content will be copied to DPI frame buffer in flush callback
        if (ctx->flags.sw_rotate && ctx->flags.use_ppa) {
            // For rotated display, LVGL needs buffers matching its logical resolution
            // After 90/270 rotation: LVGL sees vres x hres (1280x720)
            uint32_t lvgl_hres = cfg->vres;  // 1280 (LVGL's horizontal after rotation)
            uint32_t lvgl_vres = cfg->hres;  // 720 (LVGL's vertical after rotation)
            buffer_size        = lvgl_hres * lvgl_vres;

            buf1 = (lv_color_t *)heap_caps_aligned_alloc(64, buffer_size * color_bytes, MALLOC_CAP_SPIRAM);
            if (!buf1) {
                ESP_LOGE(TAG, "Alloc rotated buf1 failed");
                return NULL;
            }

            // For direct_mode (partial rendering), use single LVGL buffer:
            // buf1 always holds the complete frame; LVGL updates only dirty areas in-place.
            // PPA output still alternates between two DPI frame buffers for tearing prevention.
            // For full_refresh mode, two LVGL buffers are needed to pipeline rendering.
            if (cfg->flags.full_refresh) {
                buf2 = (lv_color_t *)heap_caps_aligned_alloc(64, buffer_size * color_bytes, MALLOC_CAP_SPIRAM);
                if (!buf2) {
                    ESP_LOGE(TAG, "Alloc rotated buf2 failed");
                    free(buf1);
                    return NULL;
                }
                ESP_LOGI(TAG, "DSI+PPA rotation (full_refresh): allocated 2x %lux%lu buffers (%u bytes each)",
                         (unsigned long)lvgl_hres, (unsigned long)lvgl_vres, (unsigned)(buffer_size * color_bytes));
            } else {
                // direct_mode: single buffer, no double-buffer sync issue
                buf2 = NULL;
                ESP_LOGI(TAG, "DSI+PPA rotation (direct_mode): single buffer %lux%lu (%u bytes)",
                         (unsigned long)lvgl_hres, (unsigned long)lvgl_vres, (unsigned)(buffer_size * color_bytes));
            }

            // Get DSI frame buffers to use as PPA output
            void *dsi_buf1 = NULL;
            void *dsi_buf2 = NULL;
            esp_err_t ret  = esp_lcd_dpi_panel_get_frame_buffer(cfg->panel_handle, 2, &dsi_buf1, &dsi_buf2);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Get DPI frame buffers for PPA output failed");
                free(buf1);
                free(buf2);
                return NULL;
            }
            if (!dsi_buf1 || !dsi_buf2) {
                ESP_LOGE(TAG, "DPI frame buffers are NULL");
                free(buf1);
                free(buf2);
                return NULL;
            }
            ctx->ppa_out_buf[0] = dsi_buf1;
            ctx->ppa_out_buf[1] = dsi_buf2;
            ESP_LOGI(TAG, "PPA output assigned to DSI frame buffers");

        } else {
            // No rotation or no PPA: use DPI frame buffers directly
            buffer_size   = cfg->hres * cfg->vres;
            esp_err_t ret = esp_lcd_dpi_panel_get_frame_buffer(cfg->panel_handle, 2, (void **)&buf1, (void **)&buf2);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Get DPI frame buffers failed");
                return NULL;
            }
            ESP_LOGI(TAG, "DSI: using DPI frame buffers directly");
        }

        ctx->trans_sem = xSemaphoreCreateCounting(1, 0);
        if (!ctx->trans_sem) {
            ESP_LOGE(TAG, "Create semaphore failed");
            return NULL;
        }
    } else
#endif
    {
        // Allocate draw buffers
        buf1 = (lv_color_t *)heap_caps_aligned_alloc(64, buffer_size * color_bytes, buff_caps);
        if (!buf1) {
            ESP_LOGE(TAG, "Alloc buf1 failed");
            return NULL;
        }

        if (cfg->double_buffer) {
            buf2 = (lv_color_t *)heap_caps_aligned_alloc(64, buffer_size * color_bytes, buff_caps);
            if (!buf2) {
                ESP_LOGE(TAG, "Alloc buf2 failed");
                free(buf1);
                return NULL;
            }
        }
    }

    // Set render mode
    lv_display_render_mode_t render_mode = LV_DISPLAY_RENDER_MODE_PARTIAL;

    // User-specified full_refresh takes priority
    if (cfg->flags.full_refresh) {
        render_mode = LV_DISPLAY_RENDER_MODE_FULL;
        ESP_LOGI(TAG, "Using FULL render mode (full_refresh enabled)");
    } else if (cfg->flags.direct_mode) {
        render_mode = LV_DISPLAY_RENDER_MODE_DIRECT;
        ESP_LOGI(TAG, "Using DIRECT render mode (direct_mode enabled)");
    }
#if lvgl_PORT_DSI_SUPPORTED
    // For DSI with avoid_tearing, default to DIRECT mode if no explicit mode set
    else if (ctx->disp_type == LVGL_DISP_TYPE_DSI && ctx->flags.avoid_tearing) {
        render_mode = LV_DISPLAY_RENDER_MODE_DIRECT;
        ESP_LOGI(TAG, "DSI avoid_tearing: using DIRECT render mode with double buffer");
    }
#endif

    // Determine display resolution for LVGL
    // For DSI with PPA rotation, LVGL sees the rotated resolution
    uint32_t disp_hres = cfg->hres;
    uint32_t disp_vres = cfg->vres;

#if lvgl_PORT_DSI_SUPPORTED && lvgl_PORT_PPA_SUPPORTED
    if (ctx->disp_type == LVGL_DISP_TYPE_DSI && ctx->flags.avoid_tearing && ctx->flags.sw_rotate &&
        ctx->flags.use_ppa &&
        (ctx->current_rotation == LV_DISPLAY_ROTATION_90 || ctx->current_rotation == LV_DISPLAY_ROTATION_270)) {
        // Swap resolution for 90/270 rotation
        disp_hres = cfg->vres;  // 1280
        disp_vres = cfg->hres;  // 720
        ESP_LOGI(TAG, "DSI+PPA: LVGL display resolution %lux%lu (rotated)", (unsigned long)disp_hres,
                 (unsigned long)disp_vres);
    }
#endif

    // Create LVGL display with appropriate resolution
    lv_display_t *disp = lv_display_create(disp_hres, disp_vres);
    if (!disp) {
        ESP_LOGE(TAG, "Create display failed");
        goto err;
    }

    lv_display_set_color_format(disp, color_format);

    lv_display_set_buffers(disp, buf1, buf2, buffer_size * color_bytes, render_mode);
    lv_display_set_flush_cb(disp, disp_flush_callback);
    lv_display_set_driver_data(disp, ctx);
    lv_display_add_event_cb(disp, disp_resolution_changed_cb, LV_EVENT_RESOLUTION_CHANGED, ctx);

    ctx->disp_drv = disp;

    return disp;

err:
    if (buf1 && !ctx->flags.avoid_tearing) free(buf1);
    if (buf2 && !ctx->flags.avoid_tearing) free(buf2);
    if (ctx->trans_sem) vSemaphoreDelete(ctx->trans_sem);
    return NULL;
}

/*******************************************************************************
 * Public API: Default display (I2C/SPI/I8080)
 ******************************************************************************/
lv_display_t *lvgl_port_add_disp(const lvgl_disp_cfg_t *disp_cfg)
{
    lvgl_port_lock(0);

    lvgl_disp_ctx_t *ctx = (lvgl_disp_ctx_t *)calloc(1, sizeof(lvgl_disp_ctx_t));
    if (!ctx) {
        ESP_LOGE(TAG, "Alloc display context failed");
        lvgl_port_unlock();
        return NULL;
    }

    ctx->disp_type = LVGL_DISP_TYPE_OTHER;

    lv_display_t *disp = add_disp_common(disp_cfg, ctx);
    if (!disp) {
        free(ctx);
        lvgl_port_unlock();
        return NULL;
    }

    // Register IO callback for flush ready (if io_handle provided)
    if (disp_cfg->io_handle) {
        const esp_lcd_panel_io_callbacks_t cbs = {
            .on_color_trans_done = [](esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *edata,
                                      void *user_ctx) -> bool {
                lv_display_t *d = (lv_display_t *)user_ctx;
                lv_disp_flush_ready(d);
                return false;
            },
        };
        esp_lcd_panel_io_register_event_callbacks(disp_cfg->io_handle, &cbs, disp);
    }

    lvgl_port_unlock();
    return disp;
}

/*******************************************************************************
 * Public API: DSI display (MIPI-DSI) - ESP32-P4 only
 ******************************************************************************/
lv_display_t *lvgl_port_add_disp_dsi(const lvgl_disp_cfg_t *disp_cfg, const lvgl_disp_dsi_cfg_t *dsi_cfg)
{
#if lvgl_PORT_DSI_SUPPORTED
    assert(dsi_cfg != NULL);

    lvgl_port_lock(0);

    lvgl_disp_ctx_t *ctx = (lvgl_disp_ctx_t *)calloc(1, sizeof(lvgl_disp_ctx_t));
    if (!ctx) {
        ESP_LOGE(TAG, "Alloc display context failed");
        lvgl_port_unlock();
        return NULL;
    }

    ctx->disp_type           = LVGL_DISP_TYPE_DSI;
    ctx->flags.avoid_tearing = dsi_cfg->flags.avoid_tearing;
    ctx->flags.use_ppa       = dsi_cfg->flags.use_ppa;
    ctx->flags.sw_rotate     = disp_cfg->flags.sw_rotate;  // Set before add_disp_common
    ctx->current_rotation    = dsi_cfg->sw_rotation;

    lv_display_t *disp = add_disp_common(disp_cfg, ctx);
    if (!disp) {
        free(ctx);
        lvgl_port_unlock();
        return NULL;
    }

#if lvgl_PORT_PPA_SUPPORTED
    // Initialize PPA for rotation if needed
    if (dsi_cfg->flags.use_ppa && disp_cfg->flags.sw_rotate) {
        uint8_t color_bytes = lv_color_format_get_size(ctx->color_format);
        // For full screen rotation, PPA buffer must hold entire screen
        size_t ppa_buf_size = disp_cfg->hres * disp_cfg->vres * color_bytes;
        ESP_LOGI(TAG, "PPA buffer size: %u bytes (%lux%lu, %u bpp)", (unsigned)ppa_buf_size,
                 (unsigned long)disp_cfg->hres, (unsigned long)disp_cfg->vres, color_bytes);
        if (ppa_rotate_init(ctx, ppa_buf_size) != ESP_OK) {
            ESP_LOGW(TAG, "PPA init failed, rotation disabled");
            ctx->flags.use_ppa = 0;
        }
    }
#endif

    // Register DPI panel callbacks
    esp_lcd_dpi_panel_event_callbacks_t cbs = {};
    if (dsi_cfg->flags.avoid_tearing) {
        cbs.on_refresh_done = dpi_panel_refresh_done_cb;
    } else {
        cbs.on_color_trans_done = dpi_panel_trans_done_cb;
    }
    esp_lcd_dpi_panel_register_event_callbacks(ctx->panel_handle, &cbs, disp);

    // For DSI + PPA rotation: don't use LVGL's software rotation
    // We already created display with rotated resolution, PPA handles the actual rotation
    // Only set rotation for non-PPA case (LVGL software rotation)
    if (dsi_cfg->sw_rotation != LV_DISPLAY_ROTATION_0) {
        if (!(ctx->flags.avoid_tearing && ctx->flags.use_ppa && ctx->flags.sw_rotate)) {
            // Use LVGL's software rotation only if not using PPA
            lv_display_set_rotation(disp, dsi_cfg->sw_rotation);
        }
    }

    lvgl_port_unlock();
    return disp;
#else
    ESP_LOGE(TAG, "MIPI-DSI is only supported on ESP32-P4!");
    return NULL;
#endif
}

/*******************************************************************************
 * Public API: RGB display (Parallel RGB) - ESP32-S3 only
 ******************************************************************************/
lv_display_t *lvgl_port_add_disp_rgb(const lvgl_disp_cfg_t *disp_cfg, const lvgl_disp_rgb_cfg_t *rgb_cfg)
{
#if lvgl_PORT_RGB_SUPPORTED
    assert(rgb_cfg != NULL);

    lvgl_port_lock(0);

    lvgl_disp_ctx_t *ctx = (lvgl_disp_ctx_t *)calloc(1, sizeof(lvgl_disp_ctx_t));
    if (!ctx) {
        ESP_LOGE(TAG, "Alloc display context failed");
        lvgl_port_unlock();
        return NULL;
    }

    ctx->disp_type           = LVGL_DISP_TYPE_RGB;
    ctx->flags.avoid_tearing = rgb_cfg->flags.avoid_tearing;

    lv_display_t *disp = add_disp_common(disp_cfg, ctx);
    if (!disp) {
        free(ctx);
        lvgl_port_unlock();
        return NULL;
    }

    // TODO: Register RGB panel callbacks for ESP32-S3

    lvgl_port_unlock();
    return disp;
#else
    ESP_LOGE(TAG, "RGB parallel display is only supported on ESP32-S3!");
    return NULL;
#endif
}

/*******************************************************************************
 * Public API: Remove display
 ******************************************************************************/
esp_err_t lvgl_port_remove_disp(lv_display_t *disp)
{
    if (!disp) {
        return ESP_ERR_INVALID_ARG;
    }

    lvgl_disp_ctx_t *ctx = (lvgl_disp_ctx_t *)lv_display_get_driver_data(disp);

    lvgl_port_lock(0);
    lv_disp_remove(disp);
    lvgl_port_unlock();

    if (ctx) {
#if lvgl_PORT_PPA_SUPPORTED
        ppa_rotate_deinit(ctx);
#endif

        if (ctx->trans_sem) {
            vSemaphoreDelete(ctx->trans_sem);
        }

        free(ctx);
    }

    return ESP_OK;
}
