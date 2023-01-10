#include <furi.h>
#include <furi_hal.h>

#include <gui/gui.h>
#include <input/input.h>

#include <stdbool.h>

#define TAG "iambic_morse_paddle"
#define DIT_LENGTH 200

typedef enum {
    Nothing,
    Dit,
    Dah,
    DitDah,
    DahDit,
} Sequence;

typedef struct {
    Sequence sequence;
} IambicState;

typedef struct {
    IambicState *state;
    FuriMutex **state_mutex;

    FuriThread *worker_thread;
    bool should_work;
} IambicPaddle;

static void draw_callback(Canvas *canvas, void *ctx) {
    furi_assert(ctx);
    IambicPaddle *paddle = ctx;
    furi_check(furi_mutex_acquire(paddle->state_mutex, FuriWaitForever) == FuriStatusOk);

    IambicState *state = paddle->state;
    char *seq = malloc(sizeof(char) * 2);
    if (state->sequence == Nothing)     seq = "  ";
    else if (state->sequence == Dit)    seq = ". ";
    else if (state->sequence == Dah)    seq = "_ ";
    else if (state->sequence == DitDah) seq = "._";
    else if (state->sequence == DahDit) seq = "_.";

    canvas_draw_str(canvas, 12, 16, "Iambic Paddle");
    canvas_draw_str(canvas, 12, 32, "Ready!");
    canvas_draw_str(canvas, 48, 29, seq);

    furi_mutex_release(paddle->state_mutex);
    free(seq);
}

static void input_callback(InputEvent *input_event, void *ctx) {
    furi_assert(ctx);
    FuriMessageQueue *event_queue = ctx;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

static int32_t worker_thread_callback(void *ctx) {
    furi_assert(ctx);
    IambicPaddle *paddle = ctx;
    FURI_LOG_I(TAG, "Worker thread starting...");

    // count defaults to 1 as this skips the first signal, which has already played as a single dit or dah
    uint8_t count = 1;
    if (furi_hal_speaker_acquire(1000)) {
        while (paddle->should_work) {
            furi_mutex_acquire(paddle->state_mutex, FuriWaitForever);
            Sequence seq = paddle->state->sequence;
            furi_mutex_release(paddle->state_mutex);
            FURI_LOG_D(TAG, "seq: %d", seq);

            bool sound = true;
            uint32_t delay;
            switch (seq) {
                case Nothing:
                    sound = false;
                    count = 1;
                    break;
                case Dit:
                    delay = DIT_LENGTH;
                    count = 1;
                    break;
                case Dah:
                    delay = 3 * DIT_LENGTH;
                    count = 1;
                    break;
                case DitDah:
                    if (count == 0) {
                        // Dit
                        count = 1;
                        delay = DIT_LENGTH;
                    } else {
                        // Dah
                        count = 0;
                        delay = 3 * DIT_LENGTH;
                    }
                    break;
                case DahDit:
                    if (count == 0) {
                        // Dah
                        count = 1;
                        delay = 3 * DIT_LENGTH;
                    } else {
                        // Dit       
                        count = 0;
                        delay = DIT_LENGTH;
                    }
                    break;
                default:
                    furi_crash("unreachable code");
                    break;
            }

            if (sound) {
                furi_hal_speaker_start(440, 1);
                furi_delay_ms(delay);
            }
            furi_hal_speaker_stop();
            furi_delay_ms(DIT_LENGTH);
        }

        furi_hal_speaker_stop();
        furi_hal_speaker_release();
    } else {
        FURI_LOG_E(TAG, "Unable to acquire speaker.");
    }

    return 0;
}

int32_t iambic_app(void* p) {
    UNUSED(p);
    IambicPaddle *paddle = malloc(sizeof(IambicPaddle));
    paddle->state = malloc(sizeof(IambicState));
    paddle->state_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    paddle->worker_thread = furi_thread_alloc_ex("IambicWorker", 1024, worker_thread_callback, paddle);
    paddle->should_work = true;
    furi_thread_start(paddle->worker_thread);

    FuriMessageQueue *event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    
    // Set ViewPort callbacks
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, draw_callback, paddle);
    view_port_input_callback_set(view_port, input_callback, event_queue);

    // Open GUI and register view_port
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    InputEvent event;
    bool running = true;
    while (running) {
        if (furi_message_queue_get(event_queue, &event, 100) == FuriStatusOk) {
            switch (event.type) {
                case InputTypeLong:
                    if (event.key == InputKeyBack) {
                        running = false;
                    }
                    break;
                case InputTypePress:
                    if (event.key == InputKeyLeft) {
                        if (paddle->state->sequence == Nothing) paddle->state->sequence = Dit;
                        else if (paddle->state->sequence == Dah) paddle->state->sequence = DahDit;
                    } else if (event.key == InputKeyRight) {
                        if (paddle->state->sequence == Nothing) paddle->state->sequence = Dah;
                        else if (paddle->state->sequence == Dit) paddle->state->sequence = DitDah;
                    }
                    break;
                case InputTypeRelease:
                    if (event.key == InputKeyLeft) {
                        if (paddle->state->sequence == DahDit) paddle->state->sequence = Dah;
                        if (paddle->state->sequence == DitDah) paddle->state->sequence = Dah;
                        else if (paddle->state->sequence == Dit) paddle->state->sequence = Nothing;
                    } else if (event.key == InputKeyRight) {
                        if (paddle->state->sequence == DitDah) paddle->state->sequence = Dit;
                        else if (paddle->state->sequence == DahDit) paddle->state->sequence = Dit;
                        else if (paddle->state->sequence == Dah) paddle->state->sequence = Nothing;
                    }
                    break;
                default:
                    break;
            }
        }
        view_port_update(view_port);
    }

    // Stop noise thread
    paddle->should_work = false;
    furi_thread_join(paddle->worker_thread);
    
    // Free state object
    furi_thread_free(paddle->worker_thread);
    furi_mutex_free(paddle->state_mutex);
    free(paddle->state);
    free(paddle);

    // Close GUI
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);
    furi_message_queue_free(event_queue);

    return 0;
}

