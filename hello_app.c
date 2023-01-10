#include <furi.h>
#include <furi_hal.h>

#include <gui/gui.h>
#include <input/input.h>

static void draw_callback(Canvas *canvas, void *ctx) {
    UNUSED(ctx);
    canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "Hello, world!");
}

static void input_callback(InputEvent *input_event, void *ctx) {
    furi_assert(ctx);
    FuriMessageQueue *event_queue = ctx;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

int32_t hello_app(void* p) {
    UNUSED(p);
    FuriMessageQueue *event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    
    // Set ViewPort callbacks
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, draw_callback, NULL);
    view_port_input_callback_set(view_port, input_callback, event_queue);

    // Open GUI and register view_port
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    InputEvent event;
    bool running = true;
    while (running) {
        if (furi_message_queue_get(event_queue, &event, 100) == FuriStatusOk) {
            if (event.type == InputTypePress && event.key == InputKeyBack) {
		running = false;
	    }
	}
	view_port_update(view_port);
    }

    // Close GUI
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);
    furi_message_queue_free(event_queue);

    return 0;
}

