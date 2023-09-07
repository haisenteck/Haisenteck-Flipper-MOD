#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <locale/locale.h>
#include <input/input.h>
#include <storage/filesystem_api_defines.h>
#include <storage/storage.h>
#include <furi_hal_bt.h>
#define TAG "fruttivendolo_app"

char* line_;
char* par1;
int BUFFER = 30;

typedef enum {
    fruttivendoloEventTypeKey,
    // You can add additional events here.
} fruttivendoloEventType;

typedef struct {
    fruttivendoloEventType type; // The reason for this event.
    InputEvent input;   // This data is specific to keypress data.
    // You can add additional data that is helpful for your events.
} fruttivendoloEvent;

typedef struct {
    FuriString* buffer;
    // You can add additional state here.
} fruttivendoloData;

typedef struct {
    FuriMessageQueue* queue; // Message queue (fruttivendoloEvent items to process).
    FuriMutex* mutex; // Used to provide thread safe access to data.
    fruttivendoloData* data; // Data accessed by multiple threads (acquire the mutex before accessing!)
} fruttivendoloContext;

// Invoked when input (button press) is detected.  We queue a message and then return to the caller.
static void fruttivendolo_input_callback(InputEvent* input_event, FuriMessageQueue* queue) {
    furi_assert(queue);
    fruttivendoloEvent event = {.type = fruttivendoloEventTypeKey, .input = *input_event};
    furi_message_queue_put(queue, &event, FuriWaitForever);
}

// Invoked by the draw callback to render the screen. We render our UI on the callback thread.
static void fruttivendolo_render_callback(Canvas* canvas, void* ctx) {
    // Attempt to aquire context, so we can read the data.
    fruttivendoloContext* fruttivendolo_context = ctx;
    if(furi_mutex_acquire(fruttivendolo_context->mutex, 200) != FuriStatusOk) {
        return;
    }

    fruttivendoloData* data = fruttivendolo_context->data;
    
	furi_string_printf(data->buffer, "Basic");
    canvas_set_bitmap_mode(canvas, 1);
	canvas_set_font(canvas, FontPrimary);
	canvas_draw_str(canvas, 10, 9, "FRUTTIVENDOLO");
	canvas_set_font(canvas, FontSecondary);
	canvas_draw_str(canvas, 3, 18, "1 normale");
	canvas_draw_str(canvas, 3, 27, "2 airtag");
	canvas_draw_str(canvas, 3, 36, "3 keyboard");
	canvas_draw_str(canvas, 3, 45, "4 tv notification");
	canvas_draw_str(canvas, 3, 54, "5 iphone notification");
	canvas_draw_str(canvas, 112, 48, "6");
	canvas_draw_str(canvas, 3, 64, "6 trasferisci numero   Sel: ");
	canvas_draw_str(canvas, 108, 64, line_);
	canvas_draw_circle(canvas, 96, 26, 16);
	canvas_set_font(canvas, FontSecondary);
	canvas_draw_str(canvas, 94, 30, "5");
	canvas_draw_str(canvas, 105, 30, "3");
	canvas_draw_str(canvas, 94, 19, "4");
	canvas_draw_str(canvas, 95, 41, "1");
	canvas_draw_str(canvas, 83, 30, "2");
	// Release the context, so other threads can update the data.
    furi_mutex_release(fruttivendolo_context->mutex);
}



//-------------------------------------------------------------------------------------------------------------
void create_file(char* par1) {
    // We need a storage struct (gain accesso to the filesystem API )
    Storage* storage = furi_record_open(RECORD_STORAGE);

    // storage_file_alloc gives to us a File pointer using the Storage API.
    File* file = storage_file_alloc(storage);
	bool result =
        storage_file_open(file, INT_PATH(".blmode.config"), FSAM_WRITE, FSOM_OPEN_ALWAYS);
    char* content = (char*)malloc(sizeof(char) * BUFFER);
    content = par1;
	line_ = content;
    if(result) {
        // this function write data on a file
        storage_file_write(file, content, strlen(content));

        // Closing the "file descriptor"
        storage_file_close(file);

        // Freeing up memory
        storage_file_free(file);
    }
	return;
}

//-------------------------------------------------------------------------------------------------------------
void read_file() {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    // here I used FSOM_OPEN_EXISTING (Open file, fail if file doesn't exist)
    bool result =
        storage_file_open(file, INT_PATH(".blmode.config"), FSAM_READ_WRITE, FSOM_OPEN_ALWAYS);

    if(result) {
        int buffer_size = 128;
        char* buffer = (char*)malloc(sizeof(char) * buffer_size);

        // read the content of the file and insert into a buffer.
        storage_file_read(file, buffer, buffer_size);
		line_ = buffer;

        // free resources
        storage_file_close(file);
        storage_file_free(file);
    }
	return;
}

//------------------------------------------------------------------------------------------------------------------------------------

int32_t fruttivendolo_app(void* p) {
    UNUSED(p);

    // Configure our initial data.
    fruttivendoloContext* fruttivendolo_context = malloc(sizeof(fruttivendoloContext));
    fruttivendolo_context->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    fruttivendolo_context->data = malloc(sizeof(fruttivendoloData));
    fruttivendolo_context->data->buffer = furi_string_alloc();

    // Queue for events (tick or input)
    fruttivendolo_context->queue = furi_message_queue_alloc(8, sizeof(fruttivendoloEvent));

    // Set ViewPort callbacks
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, fruttivendolo_render_callback, fruttivendolo_context);
    view_port_input_callback_set(view_port, fruttivendolo_input_callback, fruttivendolo_context->queue);

    // Open GUI and register view_port
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // Main loop
    fruttivendoloEvent event;
    bool processing = true;
    read_file();
	do {
		if (furi_message_queue_get(fruttivendolo_context->queue, &event, FuriWaitForever) == FuriStatusOk) {
            
			//create_file();
			FURI_LOG_T(TAG, "Got event type: %d", event.type);
            switch (event.type) {
                case fruttivendoloEventTypeKey:
                    // Short press of back button exits the program.
                    if(event.input.type == InputTypeLong && event.input.key == InputKeyBack) {
                        FURI_LOG_I(TAG, "Short-Back pressed. Exiting program.");
                        processing = false;
                    }
					if(event.input.type == InputTypeShort && event.input.key == InputKeyBack) {
                        create_file("6");
						furi_hal_bt_stop_advertising();
						furi_hal_bt_start_advertising();
                    }
					if(event.input.type == InputTypeShort && event.input.key == InputKeyOk) {
                        create_file("5");
						furi_hal_bt_stop_advertising();
						furi_hal_bt_start_advertising();
                    }
					if(event.input.type == InputTypeShort && event.input.key == InputKeyUp) {
                        create_file("4");
						furi_hal_bt_stop_advertising();
						furi_hal_bt_start_advertising();
                    }
					if(event.input.type == InputTypeShort && event.input.key == InputKeyRight) {
                        create_file("3");
						furi_hal_bt_stop_advertising();
						furi_hal_bt_start_advertising();
                    }
					if(event.input.type == InputTypeShort && event.input.key == InputKeyLeft) {
                        create_file("2");
						furi_hal_bt_stop_advertising();
						furi_hal_bt_start_advertising();
                    }
					if(event.input.type == InputTypeShort && event.input.key == InputKeyDown) {
                        create_file("1");
						furi_hal_bt_stop_advertising();
						furi_hal_bt_start_advertising();
                    }
                    break;
                default:
                    break;
            }

            // Send signal to update the screen (callback will get invoked at some point later.)
            view_port_update(view_port);
        } else {
            // We had an issue getting message from the queue, so exit application.
            processing = false;
        }
    } while (processing);

    // Free resources
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);
    furi_message_queue_free(fruttivendolo_context->queue);
    furi_mutex_free(fruttivendolo_context->mutex);
    furi_string_free(fruttivendolo_context->data->buffer);
    free(fruttivendolo_context->data);
    free(fruttivendolo_context);

    return 0;
}
