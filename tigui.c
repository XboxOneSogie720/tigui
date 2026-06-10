#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <graphx.h>
#include <string.h>
#include <math.h>
#include <ti/getcsc.h>
#include "tigui.h"

#define MAX_PALETTE_INDICES 256
#define GRAPHX_TEXT_HEIGHT  10
#define TOGGLE_RADIUS       8
#define TOGGLE_LENGTH       16

#define reset_clip() gfx_SetClipRegion(0, 0, TIGUI_SCREEN_WIDTH, TIGUI_SCREEN_HEIGHT)

static uint16_t palette_i = 0;

static int add_color(uint16_t color) {
    if (palette_i >= MAX_PALETTE_INDICES) return -1;

    uint16_t idx = palette_i;
    gfx_palette[idx] = color;
    palette_i++;

    return idx;
}

static int get_palette_index_for_color(tigui_color_t color) {
    uint16_t match = gfx_RGBTo1555(color.r, color.g, color.b);

    for (uint16_t i = 1; i < palette_i; i++) {
        if (gfx_palette[i] == match) return i;
    }

    return add_color(match);
}

void tigui_init(void) {
    gfx_Begin();
    palette_i = 0;
    add_color(gfx_palette[255]);
    gfx_SetTransparentColor(0); // Transparency color, which is stored at index 255 in the (now old) palette.
    memset(gfx_palette+1, 0, sizeof(uint16_t) * (MAX_PALETTE_INDICES - 1));

    add_color(gfx_RGBTo1555(255, 255, 255)); // White
    add_color(gfx_RGBTo1555(0,   0,   0));   // Black
    add_color(gfx_RGBTo1555(255, 0,   0));   // Red
    add_color(gfx_RGBTo1555(255, 127, 0));   // Orange
    add_color(gfx_RGBTo1555(255, 255, 0));   // Yellow
    add_color(gfx_RGBTo1555(0,   255, 0));   // Green
    add_color(gfx_RGBTo1555(0,   0,   255)); // Blue
    add_color(gfx_RGBTo1555(128, 0,   128)); // Purple
    
    gfx_FillScreen(get_palette_index_for_color(TIGUI_WHITE));
}

void tigui_deinit(void) { gfx_End(); }

const char* tigui_strerror(tigui_error_t error) {
    switch (error) {
        case TIGUI_E_SUCCESS:
            return "Success.";
        case TIGUI_E_BAD_PARAM:
            return "A bad parameter was passed to a function.";
        case TIGUI_E_COLORSPACE_FULL:
            return "There is no room for a new color.";
        case TIGUI_E_INVALID_NODESET:
            return "The canvas nodeset is invalid.";
        case TIGUI_E_CANVAS_HAS_DUPLICATE_KEYS:
            return "Every navigation key in the canvas must be different.";
        case TIGUI_E_MULTIPLE_NODES_SELECTED:
            return "Multiple nodes in the canvas nodeset are selected. EXACTLY ONE must be selected.";
        case TIGUI_E_ELEMENT_NOT_SELECTABLE:
            return "An element was attempted to be selected that is not allowed to be.";
        case TIGUI_E_UNKNOWN_ELEMENT:
            return "An unknown element type was discovered while parsing.";
        case TIGUI_E_USER_EXIT:
            return "The user has manually exited the orchestrator.";
        case TIGUI_E_THIRDPARTY_ERROR:
            return "An underlying process has failed.";
        case TIGUI_E_ORCHESTRATOR_ALERT_TRACK_FAIL:
            return "(Core Failure): Failed to track alerts within the orchestrator.";
        default:
            return "Unknown error.";
    }
}

static bool nodeset_is_valid(tigui_canvas_node_t** nodeset, size_t num_nodes) {
    if (nodeset == NULL) return false;

    for (size_t i = 0; i < num_nodes; i++) {
        if (nodeset[i] == NULL) return false;
    }

    return true;
}

static bool tigui_keys_are_unique(const sk_key_t* keys, size_t count) {
    for (size_t i = 0; i < count; i++) {
        for (size_t j = i + 1; j < count; j++) {
            if (keys[i] == keys[j]) {
                return false;
            }
        }
    }

    return true;
}

static tigui_error_t draw_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, tigui_color_t color, bool filled);
static tigui_error_t draw_node(tigui_canvas_node_t* node, tigui_color_t canvas_bg_color) {
    if (node == NULL) return TIGUI_E_BAD_PARAM;

    switch (node->type) {
        case TIGUI_ELEMENT_LINE:
            return tigui_draw_line(node->element);
        case TIGUI_ELEMENT_RECTANGLE:
            return tigui_draw_rectangle(node->element);
        case TIGUI_ELEMENT_CIRCLE:
            return tigui_draw_circle(node->element);
        case TIGUI_ELEMENT_ELLIPSE:
            return tigui_draw_ellipse(node->element);
        case TIGUI_ELEMENT_TEXT:
            // TODO draw background for new text like we do for slider.
            return tigui_draw_text(node->element);
        case TIGUI_ELEMENT_TEXT_BOX:
            return tigui_draw_text_box(node->element);
        case TIGUI_ELEMENT_BUTTON:
            return tigui_draw_button(node->element);
        case TIGUI_ELEMENT_CHECKBOX:
            return tigui_draw_checkbox(node->element);
        case TIGUI_ELEMENT_RADIO_BUTTON:
            return tigui_draw_radio_button(node->element);
        case TIGUI_ELEMENT_TOGGLE:
            return tigui_draw_toggle(node->element);
        case TIGUI_ELEMENT_PROGRESS_BAR:
            return tigui_draw_progress_bar(node->element);
        case TIGUI_ELEMENT_SPINNER:
            return tigui_draw_spinner(node->element);
        case TIGUI_ELEMENT_SLIDER: {
            tigui_slider_t* slider = node->element;
            if (slider->update == false) return TIGUI_E_SUCCESS;
            uint16_t knob_radius = slider->h / 2 + 2;
            uint16_t draw_x = slider->x - (slider->w / 2);
            uint16_t draw_y = slider->y - (slider->h / 2);
            tigui_error_t err = draw_rectangle(draw_x - knob_radius, draw_y - knob_radius, slider->w + knob_radius * 2 + 1, slider->h + knob_radius * 2, canvas_bg_color, true); if (err != TIGUI_E_SUCCESS) return err;
            return tigui_draw_slider(slider);
        }
        case TIGUI_ELEMENT_BADGE:
            return tigui_draw_badge(node->element);
        case TIGUI_ELEMENT_INPUT_FIELD:
            return tigui_draw_input_field(node->element);
        case TIGUI_ELEMENT_ALERT:
            return tigui_draw_alert(node->element);
        case TIGUI_ELEMENT_LIST:
            return tigui_draw_list(node->element);
        default:
            return TIGUI_E_BAD_PARAM;
    }
}

static bool node_is_selected(tigui_canvas_node_t* node) {
    if (node == NULL || node->selectable == false) return false;

    switch (node->type) {
        case TIGUI_ELEMENT_LINE:
            return ((tigui_line_t*)node->element)->selected;
        case TIGUI_ELEMENT_RECTANGLE:
            return ((tigui_rectangle_t*)node->element)->selected;
        case TIGUI_ELEMENT_CIRCLE:
            return ((tigui_circle_t*)node->element)->selected;
        case TIGUI_ELEMENT_ELLIPSE:
            return ((tigui_ellipse_t*)node->element)->selected;
        case TIGUI_ELEMENT_TEXT_BOX:
            return ((tigui_text_box_t*)node->element)->selected;
        case TIGUI_ELEMENT_BUTTON:
            return ((tigui_button_t*)node->element)->selected;
        case TIGUI_ELEMENT_CHECKBOX:
            return ((tigui_checkbox_t*)node->element)->selected;
        case TIGUI_ELEMENT_RADIO_BUTTON:
            return ((tigui_radio_button_t*)node->element)->selected;
        case TIGUI_ELEMENT_TOGGLE:
            return ((tigui_toggle_t*)node->element)->selected;
        case TIGUI_ELEMENT_PROGRESS_BAR:
            return ((tigui_progress_bar_t*)node->element)->selected;
        case TIGUI_ELEMENT_SLIDER:
            return ((tigui_slider_t*)node->element)->selected;
        case TIGUI_ELEMENT_BADGE:
            return ((tigui_badge_t*)node->element)->selected;
        case TIGUI_ELEMENT_INPUT_FIELD:
            return ((tigui_input_field_t*)node->element)->selected;
        case TIGUI_ELEMENT_LIST:
            return ((tigui_list_t*)node->element)->selected;
        case TIGUI_ELEMENT_SPINNER:
        case TIGUI_ELEMENT_TEXT:
        case TIGUI_ELEMENT_ALERT:
        default:
            return false;
    }
}

static tigui_error_t draw_canvas_nodeset(tigui_canvas_t* canvas, tigui_canvas_node_t** currently_selected_node) {
    if (canvas == NULL || currently_selected_node == NULL || *currently_selected_node != NULL) return TIGUI_E_BAD_PARAM;

    tigui_error_t err;
    for (size_t i = 0; i < canvas->num_nodes; i++) {
        err = draw_node(canvas->nodeset[i], canvas->bg_color); if (err != TIGUI_E_SUCCESS) return err;

        if (node_is_selected(canvas->nodeset[i])) {
            if (*currently_selected_node != NULL) {
                return TIGUI_E_MULTIPLE_NODES_SELECTED;
            } else {
                *currently_selected_node = canvas->nodeset[i];
            }
        }
    }

    return TIGUI_E_SUCCESS;
}

static tigui_error_t set_node_element_selection_state(tigui_canvas_node_t* node, bool new_selection_state) {
    if (node == NULL || node->element == NULL) return TIGUI_E_BAD_PARAM;

    switch (node->type) {
        case TIGUI_ELEMENT_LINE: {
            tigui_line_t* line = node->element;
            line->selected = new_selection_state;
            line->update   = true;
            break;
        }
        case TIGUI_ELEMENT_RECTANGLE: {
            tigui_rectangle_t* rectangle = node->element;
            rectangle->selected = new_selection_state;
            rectangle->update   = true;
            break;
        }
        case TIGUI_ELEMENT_CIRCLE: {
            tigui_circle_t* circle = node->element;
            circle->selected = new_selection_state;
            circle->update   = true;
            break;
        }
        case TIGUI_ELEMENT_ELLIPSE: {
            tigui_ellipse_t* ellipse = node->element;
            ellipse->selected = new_selection_state;
            ellipse->update   = true;
            break;
        }
        case TIGUI_ELEMENT_TEXT_BOX: {
            tigui_text_box_t* text_box = node->element;
            text_box->selected = new_selection_state;
            text_box->update   = true;
            break;
        }
        case TIGUI_ELEMENT_BUTTON: {
            tigui_button_t* button = node->element;
            button->selected = new_selection_state;
            button->update   = true;
            break;
        }
        case TIGUI_ELEMENT_CHECKBOX: {
            tigui_checkbox_t* checkbox = node->element;
            checkbox->selected = new_selection_state;
            checkbox->update   = true;
            break;
        }
        case TIGUI_ELEMENT_RADIO_BUTTON: {
            tigui_radio_button_t* radio_button = node->element;
            radio_button->selected = new_selection_state;
            radio_button->update   = true;
            break;
        }
        case TIGUI_ELEMENT_TOGGLE: {
            tigui_toggle_t* toggle = node->element;
            toggle->selected = new_selection_state;
            toggle->update   = true;
            break;
        }
        case TIGUI_ELEMENT_PROGRESS_BAR: {
            tigui_progress_bar_t* progress_bar = node->element;
            progress_bar->selected = new_selection_state;
            progress_bar->update   = true;
            break;
        }
        case TIGUI_ELEMENT_SLIDER: {
            tigui_slider_t* slider = node->element;
            slider->selected = new_selection_state;
            slider->update   = true;
            break;
        }
        case TIGUI_ELEMENT_BADGE: {
            tigui_badge_t* badge = node->element;
            badge->selected = new_selection_state;
            badge->update   = true;
            break;
        }
        case TIGUI_ELEMENT_INPUT_FIELD: {
            tigui_input_field_t* input_field = node->element;
            input_field->selected = new_selection_state;
            input_field->update   = true;
            break;
        }
        case TIGUI_ELEMENT_LIST: {
            tigui_list_t* list = node->element;
            list->selected = new_selection_state;
            list->update   = true;
            break;
        }
        case TIGUI_ELEMENT_TEXT:
        case TIGUI_ELEMENT_SPINNER:
        case TIGUI_ELEMENT_ALERT:
            return TIGUI_E_ELEMENT_NOT_SELECTABLE;
        default:
            return TIGUI_E_UNKNOWN_ELEMENT;
    }

    return TIGUI_E_SUCCESS;
}

static tigui_error_t handle_selection_transfer(tigui_canvas_node_t** currently_selected, tigui_canvas_node_t* to_be_selected, tigui_color_t canvas_bg_color) {
    if (currently_selected == NULL || *currently_selected == NULL || to_be_selected == NULL) return TIGUI_E_BAD_PARAM;
    if (to_be_selected->selectable == false) return TIGUI_E_ELEMENT_NOT_SELECTABLE;

    tigui_error_t err;

    err = set_node_element_selection_state(*currently_selected, false); if (err != TIGUI_E_SUCCESS) return err;
    err = set_node_element_selection_state(to_be_selected, true);       if (err != TIGUI_E_SUCCESS) return err;
    err = draw_node(*currently_selected, canvas_bg_color);              if (err != TIGUI_E_SUCCESS) return err;
    err = draw_node(to_be_selected, canvas_bg_color);                   if (err != TIGUI_E_SUCCESS) return err;

    *currently_selected = to_be_selected;

    return TIGUI_E_SUCCESS;
}

static tigui_error_t set_node_element_update_state(tigui_canvas_node_t* node, bool new_update_state) {
    if (node == NULL || node->element == NULL) return TIGUI_E_BAD_PARAM;

    switch (node->type) {
        case TIGUI_ELEMENT_LINE: {
            tigui_line_t* line = node->element;
            line->update = new_update_state;
            break;
        }
        case TIGUI_ELEMENT_RECTANGLE: {
            tigui_rectangle_t* rectangle = node->element;
            rectangle->update = new_update_state;
            break;
        }
        case TIGUI_ELEMENT_CIRCLE: {
            tigui_circle_t* circle = node->element;
            circle->update = new_update_state;
            break;
        }
        case TIGUI_ELEMENT_ELLIPSE: {
            tigui_ellipse_t* ellipse = node->element;
            ellipse->update = new_update_state;
            break;
        }
        case TIGUI_ELEMENT_TEXT: {
            tigui_text_t* text = node->element;
            text->update = new_update_state;
            break;
        }
        case TIGUI_ELEMENT_TEXT_BOX: {
            tigui_text_box_t* text_box = node->element;
            text_box->update = new_update_state;
            break;
        }
        case TIGUI_ELEMENT_BUTTON: {
            tigui_button_t* button = node->element;
            button->update = new_update_state;
            break;
        }
        case TIGUI_ELEMENT_CHECKBOX: {
            tigui_checkbox_t* checkbox = node->element;
            checkbox->update = new_update_state;
            break;
        }
        case TIGUI_ELEMENT_RADIO_BUTTON: {
            tigui_radio_button_t* radio_button = node->element;
            radio_button->update = new_update_state;
            break;
        }
        case TIGUI_ELEMENT_TOGGLE: {
            tigui_toggle_t* toggle = node->element;
            toggle->update = new_update_state;
            break;
        }
        case TIGUI_ELEMENT_PROGRESS_BAR: {
            tigui_progress_bar_t* progress_bar = node->element;
            progress_bar->update = new_update_state;
            break;
        }
        case TIGUI_ELEMENT_SPINNER: {
            tigui_spinner_t* spinner = node->element;
            spinner->update = new_update_state;
            break;
        }
        case TIGUI_ELEMENT_SLIDER: {
            tigui_slider_t* slider = node->element;
            slider->update = new_update_state;
            break;
        }
        case TIGUI_ELEMENT_BADGE: {
            tigui_badge_t* badge = node->element;
            badge->update = new_update_state;
            break;
        }
        case TIGUI_ELEMENT_INPUT_FIELD: {
            tigui_input_field_t* input_field = node->element;
            input_field->update = new_update_state;
            break;
        }
        case TIGUI_ELEMENT_ALERT: {
            tigui_alert_t* alert = node->element;
            alert->update = new_update_state;
            break;
        }
        case TIGUI_ELEMENT_LIST: {
            tigui_list_t* list = node->element;
            list->update = new_update_state;
            break;
        }
        default:
            return TIGUI_E_UNKNOWN_ELEMENT;
    }

    return TIGUI_E_SUCCESS;
}

static tigui_error_t draw_canvas(tigui_canvas_t* canvas, tigui_canvas_node_t** currently_selected_node) {
    if (canvas == NULL || currently_selected_node == NULL) return TIGUI_E_BAD_PARAM;

    tigui_error_t err = tigui_bgcolor(canvas->bg_color); if (err != TIGUI_E_SUCCESS) return err;

    for (size_t i = 0; i < canvas->num_nodes; i++) {
        err = set_node_element_update_state(canvas->nodeset[i], true); if (err != TIGUI_E_SUCCESS) return err;
    }

    err = draw_canvas_nodeset(canvas, currently_selected_node);

    return err;
}

static void tick_spinners(tigui_canvas_t* canvas) {
    if (canvas == NULL || canvas->nodeset == NULL) return;

    for (size_t i = 0; i < canvas->num_nodes; i++) {
        if (canvas->nodeset[i]->type != TIGUI_ELEMENT_SPINNER || canvas->nodeset[i]->element == NULL) continue;
        
        tigui_spinner_t* spinner = canvas->nodeset[i]->element;
        spinner->tick++;
        
        if (spinner->tick >= spinner->speed) {
            spinner->tick = 0;
            spinner->current_frame = (spinner->current_frame + 1) % spinner->num_dots;
            spinner->update = true;
        }
    }
}

static tigui_error_t draw_spinners(tigui_canvas_t* canvas) {
    if (canvas == NULL || canvas->nodeset == NULL) return TIGUI_E_BAD_PARAM;

    tigui_error_t err;
    for (size_t i = 0; i < canvas->num_nodes; i++) {
        if (canvas->nodeset[i]->type != TIGUI_ELEMENT_SPINNER || canvas->nodeset[i]->element == NULL) continue;

        err = draw_node(canvas->nodeset[i], canvas->bg_color); if (err != TIGUI_E_SUCCESS) return err;
    }

    return TIGUI_E_SUCCESS;
}

typedef enum {
    LEFT,
    RIGHT
} tigui_navigation_direction_t;

static tigui_error_t step_slider(tigui_slider_t* slider, tigui_navigation_direction_t direction) {
    if (slider == NULL) return TIGUI_E_BAD_PARAM;

    // Slide
    switch (direction) {
        case LEFT: 
            slider->slid -= slider->step;
            break;
        case RIGHT:
            slider->slid += slider->step;
            break;
        default:
            return TIGUI_E_BAD_PARAM;
    }

    // Clamping is handled in tigui_draw_slider().

    slider->update = true;

    return TIGUI_E_SUCCESS;
}

static tigui_error_t sync_radio_buttons_by_id(tigui_canvas_node_t* newly_filled, tigui_canvas_t* canvas) {
    if (newly_filled == NULL || newly_filled->type != TIGUI_ELEMENT_RADIO_BUTTON || canvas == NULL || canvas->nodeset == NULL) return TIGUI_E_BAD_PARAM;

    tigui_radio_button_t* newly_filled_rb = newly_filled->element;
    if (newly_filled_rb->filled != true) {
        newly_filled_rb->filled = true;
        newly_filled_rb->update = true;
    }
    tigui_error_t err = draw_node(newly_filled, canvas->bg_color);

    for (size_t i = 0; i < canvas->num_nodes; i++) {
        if (canvas->nodeset[i] == newly_filled || canvas->nodeset[i]->type != TIGUI_ELEMENT_RADIO_BUTTON) continue;
        
        tigui_radio_button_t* radio_button = canvas->nodeset[i]->element;
        if (radio_button->group_id != newly_filled_rb->group_id) continue;

        if (radio_button->filled == true) {
            radio_button->filled = false;
            radio_button->update = true;
        }

        err = draw_node(canvas->nodeset[i], canvas->bg_color); if (err != TIGUI_E_SUCCESS) return err;
    }

    return TIGUI_E_SUCCESS;
}

static tigui_error_t toggle_node_element(tigui_canvas_node_t* node, tigui_canvas_t* canvas) {
    if (node == NULL) return TIGUI_E_BAD_PARAM;

    switch (node->type) {
        case TIGUI_ELEMENT_CHECKBOX: {
            tigui_checkbox_t* checkbox = node->element;
            checkbox->checked = !checkbox->checked;
            checkbox->update = true;
            break;
        }
        case TIGUI_ELEMENT_TOGGLE: {
            tigui_toggle_t* toggle = node->element;
            toggle->on = !toggle->on;
            toggle->update = true;
            break;
        }
        default:
            return TIGUI_E_UNKNOWN_ELEMENT;
    }

    return draw_node(node, canvas->bg_color);
}

static tigui_error_t handle_default_node_press(tigui_canvas_t* canvas, tigui_canvas_node_t* currently_selected_node) {
    if (currently_selected_node == NULL) return TIGUI_E_BAD_PARAM;

    switch (currently_selected_node->type) {
        case TIGUI_ELEMENT_CHECKBOX:
        case TIGUI_ELEMENT_TOGGLE:
            return toggle_node_element(currently_selected_node, canvas);
        case TIGUI_ELEMENT_RADIO_BUTTON:
            return sync_radio_buttons_by_id(currently_selected_node, canvas);
        case TIGUI_ELEMENT_LINE:
        case TIGUI_ELEMENT_RECTANGLE:
        case TIGUI_ELEMENT_CIRCLE:
        case TIGUI_ELEMENT_ELLIPSE:
        case TIGUI_ELEMENT_TEXT:
        case TIGUI_ELEMENT_TEXT_BOX:
        case TIGUI_ELEMENT_BUTTON:
        case TIGUI_ELEMENT_PROGRESS_BAR:
        case TIGUI_ELEMENT_SPINNER:
        case TIGUI_ELEMENT_SLIDER: // Uses hijack rule
        case TIGUI_ELEMENT_BADGE:
        case TIGUI_ELEMENT_INPUT_FIELD: // Uses hijack rule
        case TIGUI_ELEMENT_ALERT: // Uses special orchestrator
        case TIGUI_ELEMENT_LIST: // Uses hijack rule
            return TIGUI_E_SUCCESS; // NOP
        default:
            return TIGUI_E_UNKNOWN_ELEMENT;
    }

    return TIGUI_E_SUCCESS;
}

static char map_keypress_to_char(sk_key_t key_pressed, const tigui_keymap_entry_t* keymap_entires) {
    if (keymap_entires == NULL || key_pressed == 0) return 0;

    for (int i = 0; keymap_entires[i].key != 0 && keymap_entires[i].c != '\0'; i++) {
        if (keymap_entires[i].key == key_pressed) return keymap_entires[i].c;
    }

    return 0;
}

static tigui_error_t shift_input_field_offset(tigui_input_field_t* input_field, tigui_navigation_direction_t direction) {
    if (input_field == NULL || input_field->input_buf == NULL || input_field->input_buf_size <= 1) return TIGUI_E_BAD_PARAM;

    switch (direction) {
        case LEFT:
            if (input_field->input_buf_offset == 0) return TIGUI_E_SUCCESS;
            input_field->input_buf_offset--;
            input_field->update = true;
            break;
        case RIGHT:
            if (input_field->input_buf_offset >= strlen(input_field->input_buf)) return TIGUI_E_SUCCESS;
            input_field->input_buf_offset++;
            input_field->update = true;
            break;
        default:
            return TIGUI_E_BAD_PARAM;
    }
    
    return TIGUI_E_SUCCESS;
}

static tigui_error_t remove_char_from_input_field_at_current_offset(tigui_input_field_t* input_field) {
    if (input_field == NULL || input_field->input_buf == NULL || input_field->input_buf_size <= 1) {
        return TIGUI_E_BAD_PARAM;
    } else if (input_field->input_buf_offset == 0) {
        return TIGUI_E_SUCCESS;
    }

    size_t len = strlen(input_field->input_buf);

    memmove(&input_field->input_buf[input_field->input_buf_offset - 1], &input_field->input_buf[input_field->input_buf_offset], len - input_field->input_buf_offset + 1);
    input_field->input_buf_offset--;
    input_field->update = true;

    return TIGUI_E_SUCCESS;
}

static tigui_error_t insert_char_in_input_field_at_current_offset(const char c, tigui_input_field_t* input_field) {
    if (c == 0 || input_field == NULL || input_field->input_buf == NULL || input_field->input_buf_size <= 1) return TIGUI_E_BAD_PARAM;

    size_t len = strlen(input_field->input_buf);
    if (len + 1 >= input_field->input_buf_size) return TIGUI_E_SUCCESS;

    memmove(&input_field->input_buf[input_field->input_buf_offset + 1], &input_field->input_buf[input_field->input_buf_offset], len - input_field->input_buf_offset + 1);
    input_field->input_buf[input_field->input_buf_offset] = c;
    input_field->input_buf_offset++;
    input_field->update = true;

    return TIGUI_E_SUCCESS;
}

static tigui_alert_t* find_showing_alerts(tigui_canvas_t* canvas) {
    if (canvas == NULL) return NULL;

    for (size_t i = 0; i < canvas->num_nodes; i++) {
        if (canvas->nodeset[i]->type != TIGUI_ELEMENT_ALERT) continue;

        tigui_alert_t* alert = canvas->nodeset[i]->element;

        if (alert->is_showing == true) return alert;
    }

    return NULL;
}

static tigui_error_t orchestrate_alert(tigui_canvas_t* canvas, tigui_alert_t* alert) {
    if (canvas == NULL || alert == NULL) return TIGUI_E_BAD_PARAM;

    tigui_error_t err;
    sk_key_t key_pressed = 0;
    while (alert->is_showing == true) {
        err = tigui_draw_alert(alert); if (err != TIGUI_E_SUCCESS) return err;

        key_pressed = os_GetCSC();

        if (key_pressed == canvas->left_key && alert->fail_button_label != NULL) {
            if (alert->selection_i != 0) {
                alert->selection_i = 0;
                alert->update = true;
            }
        } else if (key_pressed == canvas->right_key && alert->fail_button_label != NULL) {
            if (alert->selection_i != 1) {
                alert->selection_i = 1;
                alert->update = true;
            }
        } else if (key_pressed == canvas->enter_key) {
            alert->is_showing = false;
            
            if (alert->selection_i == 0 && alert->fail_button_label != NULL) {
                if (alert->fail_cb != NULL) alert->fail_cb(canvas->user_data);
            } else {
                if (alert->pass_cb != NULL) alert->pass_cb(canvas->user_data);
            }
        }
    }

    return TIGUI_E_SUCCESS;
}

/**
 * Hijack rules are used for elements that need to poll keypad input.
 * Instead of spinning up a new loop inside of handle_default_node_press(),
 * we just use the regular draw loop inside tigui_orchestrate_canvas().
 * 
 * This allows other elements to still work like the spinner, which is driven by the main loop.
 */
typedef enum {
    TIGUI_HIJACK_RULE_FOLLOW_NODE, // Follows the navigation pointers in the currently selected node.
    TIGUI_HIJACK_RULE_SLIDER,      // Slides the slider left or right.
    TIGUI_HIJACK_RULE_INPUT_FIELD, // Gets full keypad input.
    TIGUI_HIJACK_RULE_LIST
} tigui_hijack_rule_t;

tigui_error_t tigui_orchestrate_canvas(tigui_canvas_t* canvas) {
    if (canvas == NULL) return TIGUI_E_BAD_PARAM;
    if (nodeset_is_valid(canvas->nodeset, canvas->num_nodes) != true) return TIGUI_E_INVALID_NODESET;
    
    sk_key_t keys[] = {
        canvas->exit_key,
        canvas->enter_key,
        canvas->delete_key,
        canvas->up_key,
        canvas->down_key,
        canvas->left_key,
        canvas->right_key
    };
    if (tigui_keys_are_unique(keys, sizeof(keys) / sizeof(keys[0])) != true) return TIGUI_E_CANVAS_HAS_DUPLICATE_KEYS;

    tigui_canvas_node_t* currently_selected_node = NULL;
    tigui_error_t err = draw_canvas(canvas, &currently_selected_node); if (err != TIGUI_E_SUCCESS) return err;

    /* Handling loop */
    sk_key_t key_pressed = 0;
    tigui_hijack_rule_t hijack_rule = TIGUI_HIJACK_RULE_FOLLOW_NODE;
    tigui_alert_t* alert = NULL;
    while (canvas->is_running == true && err == TIGUI_E_SUCCESS) {
        if ((key_pressed = os_GetCSC()) == canvas->exit_key && hijack_rule == TIGUI_HIJACK_RULE_FOLLOW_NODE) return TIGUI_E_USER_EXIT;

        /* Handle special elements */
        tick_spinners(canvas);
        err = draw_spinners(canvas); if (err != TIGUI_E_SUCCESS) return err;

        if ((alert = find_showing_alerts(canvas)) != NULL) {
            err = orchestrate_alert(canvas, alert); if (err != TIGUI_E_SUCCESS) return err;
            alert = NULL;
            
            if (canvas->is_running == true) {
                // Prevent flicker draw if a node shuts down the canvas.
                currently_selected_node = NULL;
                err = draw_canvas(canvas, &currently_selected_node);
            }
        }
        
        /* Navigation */
        if (currently_selected_node != NULL) {
            switch (hijack_rule) {
                case TIGUI_HIJACK_RULE_FOLLOW_NODE: {
                    if (key_pressed == canvas->enter_key) {
                        err = handle_default_node_press(canvas, currently_selected_node); if (err != TIGUI_E_SUCCESS) return err;

                        /* Handle hijack rules */
                        if (currently_selected_node->type == TIGUI_ELEMENT_SLIDER) {
                            hijack_rule = TIGUI_HIJACK_RULE_SLIDER;
                        } else if (currently_selected_node->type == TIGUI_ELEMENT_INPUT_FIELD) {
                            hijack_rule = TIGUI_HIJACK_RULE_INPUT_FIELD;
                        } else if (currently_selected_node->type == TIGUI_ELEMENT_LIST) {
                            hijack_rule = TIGUI_HIJACK_RULE_LIST;
                        }
                        
                        if (currently_selected_node->on_pressed != NULL) {
                            bool redraw_ground_up = currently_selected_node->on_pressed(canvas->user_data);
                            currently_selected_node = NULL;
                            
                            if (redraw_ground_up) {
                                err = draw_canvas(canvas, &currently_selected_node);
                            } else {
                                err = draw_canvas_nodeset(canvas, &currently_selected_node);
                            }
                        }
                    }
                    // 1. Change element's selection state
                    // 2. Change currently selected node
                    // 3. Redraw both nodes
                    else if (key_pressed == canvas->up_key) {
                        if (currently_selected_node->up == NULL || currently_selected_node->up->selectable == false) break;
                        err = handle_selection_transfer(&currently_selected_node, currently_selected_node->up, canvas->bg_color);
                    } else if (key_pressed == canvas->down_key) {
                        if (currently_selected_node->down == NULL || currently_selected_node->down->selectable == false) break;
                        err = handle_selection_transfer(&currently_selected_node, currently_selected_node->down, canvas->bg_color);
                    } else if (key_pressed == canvas->left_key) {
                        if (currently_selected_node->left == NULL || currently_selected_node->left->selectable == false) break;
                        err = handle_selection_transfer(&currently_selected_node, currently_selected_node->left, canvas->bg_color);
                    } else if (key_pressed == canvas->right_key) {
                        if (currently_selected_node->right == NULL || currently_selected_node->right->selectable == false) break;
                        err = handle_selection_transfer(&currently_selected_node, currently_selected_node->right, canvas->bg_color);
                    }

                    break;
                }

                case TIGUI_HIJACK_RULE_SLIDER: {
                    if (key_pressed == canvas->enter_key || key_pressed == canvas->exit_key) {
                        hijack_rule = TIGUI_HIJACK_RULE_FOLLOW_NODE;
                    } else if (key_pressed == canvas->left_key || key_pressed == canvas->right_key) {
                        err = step_slider(currently_selected_node->element, key_pressed == canvas->left_key ? LEFT : RIGHT); if (err != TIGUI_E_SUCCESS) return err;
                        err = draw_node(currently_selected_node, canvas->bg_color); if (err != TIGUI_E_SUCCESS) return err;
                    }

                    break;
                }

                case TIGUI_HIJACK_RULE_INPUT_FIELD: {
                    tigui_input_field_t* input_field = currently_selected_node->element;
                    if (input_field->draw_cursor == false) {
                        input_field->draw_cursor = true;
                        input_field->update = true;
                        err = draw_node(currently_selected_node, canvas->bg_color); if (err != TIGUI_E_SUCCESS) return err;
                    }

                    if (key_pressed == canvas->enter_key || key_pressed == canvas->exit_key) {
                        hijack_rule = TIGUI_HIJACK_RULE_FOLLOW_NODE;
                        input_field->draw_cursor = false;
                        input_field->update = true;
                    } else if (key_pressed == canvas->left_key || key_pressed == canvas->right_key) {
                        err = shift_input_field_offset(currently_selected_node->element, key_pressed == canvas->left_key ? LEFT : RIGHT); if (err != TIGUI_E_SUCCESS) return err;
                    } else if (key_pressed == canvas->delete_key) {
                        err = remove_char_from_input_field_at_current_offset(currently_selected_node->element); if (err != TIGUI_E_SUCCESS) return err;
                    } else if (key_pressed == input_field->descriptor_switch_key) {
                        input_field->current_descriptor_i = (input_field->current_descriptor_i + 1) % input_field->num_descriptors;
                        input_field->update = true;
                    } else {
                        char typed;
                        if ((typed = map_keypress_to_char(key_pressed, input_field->descriptors[input_field->current_descriptor_i]->entries)) == 0) break;
                        err = insert_char_in_input_field_at_current_offset(typed, input_field); if (err != TIGUI_E_SUCCESS) return err;
                    }

                    err = draw_node(currently_selected_node, canvas->bg_color); if (err != TIGUI_E_SUCCESS) return err;

                    break;
                }

                case TIGUI_HIJACK_RULE_LIST: {
                    tigui_list_t* list = currently_selected_node->element;
                    if (list->is_browsing != true) {
                        list->is_browsing = true;
                        err = set_node_element_update_state(currently_selected_node, true); if (err != TIGUI_E_SUCCESS) return err;
                        err = draw_node(currently_selected_node, canvas->bg_color);         if (err != TIGUI_E_SUCCESS) return err;
                    }

                    if (key_pressed == canvas->exit_key) {
                        hijack_rule = TIGUI_HIJACK_RULE_FOLLOW_NODE;
                        list->is_browsing = false;
                        err = set_node_element_update_state(currently_selected_node, true); if (err != TIGUI_E_SUCCESS) return err;
                        err = draw_node(currently_selected_node, canvas->bg_color);         if (err != TIGUI_E_SUCCESS) return err;
                    } else if (key_pressed == canvas->up_key) {
                        if (list->selected_i > 0) {
                            list->selected_i--;
                            if (list->selected_i < list->scroll_offset_i) {
                                list->scroll_offset_i = list->selected_i;
                            }
                            list->update = true;
                            err = draw_node(currently_selected_node, canvas->bg_color);
                            if (err != TIGUI_E_SUCCESS) return err;
                        }
                    } else if (key_pressed == canvas->down_key) {
                        if (list->selected_i < list->num_items - 1) {
                            list->selected_i++;
                            size_t visible_items = list->h / list->item_h;
                            if (list->selected_i >= list->scroll_offset_i + visible_items) {
                                list->scroll_offset_i = list->selected_i - visible_items + 1;
                            }
                            list->update = true;
                            err = draw_node(currently_selected_node, canvas->bg_color);
                            if (err != TIGUI_E_SUCCESS) return err;
                        }
                    } else if (key_pressed == canvas->enter_key) {
                        if (list->items[list->selected_i].pressed_cb != NULL) {
                            list->items[list->selected_i].pressed_cb(list->items[list->selected_i].text, canvas->user_data);
                        }
                    }

                    break;
                }

                default: { break; } // NOP
            }
        }

        /* Draw complete */
        if (canvas->on_draw_complete != NULL && canvas->on_draw_complete(canvas->user_data) == true) {
            currently_selected_node = NULL;
            err = draw_canvas_nodeset(canvas, &currently_selected_node); if (err != TIGUI_E_SUCCESS) return err;
        }
    }

    return err;
}

uint16_t tigui_get_total_palette_slots(void) { return MAX_PALETTE_INDICES; }
uint16_t tigui_get_num_palette_slots_used(void)  { return palette_i; }

tigui_color_t tigui_1555_to_color(uint16_t color) {
    return (tigui_color_t){
        .r = (uint8_t)((((color) >> 10) & 0x1F) * 255 / 31),
        .g = (uint8_t)((((color) >> 5)  & 0x1F) * 255 / 31),
        .b = (uint8_t)(((color)         & 0x1F) * 255 / 31)
    };
}

uint16_t tigui_get_font_character_height(void) { return GRAPHX_TEXT_HEIGHT; }

tigui_error_t tigui_bgcolor(tigui_color_t color) {
    int idx = get_palette_index_for_color(color);
    if (idx == -1) {
        return TIGUI_E_COLORSPACE_FULL;
    } else {
        gfx_FillScreen(idx);
    }

    return TIGUI_E_SUCCESS;
}

static tigui_error_t draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, tigui_color_t color) {
    int color_i = get_palette_index_for_color(color);
    if (color_i == -1) return TIGUI_E_COLORSPACE_FULL;
    
    gfx_SetColor(color_i);
    gfx_Line(x0, y0, x1, y1);
    
    return TIGUI_E_SUCCESS;
}

tigui_error_t tigui_draw_line(tigui_line_t* line) {
    if (line == NULL) return TIGUI_E_BAD_PARAM;
    if (line->update == false) return TIGUI_E_SUCCESS;

    tigui_error_t err = draw_line(line->x0, line->y0, line->x1, line->y1, line->selected ? line->selected_color : line->color); if (err != TIGUI_E_SUCCESS) return err;

    line->update = false;
    return TIGUI_E_SUCCESS;
}

static tigui_error_t draw_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, tigui_color_t color, bool filled) {
    int color_i = get_palette_index_for_color(color);
    if (color_i == -1) return TIGUI_E_COLORSPACE_FULL;
    
    gfx_SetColor(color_i);
    if (filled == true) {
        gfx_FillRectangle(x, y, w, h);
    } else {
        gfx_Rectangle(x, y, w, h);
    }
    
    return TIGUI_E_SUCCESS;
}

tigui_error_t tigui_draw_rectangle(tigui_rectangle_t* rectangle) {
    if (rectangle == NULL) return TIGUI_E_BAD_PARAM;
    if (rectangle->update == false) return TIGUI_E_SUCCESS;

    uint16_t draw_x = rectangle->x - (rectangle->w / 2);
    uint16_t draw_y = rectangle->y - (rectangle->h / 2);
    tigui_error_t err;

    if (rectangle->filled == true) {
        err = draw_rectangle(draw_x, draw_y, rectangle->w, rectangle->h, rectangle->filled_color, true); if (err != TIGUI_E_SUCCESS) return err;
    }

    err = draw_rectangle(draw_x, draw_y, rectangle->w, rectangle->h, rectangle->selected ? rectangle->selected_bdr_color : rectangle->bdr_color, false); if (err != TIGUI_E_SUCCESS) return err;

    rectangle->update = false;
    return TIGUI_E_SUCCESS;
}

static tigui_error_t draw_circle(uint16_t x, uint16_t y, uint16_t radius, tigui_color_t color, bool filled) {
    int color_i = get_palette_index_for_color(color);
    if (color_i == -1) return TIGUI_E_COLORSPACE_FULL;
    
    gfx_SetColor(color_i);
    if (filled == true) {
        gfx_FillCircle(x, y, radius);
    } else {
        gfx_Circle(x, y, radius);
    }
    
    return TIGUI_E_SUCCESS;
}

tigui_error_t tigui_draw_circle(tigui_circle_t* circle) {
    if (circle == NULL) return TIGUI_E_BAD_PARAM;
    if (circle->update == false) return TIGUI_E_SUCCESS;

    tigui_error_t err;

    if (circle->filled == true) {
        err = draw_circle(circle->x, circle->y, circle->radius, circle->filled_color, true); if (err != TIGUI_E_SUCCESS) return err;
    }

    err = draw_circle(circle->x, circle->y, circle->radius, circle->selected ? circle->selected_bdr_color : circle->bdr_color, false); if (err != TIGUI_E_SUCCESS) return err;

    circle->update = false;
    return TIGUI_E_SUCCESS;
}

static tigui_error_t draw_ellipse(uint16_t x, uint16_t y, uint16_t h_radius, uint16_t v_radius, tigui_color_t color, bool filled) {
    int color_i = get_palette_index_for_color(color);
    if (color_i == -1) return TIGUI_E_COLORSPACE_FULL;
    
    gfx_SetColor(color_i);
    if (filled == true) {
        gfx_FillEllipse(x, y, h_radius, v_radius);
    } else {
        gfx_Ellipse(x, y, h_radius, v_radius);
    }
    
    return TIGUI_E_SUCCESS;
}


tigui_error_t tigui_draw_ellipse(tigui_ellipse_t* ellipse) {
    if (ellipse == NULL) return TIGUI_E_BAD_PARAM;
    if (ellipse->update == false) return TIGUI_E_SUCCESS;

    tigui_error_t err;

    if (ellipse->filled == true) {
        err = draw_ellipse(ellipse->x, ellipse->y, ellipse->h_radius, ellipse->v_radius, ellipse->filled_color, true); if (err != TIGUI_E_SUCCESS) return err;
    }

    err = draw_ellipse(ellipse->x, ellipse->y, ellipse->h_radius, ellipse->v_radius, ellipse->selected ? ellipse->selected_bdr_color : ellipse->bdr_color, false); if (err != TIGUI_E_SUCCESS) return err;

    ellipse->update = false;
    return TIGUI_E_SUCCESS;
}

static uint16_t get_line_width(const char* text, uint16_t x_clip, uint16_t start_x) {
    uint16_t width = 0;
    for (size_t i = 0; text[i] != '\0' && text[i] != '\n'; i++) {
        uint16_t cw = gfx_GetCharWidth(text[i]);
        if (start_x + width + cw >= x_clip) break;
        width += cw;
    }
    return width;
}

static uint16_t get_aligned_start_x(uint16_t x, uint16_t line_w, tigui_text_reference_point_t alignment) {
    switch (alignment) {
        case TIGUI_TEXT_CENTER_POINT: return x - (line_w / 2);
        case TIGIU_TEXT_RIGHT_POINT:  return x - line_w;
        default:                        return x;
    }
}

static tigui_error_t draw_text(const char* text, uint16_t x, uint16_t y, uint16_t x_clip, uint16_t y_clip, tigui_color_t color, bool wrap, tigui_text_reference_point_t alignment) {
    if (text == NULL) return TIGUI_E_BAD_PARAM;

    int color_i = get_palette_index_for_color(color);
    if (color_i == -1) return TIGUI_E_COLORSPACE_FULL;

    gfx_SetTextFGColor(color_i);
    gfx_SetTextBGColor(0);
    gfx_SetTextTransparentColor(0);
    gfx_SetTextConfig(gfx_text_clip);
    gfx_SetClipRegion(0, y, x_clip, y_clip);

    uint16_t line_w = get_line_width(text, x_clip, 0);
    uint16_t start_x = get_aligned_start_x(x, line_w, alignment);
    gfx_SetTextXY(start_x, y);

    for (size_t i = 0; text[i] != '\0'; i++) {
        char c = text[i];

        switch (c) {
            case '\t':
            case '\a':
            case '\b':
            case '\f':
            case '\v':
            case '\r':
                continue;
            case '\n': {
                if (wrap == false) continue;
                uint16_t next_y = gfx_GetTextY() + GRAPHX_TEXT_HEIGHT;
                if (next_y + GRAPHX_TEXT_HEIGHT >= y_clip) goto done;
                line_w = get_line_width(&text[i + 1], x_clip, 0);
                start_x = get_aligned_start_x(x, line_w, alignment);
                gfx_SetTextXY(start_x, next_y);
                continue;
            }
        }

        if (wrap == true) {
            if ((gfx_GetTextX() + gfx_GetCharWidth(c)) >= x_clip) {
                uint16_t next_y = gfx_GetTextY() + GRAPHX_TEXT_HEIGHT;
                if (next_y + GRAPHX_TEXT_HEIGHT >= y_clip) goto done;
                line_w = get_line_width(&text[i], x_clip, 0);
                start_x = get_aligned_start_x(x, line_w, alignment);
                gfx_SetTextXY(start_x, next_y);
            }
        }

        if ((gfx_GetTextX() + gfx_GetCharWidth(c)) <= x_clip) {
            gfx_PrintChar(c);
        }
    }

done:
    reset_clip();
    return TIGUI_E_SUCCESS;
}

tigui_error_t tigui_draw_text(tigui_text_t* text) {
    if (text == NULL) return TIGUI_E_BAD_PARAM;
    if (text->text == NULL) return TIGUI_E_BAD_PARAM;
    if (text->update == false) return TIGUI_E_SUCCESS;

    tigui_error_t err = draw_text(text->text, text->x, text->y, text->x_clip, text->y_clip, text->color, text->wrap, text->reference_point); if (err != TIGUI_E_SUCCESS) return err;

    text->update = false;
    return TIGUI_E_SUCCESS;
}

tigui_error_t tigui_draw_text_box(tigui_text_box_t* text_box) {
    if (text_box == NULL) return TIGUI_E_BAD_PARAM;
    if (text_box->update == false) return TIGUI_E_SUCCESS;

    uint16_t draw_x = text_box->x - (text_box->w / 2);
    uint16_t draw_y = text_box->y - (text_box->h / 2);
    tigui_error_t err;

    /* Draw background */
    err = draw_rectangle(draw_x, draw_y, text_box->w, text_box->h, text_box->bg_color, true); if (err != TIGUI_E_SUCCESS) return err;

    /* Draw border */
    err = draw_rectangle(draw_x, draw_y, text_box->w, text_box->h, text_box->selected ? text_box->selected_bdr_color : text_box->bdr_color, false); if (err != TIGUI_E_SUCCESS) return err;

    /* Draw text */
    if (text_box->text != NULL) {
        err = draw_text(text_box->text, draw_x + text_box->padding, draw_y + text_box->padding, draw_x + text_box->w - text_box->padding, draw_y + text_box->h - text_box->padding, text_box->txt_fg_color, text_box->wrap, TIGUI_TEXT_LEFT_POINT); if (err != TIGUI_E_SUCCESS) return err;
    }

    text_box->update = false;
    return TIGUI_E_SUCCESS;
}

tigui_error_t tigui_draw_button(tigui_button_t* button) {
    if (button == NULL) return TIGUI_E_BAD_PARAM;
    if (button->update == false) return TIGUI_E_SUCCESS;

    uint16_t draw_x = button->x - (button->w / 2);
    uint16_t draw_y = button->y - (button->h / 2);
    tigui_error_t err;

    /* Draw background */
    err = draw_rectangle(draw_x, draw_y, button->w, button->h, button->bg_color, true); if (err != TIGUI_E_SUCCESS) return err;

    /* Draw label */
    if (button->label != NULL) {
        size_t str_w = gfx_GetStringWidth(button->label);
        uint16_t text_x = draw_x + (button->w - str_w) / 2;
        uint16_t text_y = draw_y + (button->h - GRAPHX_TEXT_HEIGHT) / 2;
        err = draw_text(button->label, text_x, text_y, draw_x + button->w, draw_y + button->h, button->txt_fg_color, false, TIGUI_TEXT_LEFT_POINT); if (err != TIGUI_E_SUCCESS) return err;
    }

    /* Draw border */
    err = draw_rectangle(draw_x, draw_y, button->w, button->h, button->selected ? button->selected_bdr_color : button->bdr_color, false); if (err != TIGUI_E_SUCCESS) return err;

    button->update = false;
    return TIGUI_E_SUCCESS;
}

tigui_error_t tigui_draw_checkbox(tigui_checkbox_t* checkbox) {
    if (checkbox == NULL) return TIGUI_E_BAD_PARAM;
    if (checkbox->update == false) return TIGUI_E_SUCCESS;

    uint16_t draw_x = checkbox->x - (checkbox->size / 2);
    uint16_t draw_y = checkbox->y - (checkbox->size / 2);
    tigui_error_t err;

    /* Draw background */
    err = draw_rectangle(draw_x, draw_y, checkbox->size, checkbox->size, checkbox->checked ? checkbox->checked_bg_color : checkbox->unchecked_bg_color, true); if (err != TIGUI_E_SUCCESS) return err;

    /* Draw border */
    err = draw_rectangle(draw_x, draw_y, checkbox->size, checkbox->size, checkbox->selected ? checkbox->selected_bdr_color : checkbox->bdr_color, false); if (err != TIGUI_E_SUCCESS) return err;

    /* Draw checkmark */
    if (checkbox->checked == true) {
        err = draw_line(draw_x + checkbox->size / 5,
                        draw_y + checkbox->size / 2,
                        draw_x + checkbox->size * 2 / 5,
                        draw_y + checkbox->size * 3 / 4,
                        checkbox->check_color);
        if (err != TIGUI_E_SUCCESS) return err;

        err = draw_line(draw_x + checkbox->size * 2 / 5,
                        draw_y + checkbox->size * 3 / 4,
                        draw_x + checkbox->size * 4 / 5,
                        draw_y + checkbox->size / 4,
                        checkbox->check_color);
        if (err != TIGUI_E_SUCCESS) return err;
    }

    /* Draw label */
    if (checkbox->label != NULL) {
        uint16_t text_x = draw_x + checkbox->size + checkbox->left_label_padding;
        uint16_t text_y = draw_y + (checkbox->size - GRAPHX_TEXT_HEIGHT) / 2;
        err = draw_text(checkbox->label, text_x, text_y, checkbox->x_clip, draw_y + checkbox->size, checkbox->txt_fg_color, false, TIGUI_TEXT_LEFT_POINT); if (err != TIGUI_E_SUCCESS) return err;
    }

    checkbox->update = false;
    return TIGUI_E_SUCCESS;
}

tigui_error_t tigui_draw_radio_button(tigui_radio_button_t* radio_button) {
    if (radio_button == NULL) return TIGUI_E_BAD_PARAM;
    if (radio_button->update == false) return TIGUI_E_SUCCESS;

    tigui_error_t err;

    /* Draw background */
    err = draw_circle(radio_button->x, radio_button->y, radio_button->radius, radio_button->filled ? radio_button->filled_bg_color : radio_button->unfilled_bg_color, true); if (err != TIGUI_E_SUCCESS) return err;

    /* Draw border */
    err = draw_circle(radio_button->x, radio_button->y, radio_button->radius, radio_button->selected ? radio_button->selected_bdr_color : radio_button->bdr_color, false); if (err != TIGUI_E_SUCCESS) return err;

    /* Draw dot */
    if (radio_button->filled == true) {
        err = draw_circle(radio_button->x, radio_button->y, radio_button->radius - radio_button->dot_radius_reduction_factor, radio_button->dot_color, true); if (err != TIGUI_E_SUCCESS) return err;
    }

    /* Draw label */
    if (radio_button->label != NULL) {
        uint16_t text_x = radio_button->x + radio_button->radius + radio_button->left_label_padding;
        uint16_t text_y = radio_button->y - (GRAPHX_TEXT_HEIGHT / 2);
        err = draw_text(radio_button->label, text_x, text_y, radio_button->x_clip, radio_button->y + radio_button->radius, radio_button->txt_fg_color, false, TIGUI_TEXT_LEFT_POINT); if (err != TIGUI_E_SUCCESS) return err;
    }

    radio_button->update = false;
    return TIGUI_E_SUCCESS;
}

tigui_error_t tigui_draw_toggle(tigui_toggle_t* toggle) {
    if (toggle == NULL) return TIGUI_E_BAD_PARAM;
    if (toggle->update == false) return TIGUI_E_SUCCESS;

    uint16_t pill_x = toggle->x - (TOGGLE_RADIUS + TOGGLE_LENGTH / 2);
    uint16_t pill_y = toggle->y - TOGGLE_RADIUS;
    tigui_error_t err;

    /* Draw border */
    tigui_color_t bdr = toggle->selected == true ? toggle->selected_bdr_color : toggle->bdr_color;
    err = draw_rectangle(pill_x + TOGGLE_RADIUS, pill_y - 1, TOGGLE_LENGTH, TOGGLE_RADIUS * 2 + 3, bdr, true); if (err != TIGUI_E_SUCCESS) return err;

    err = draw_circle(pill_x + TOGGLE_RADIUS, toggle->y, TOGGLE_RADIUS + 1, bdr, true); if (err != TIGUI_E_SUCCESS) return err;

    err = draw_circle(pill_x + TOGGLE_RADIUS + TOGGLE_LENGTH, toggle->y, TOGGLE_RADIUS + 1, bdr, true); if (err != TIGUI_E_SUCCESS) return err;

    /* Draw pill */
    tigui_color_t pill_color = toggle->on == true ? toggle->on_color : toggle->off_color;
    err = draw_rectangle(pill_x + TOGGLE_RADIUS, pill_y, TOGGLE_LENGTH, TOGGLE_RADIUS * 2 + 1, pill_color, true); if (err != TIGUI_E_SUCCESS) return err;

    err = draw_circle(pill_x + TOGGLE_RADIUS, toggle->y, TOGGLE_RADIUS, pill_color, true); if (err != TIGUI_E_SUCCESS) return err;

    err = draw_circle(pill_x + TOGGLE_RADIUS + TOGGLE_LENGTH, toggle->y, TOGGLE_RADIUS, pill_color, true); if (err != TIGUI_E_SUCCESS) return err;

    /* Draw knob */
    uint16_t knob_x = toggle->on == true ? pill_x + TOGGLE_RADIUS + TOGGLE_LENGTH : pill_x + TOGGLE_RADIUS;
    err = draw_circle(knob_x, toggle->y, TOGGLE_RADIUS - 1, toggle->knob_color, true); if (err != TIGUI_E_SUCCESS) return err;

    /* Draw label */
    if (toggle->label != NULL) {
        uint16_t text_x = pill_x - toggle->right_label_padding - gfx_GetStringWidth(toggle->label);
        uint16_t text_y = toggle->y - (GRAPHX_TEXT_HEIGHT / 2);
        err = draw_text(toggle->label, text_x, text_y, pill_x - toggle->right_label_padding, toggle->y + TOGGLE_RADIUS, toggle->txt_fg_color, false, TIGUI_TEXT_LEFT_POINT); if (err != TIGUI_E_SUCCESS) return err;
    }

    toggle->update = false;
    return TIGUI_E_SUCCESS;
}

tigui_error_t tigui_draw_progress_bar(tigui_progress_bar_t* progress_bar) {
    if (progress_bar == NULL) return TIGUI_E_BAD_PARAM;
    if (progress_bar->update == false) return TIGUI_E_SUCCESS;

    /* Clamp progress */
    if (progress_bar->progress < 0.0f) progress_bar->progress = 0.0f;
    if (progress_bar->progress > 1.0f) progress_bar->progress = 1.0f;

    /* Draw bar */
    uint16_t draw_x = progress_bar->x - (progress_bar->w / 2);
    uint16_t draw_y = progress_bar->y - (progress_bar->h / 2);
    uint16_t fill_w = (uint16_t)(progress_bar->progress * progress_bar->w);
    tigui_error_t err;

    err = draw_rectangle(draw_x, draw_y, progress_bar->w, progress_bar->h, progress_bar->bg_color, true); if (err != TIGUI_E_SUCCESS) return err;

    if (fill_w > 0) err = draw_rectangle(draw_x, draw_y, fill_w, progress_bar->h, progress_bar->progressed_color, true); if (err != TIGUI_E_SUCCESS) return err;

    if (progress_bar->selected == true) {
        err = draw_rectangle(draw_x, draw_y, progress_bar->w, progress_bar->h, progress_bar->selected_bdr_color, false); if (err != TIGUI_E_SUCCESS) return err;
    } else {
        err = draw_rectangle(draw_x, draw_y, progress_bar->w, progress_bar->h, progress_bar->bdr_color, false); if (err != TIGUI_E_SUCCESS) return err;
    }

    /* Draw labels */
    char percent_disp_buf[7];
    sprintf(percent_disp_buf, "%.1f%%", progress_bar->progress * 100.0f);
    size_t str_w = gfx_GetStringWidth(percent_disp_buf);
    uint16_t percent_text_x = draw_x + (progress_bar->w - str_w) / 2;
    uint16_t percent_text_y = draw_y + (progress_bar->h - GRAPHX_TEXT_HEIGHT) / 2;
    err = draw_text(percent_disp_buf, percent_text_x, percent_text_y, draw_x + progress_bar->w, draw_y + progress_bar->h, progress_bar->txt_fg_color, false, TIGUI_TEXT_LEFT_POINT); if (err != TIGUI_E_SUCCESS) return err;

    if (progress_bar->label != NULL) {
        bool bail = false;
        size_t label_w = gfx_GetStringWidth(progress_bar->label);
        uint16_t label_text_x, label_text_y, label_x_clip;

        switch (progress_bar->label_pos) {
            case TIGUI_LABEL_POS_TOP:
                label_text_x = draw_x + (progress_bar->w - label_w) / 2;
                label_text_y = draw_y - progress_bar->label_padding - GRAPHX_TEXT_HEIGHT;
                label_x_clip = draw_x + progress_bar->w;
                break;
            case TIGUI_LABEL_POS_BOTTOM:
                label_text_x = draw_x + (progress_bar->w - label_w) / 2;
                label_text_y = draw_y + progress_bar->h + progress_bar->label_padding;
                label_x_clip = draw_x + progress_bar->w;
                break;
            case TIGUI_LABEL_POS_LEFT:
                label_text_x = draw_x - progress_bar->label_padding - label_w;
                label_text_y = draw_y + (progress_bar->h - GRAPHX_TEXT_HEIGHT) / 2;
                label_x_clip = draw_x - progress_bar->label_padding;
                break;
            case TIGUI_LABEL_POS_RIGHT:
                label_text_x = draw_x + progress_bar->w + progress_bar->label_padding;
                label_text_y = draw_y + (progress_bar->h - GRAPHX_TEXT_HEIGHT) / 2;
                label_x_clip = draw_x + progress_bar->w + progress_bar->label_padding + label_w;
                break;
            default:
                bail = true;
                break;
        }

        if (bail == true) goto done;

        err = draw_text(progress_bar->label, label_text_x, label_text_y, label_x_clip, label_text_y + GRAPHX_TEXT_HEIGHT, progress_bar->txt_fg_color, false, TIGUI_TEXT_LEFT_POINT); if (err != TIGUI_E_SUCCESS) return err;
    }
    goto done;

done:
    progress_bar->update = false;
    return TIGUI_E_SUCCESS;
}

tigui_error_t tigui_draw_spinner(tigui_spinner_t* spinner) {
    if (spinner == NULL) return TIGUI_E_BAD_PARAM;
    if (spinner->update == false) return TIGUI_E_SUCCESS;

    tigui_error_t err;

    /* Draw dots */
    for (uint8_t i = 0; i < spinner->num_dots; i++) {
        float angle = (2.0f * M_PI * i) / spinner->num_dots;
        uint16_t dot_x = (uint16_t)(spinner->x + spinner->radius * cosf(angle));
        uint16_t dot_y = (uint16_t)(spinner->y + spinner->radius * sinf(angle));

        tigui_color_t color = (i == spinner->current_frame) ? spinner->active_color : spinner->inactive_color;
        err = draw_circle(dot_x, dot_y, spinner->dot_radius, color, true);
        if (err != TIGUI_E_SUCCESS) return err;
    }

    /* Draw label */
    if (spinner->label != NULL) {
        unsigned int str_w = gfx_GetStringWidth(spinner->label);
        uint16_t spinner_right = spinner->x + spinner->radius + spinner->dot_radius;
        uint16_t spinner_left  = spinner->x - spinner->radius - spinner->dot_radius;
        uint16_t text_x = spinner_left + ((spinner_right - spinner_left) - str_w) / 2;
        uint16_t text_y = spinner->y - (GRAPHX_TEXT_HEIGHT / 2);

        err = draw_text(spinner->label, text_x, text_y, spinner_right, spinner->y + (GRAPHX_TEXT_HEIGHT / 2), spinner->txt_fg_color, false, TIGUI_TEXT_LEFT_POINT);
    }

    spinner->update = false;
    return TIGUI_E_SUCCESS;
}

tigui_error_t tigui_draw_slider(tigui_slider_t* slider) {
    if (slider == NULL) return TIGUI_E_BAD_PARAM;
    if (slider->update == false) return TIGUI_E_SUCCESS;

    /* Clamp slid */
    if (slider->slid < 0.0f) slider->slid = 0.0f;
    if (slider->slid > 1.0f) slider->slid = 1.0f;

    /* Draw bar */
    uint16_t draw_x = slider->x - (slider->w / 2);
    uint16_t draw_y = slider->y - (slider->h / 2);
    uint16_t fill_w = (uint16_t)(slider->slid * slider->w);
    tigui_error_t err;

    err = draw_rectangle(draw_x, draw_y, slider->w, slider->h, slider->bg_color, true); if (err != TIGUI_E_SUCCESS) return err;

    if (fill_w > 0) err = draw_rectangle(draw_x, draw_y, fill_w, slider->h, slider->slid_color, true); if (err != TIGUI_E_SUCCESS) return err;

    if (slider->selected == true) {
        err = draw_rectangle(draw_x, draw_y, slider->w, slider->h, slider->selected_bdr_color, false); if (err != TIGUI_E_SUCCESS) return err;
    } else {
        err = draw_rectangle(draw_x, draw_y, slider->w, slider->h, slider->bdr_color, false); if (err != TIGUI_E_SUCCESS) return err;
    }

    /* Draw knob */
    uint16_t knob_x = draw_x + fill_w;
    uint16_t knob_y = slider->y;
    uint16_t knob_radius = slider->h / 2 + 2;
    err = draw_circle(knob_x, knob_y, knob_radius, slider->selected == true ? slider->selected_bdr_color : slider->bdr_color, true); if (err != TIGUI_E_SUCCESS) return err;

    /* Draw label */
    if (slider->label != NULL) {
        bool bail = false;
        size_t label_w = gfx_GetStringWidth(slider->label);
        uint16_t label_text_x, label_text_y, label_x_clip;

        switch (slider->label_pos) {
            case TIGUI_LABEL_POS_TOP:
                label_text_x = draw_x + (slider->w - label_w) / 2;
                label_text_y = draw_y - slider->label_padding - GRAPHX_TEXT_HEIGHT;
                label_x_clip = draw_x + slider->w;
                break;
            case TIGUI_LABEL_POS_BOTTOM:
                label_text_x = draw_x + (slider->w - label_w) / 2;
                label_text_y = draw_y + slider->h + slider->label_padding;
                label_x_clip = draw_x + slider->w;
                break;
            case TIGUI_LABEL_POS_LEFT:
                label_text_x = draw_x - label_w - knob_radius - slider->label_padding;
                label_text_y = draw_y + (slider->h - GRAPHX_TEXT_HEIGHT) / 2;
                label_x_clip = draw_x - slider->label_padding;
                break;
            case TIGUI_LABEL_POS_RIGHT:
                label_text_x = draw_x + slider->w + knob_radius + slider->label_padding;
                label_text_y = draw_y + (slider->h - GRAPHX_TEXT_HEIGHT) / 2;
                label_x_clip = label_text_x + label_w;
                break;
            default:
                bail = true;
                break;
        }

        if (bail == true) goto done;

        err = draw_text(slider->label, label_text_x, label_text_y, label_x_clip, label_text_y + GRAPHX_TEXT_HEIGHT, slider->txt_fg_color, false, TIGUI_TEXT_LEFT_POINT); if (err != TIGUI_E_SUCCESS) return err;
    }

done:
    slider->update = false;
    return TIGUI_E_SUCCESS;
}

tigui_error_t tigui_draw_badge(tigui_badge_t* badge) {
    if (badge == NULL) return TIGUI_E_BAD_PARAM;
    if (badge->update == false) return TIGUI_E_SUCCESS;

    tigui_error_t err;

    /* Draw fill */
    err = draw_circle(badge->x, badge->y, badge->radius, badge->filled_color, true);
    if (err != TIGUI_E_SUCCESS) return err;

    /* Draw border */
    err = draw_circle(badge->x, badge->y, badge->radius, badge->selected == true ? badge->selected_bdr_color : badge->bdr_color, false);
    if (err != TIGUI_E_SUCCESS) return err;

    /* Draw value */
    char val_buf[9];
    sprintf(val_buf, "%u", badge->value);
    unsigned int str_w = gfx_GetStringWidth(val_buf);
    uint16_t text_x = badge->x - (str_w / 2);
    uint16_t text_y = badge->y - (GRAPHX_TEXT_HEIGHT / 2);
    err = draw_text(val_buf, text_x, text_y, badge->x + (str_w / 2), badge->y + (GRAPHX_TEXT_HEIGHT / 2), badge->txt_fg_color, false, TIGUI_TEXT_LEFT_POINT);
    if (err != TIGUI_E_SUCCESS) return err;

    badge->update = false;
    return TIGUI_E_SUCCESS;
}

tigui_keymap_entry_t tigui_default_lowercase_ascii_input_descriptor_entries[] = {
    {sk_Math,   'a'},
    {sk_Apps,   'b'},
    {sk_Prgm,   'c'},
    {sk_Recip,  'd'},
    {sk_Sin,    'e'},
    {sk_Cos,    'f'},
    {sk_Tan,    'g'},
    {sk_Power,  'h'},
    {sk_Square, 'i'},
    {sk_Comma,  'j'},
    {sk_LParen, 'k'},
    {sk_RParen, 'l'},
    {sk_Div,    'm'},
    {sk_Log,    'n'},
    {sk_7,      'o'},
    {sk_8,      'p'},
    {sk_9,      'q'},
    {sk_Mul,    'r'},
    {sk_Ln,     's'},
    {sk_4,      't'},
    {sk_5,      'u'},
    {sk_6,      'v'},
    {sk_Sub,    'w'},
    {sk_Store,  'x'},
    {sk_1,      'y'},
    {sk_2,      'z'},
    {sk_Add,    '"'},
    {sk_0,      ' '},
    {sk_DecPnt, ':'},
    {sk_Chs,    '?'},
    tigui_end_keymap_entries()
};
tigui_input_field_descriptor_t tigui_default_lowercase_ascii_input_descriptor = {
    .descriptor = 'a',
    .entries = tigui_default_lowercase_ascii_input_descriptor_entries
};
tigui_keymap_entry_t tigui_default_uppercase_ascii_input_descriptor_entries[] = {
    {sk_Math,   'A'},
    {sk_Apps,   'B'},
    {sk_Prgm,   'C'},
    {sk_Recip,  'D'},
    {sk_Sin,    'E'},
    {sk_Cos,    'F'},
    {sk_Tan,    'G'},
    {sk_Power,  'H'},
    {sk_Square, 'I'},
    {sk_Comma,  'J'},
    {sk_LParen, 'K'},
    {sk_RParen, 'L'},
    {sk_Div,    'M'},
    {sk_Log,    'N'},
    {sk_7,      'O'},
    {sk_8,      'P'},
    {sk_9,      'Q'},
    {sk_Mul,    'R'},
    {sk_Ln,     'S'},
    {sk_4,      'T'},
    {sk_5,      'U'},
    {sk_6,      'V'},
    {sk_Sub,    'W'},
    {sk_Store,  'X'},
    {sk_1,      'Y'},
    {sk_2,      'Z'},
    {sk_Add,    '"'},
    {sk_0,      ' '},
    {sk_DecPnt, ':'},
    {sk_Chs,    '?'},
    tigui_end_keymap_entries()
};
tigui_input_field_descriptor_t tigui_default_uppercase_ascii_input_descriptor = {
    .descriptor = 'A',
    .entries = tigui_default_uppercase_ascii_input_descriptor_entries
};
tigui_keymap_entry_t tigui_default_numeric_input_descriptor_entries[] = {
    {sk_Power,  '^'},
    {sk_Comma,  ','},
    {sk_LParen, '('},
    {sk_RParen, ')'},
    {sk_Div,    '/'},
    {sk_7,      '7'},
    {sk_8,      '8'},
    {sk_9,      '9'},
    {sk_Mul,    '*'},
    {sk_4,      '4'},
    {sk_5,      '5'},
    {sk_6,      '6'},
    {sk_Sub,    '-'},
    {sk_1,      '1'},
    {sk_2,      '2'},
    {sk_3,      '3'},
    {sk_Add,    '+'},
    {sk_0,      '0'},
    {sk_DecPnt, '.'},
    {sk_Chs,    '_'},
    tigui_end_keymap_entries()
};
tigui_input_field_descriptor_t tigui_default_numeric_input_descriptor = {
    .descriptor = '#',
    .entries = tigui_default_numeric_input_descriptor_entries
};

tigui_input_field_descriptor_t* tigui_default_input_field_descriptors[] = {
    &tigui_default_lowercase_ascii_input_descriptor,
    &tigui_default_uppercase_ascii_input_descriptor,
    &tigui_default_numeric_input_descriptor
};

tigui_error_t tigui_draw_input_field(tigui_input_field_t* input_field) {
    if (input_field == NULL || input_field->input_buf == NULL || input_field->input_buf_size <= 1) return TIGUI_E_BAD_PARAM;
    if (input_field->update == false) return TIGUI_E_SUCCESS;

    uint16_t draw_x = input_field->x - (input_field->w / 2);
    uint16_t draw_y = input_field->y - (input_field->h / 2);
    tigui_error_t err;

    /* Draw background */
    err = draw_rectangle(draw_x, draw_y, input_field->w, input_field->h, input_field->bg_color, true); if (err != TIGUI_E_SUCCESS) return err;

    /* Draw border */
    err = draw_rectangle(draw_x, draw_y, input_field->w, input_field->h, input_field->selected == true ? input_field->selected_bdr_color : input_field->bdr_color, false); if (err != TIGUI_E_SUCCESS) return err;

    /* Draw descriptor box */
    uint16_t descriptor_w = gfx_GetCharWidth(input_field->descriptors[input_field->current_descriptor_i]->descriptor) + 4;
    err = draw_rectangle(draw_x, draw_y, descriptor_w, input_field->h, input_field->type_descriptor_bg_color, true); if (err != TIGUI_E_SUCCESS) return err;

    char desc_str[2] = { input_field->descriptors[input_field->current_descriptor_i]->descriptor, '\0' };
    uint16_t desc_text_x = draw_x + (descriptor_w - gfx_GetCharWidth(input_field->descriptors[input_field->current_descriptor_i]->descriptor)) / 2;
    uint16_t desc_text_y = draw_y + (input_field->h - GRAPHX_TEXT_HEIGHT) / 2;
    err = draw_text(desc_str, desc_text_x, desc_text_y, draw_x + descriptor_w, draw_y + input_field->h, input_field->type_descriptor_txt_fg_color, false, TIGUI_TEXT_LEFT_POINT); if (err != TIGUI_E_SUCCESS) return err;

    /* Calculate text area */
    uint16_t text_area_x = draw_x + descriptor_w + 2;
    uint16_t text_area_w = draw_x + input_field->w - text_area_x;
    uint16_t text_y = draw_y + (input_field->h - GRAPHX_TEXT_HEIGHT) / 2;
    bool buf_empty = (input_field->input_buf[0] == '\0');

    if (buf_empty == true) {
        /* Draw placeholder */
        if (input_field->placeholder != NULL) {
            err = draw_text(input_field->placeholder, text_area_x, text_y, draw_x + input_field->w, draw_y + input_field->h, input_field->placeholder_txt_fg_color, false, TIGUI_TEXT_LEFT_POINT); if (err != TIGUI_E_SUCCESS) return err;
        }

        /* Draw cursor at start */
        if (input_field->draw_cursor == true) {
            err = draw_line(text_area_x, draw_y + 2, text_area_x, draw_y + input_field->h - 2, input_field->txt_fg_color); if (err != TIGUI_E_SUCCESS) return err;
        }
    } else {
        /* Calculate draw offset so cursor stays in view */
        size_t draw_offset = 0;
        while (draw_offset < input_field->input_buf_offset) {
            char temp[input_field->input_buf_size];
            size_t len = input_field->input_buf_offset - draw_offset;
            memcpy(temp, &input_field->input_buf[draw_offset], len);
            temp[len] = '\0';
            if (gfx_GetStringWidth(temp) <= text_area_w) break;
            draw_offset++;
        }

        /* Draw visible portion of input buffer */
        err = draw_text(&input_field->input_buf[draw_offset], text_area_x, text_y, draw_x + input_field->w, draw_y + input_field->h, input_field->txt_fg_color, false, TIGUI_TEXT_LEFT_POINT); if (err != TIGUI_E_SUCCESS) return err;

        /* Draw cursor */
        if (input_field->draw_cursor == true) {
            char before_cursor[input_field->input_buf_size];
            size_t visible_len = input_field->input_buf_offset - draw_offset;
            memcpy(before_cursor, &input_field->input_buf[draw_offset], visible_len);
            before_cursor[visible_len] = '\0';
            uint16_t cursor_x = text_area_x + gfx_GetStringWidth(before_cursor);
            if (cursor_x <= draw_x + input_field->w) {
                err = draw_line(cursor_x, draw_y + 2, cursor_x, draw_y + input_field->h - 2, input_field->txt_fg_color); if (err != TIGUI_E_SUCCESS) return err;
            }
        }
    }

    /* Draw label */
    if (input_field->label != NULL) {
        size_t label_w = gfx_GetStringWidth(input_field->label);
        uint16_t label_text_x, label_text_y, label_x_clip;

        switch (input_field->label_pos) {
            case TIGUI_LABEL_POS_TOP:
                label_text_x = draw_x + (input_field->w - label_w) / 2;
                label_text_y = draw_y - input_field->label_padding - GRAPHX_TEXT_HEIGHT;
                label_x_clip = draw_x + input_field->w;
                break;
            case TIGUI_LABEL_POS_BOTTOM:
                label_text_x = draw_x + (input_field->w - label_w) / 2;
                label_text_y = draw_y + input_field->h + input_field->label_padding;
                label_x_clip = draw_x + input_field->w;
                break;
            case TIGUI_LABEL_POS_LEFT:
                label_text_x = draw_x - input_field->label_padding - label_w;
                label_text_y = draw_y + (input_field->h - GRAPHX_TEXT_HEIGHT) / 2;
                label_x_clip = draw_x - input_field->label_padding;
                break;
            case TIGUI_LABEL_POS_RIGHT:
                label_text_x = draw_x + input_field->w + input_field->label_padding;
                label_text_y = draw_y + (input_field->h - GRAPHX_TEXT_HEIGHT) / 2;
                label_x_clip = draw_x + input_field->w + input_field->label_padding + label_w;
                break;
            default:
                goto done;
        }

        err = draw_text(input_field->label, label_text_x, label_text_y, label_x_clip, label_text_y + GRAPHX_TEXT_HEIGHT, input_field->txt_fg_color, false, TIGUI_TEXT_LEFT_POINT); if (err != TIGUI_E_SUCCESS) return err;
    }

done:
    input_field->update = false;
    return TIGUI_E_SUCCESS;
}

tigui_error_t tigui_draw_alert(tigui_alert_t* alert) {
    if (alert == NULL) return TIGUI_E_BAD_PARAM;
    if (alert->update == false || alert->is_showing == false) return TIGUI_E_SUCCESS;

    uint16_t total_h = alert->h + alert->button_h;
    uint16_t draw_x = alert->x - (alert->w / 2);
    uint16_t draw_y = alert->y - (total_h / 2);
    tigui_error_t err;

    /* Draw title/body container */
    err = draw_rectangle(draw_x, draw_y, alert->w, alert->h, alert->bg_color, true);
    if (err != TIGUI_E_SUCCESS) return err;

    err = draw_rectangle(draw_x, draw_y, alert->w, alert->h, alert->bdr_color, false);
    if (err != TIGUI_E_SUCCESS) return err;

    /* Draw title */
    if (alert->title != NULL) {
        uint16_t title_y = draw_y + alert->label_padding;
        err = draw_text(alert->title, alert->x, title_y, draw_x + alert->w, title_y + GRAPHX_TEXT_HEIGHT, alert->info_txt_fg_color, false, TIGUI_TEXT_CENTER_POINT); if (err != TIGUI_E_SUCCESS) return err;
    }

    /* Draw body */
    if (alert->body != NULL) {
        uint16_t body_y = draw_y + alert->label_padding + GRAPHX_TEXT_HEIGHT + alert->label_padding;
        err = draw_text(alert->body, draw_x + alert->label_padding, body_y, draw_x + alert->w - alert->label_padding, draw_y + alert->h - alert->label_padding, alert->info_txt_fg_color, true, TIGUI_TEXT_LEFT_POINT); if (err != TIGUI_E_SUCCESS) return err;
    }

    /* Draw buttons */
    uint16_t button_y = draw_y + alert->h;
    if (alert->fail_button_label == NULL) {
        /* Single pass button takes full width */
        uint16_t pass_selected = (alert->selection_i == 0);
        err = draw_rectangle(draw_x, button_y, alert->w, alert->button_h, alert->bg_color, true);
        if (err != TIGUI_E_SUCCESS) return err;
        err = draw_rectangle(draw_x, button_y, alert->w, alert->button_h, pass_selected ? alert->selected_bdr_color : alert->bdr_color, false); if (err != TIGUI_E_SUCCESS) return err;
        if (alert->pass_button_label != NULL) {
            uint16_t btn_center_x = draw_x + (alert->w / 2);
            uint16_t btn_text_y = button_y + (alert->button_h - GRAPHX_TEXT_HEIGHT) / 2;
            err = draw_text(alert->pass_button_label, btn_center_x, btn_text_y, draw_x + alert->w, btn_text_y + GRAPHX_TEXT_HEIGHT, alert->pass_button_txt_fg_color, false, TIGUI_TEXT_CENTER_POINT); if (err != TIGUI_E_SUCCESS) return err;
        }
    } else {
        /* Two buttons, each half width */
        uint16_t half_w = alert->w / 2;

        /* Fail button on left */
        uint16_t fail_selected = (alert->selection_i == 0);
        err = draw_rectangle(draw_x, button_y, half_w, alert->button_h, alert->bg_color, true);
        if (err != TIGUI_E_SUCCESS) return err;
        err = draw_rectangle(draw_x, button_y, half_w, alert->button_h, fail_selected ? alert->selected_bdr_color : alert->bdr_color, false); if (err != TIGUI_E_SUCCESS) return err;
        uint16_t fail_center_x = draw_x + (half_w / 2);
        uint16_t btn_text_y = button_y + (alert->button_h - GRAPHX_TEXT_HEIGHT) / 2;
        err = draw_text(alert->fail_button_label, fail_center_x, btn_text_y, draw_x + half_w, btn_text_y + GRAPHX_TEXT_HEIGHT, alert->fail_button_txt_fg_color, false, TIGUI_TEXT_CENTER_POINT); if (err != TIGUI_E_SUCCESS) return err;

        /* Pass button on right */
        uint16_t pass_selected = (alert->selection_i == 1);
        err = draw_rectangle(draw_x + half_w, button_y, half_w, alert->button_h, alert->bg_color, true);
        if (err != TIGUI_E_SUCCESS) return err;
        err = draw_rectangle(draw_x + half_w, button_y, half_w, alert->button_h, pass_selected ? alert->selected_bdr_color : alert->bdr_color, false); if (err != TIGUI_E_SUCCESS) return err;
        uint16_t pass_center_x = draw_x + half_w + (half_w / 2);
        err = draw_text(alert->pass_button_label != NULL ? alert->pass_button_label : "", pass_center_x, btn_text_y, draw_x + alert->w, btn_text_y + GRAPHX_TEXT_HEIGHT, alert->pass_button_txt_fg_color, false, TIGUI_TEXT_CENTER_POINT); if (err != TIGUI_E_SUCCESS) return err;
    }

    alert->update = false;
    return TIGUI_E_SUCCESS;
}

tigui_error_t tigui_draw_list(tigui_list_t* list) {
    if (list == NULL || list->items == NULL) return TIGUI_E_BAD_PARAM;
    if (list->update == false) return TIGUI_E_SUCCESS;

    uint16_t draw_x = list->x - (list->w / 2);
    uint16_t draw_y = list->y - (list->h / 2);
    tigui_error_t err;

    /* Draw background */
    err = draw_rectangle(draw_x, draw_y, list->w, list->h, list->bg_color, true); if (err != TIGUI_E_SUCCESS) return err;

    /* Draw items */
    size_t visible_items = list->h / list->item_h;
    size_t end_i = list->scroll_offset_i + visible_items;
    if (end_i > list->num_items) end_i = list->num_items;

    for (size_t i = list->scroll_offset_i; i < end_i; i++) {
        uint16_t item_y = draw_y + (i - list->scroll_offset_i) * list->item_h;
        bool is_selected = (i == list->selected_i && list->is_browsing == true);

        /* Draw item background */
        tigui_color_t item_bg = is_selected ? list->selected_bdr_color : list->bg_color;
        err = draw_rectangle(draw_x, item_y, list->w, list->item_h, item_bg, true); if (err != TIGUI_E_SUCCESS) return err;

        /* Draw item text */
        if (list->items[i].text != NULL) {
            uint16_t text_y = item_y + (list->item_h - GRAPHX_TEXT_HEIGHT) / 2;
            uint16_t text_x;
            switch (list->item_txt_reference_point) {
                case TIGUI_TEXT_CENTER_POINT: text_x = list->x; break;
                case TIGIU_TEXT_RIGHT_POINT:  text_x = draw_x + list->w; break;
                default:                      text_x = draw_x + 2; break;
            }
            tigui_color_t txt_color = is_selected ? list->bg_color : list->selected_bdr_color;
            err = draw_text(list->items[i].text, text_x, text_y, draw_x + list->w, text_y + GRAPHX_TEXT_HEIGHT, txt_color, false, list->item_txt_reference_point); if (err != TIGUI_E_SUCCESS) return err;
        }
    }

    /* Draw border */
    tigui_color_t* bdr_color = NULL;
    if (list->selected == true && list->is_browsing == false) {
        bdr_color = &list->selected_bdr_color;
    } else {
        bdr_color = &list->bdr_color;
    }
    err = draw_rectangle(draw_x, draw_y, list->w, list->h, *bdr_color, false); if (err != TIGUI_E_SUCCESS) return err;

    /* Draw label */
    if (list->label != NULL) {
        size_t label_w = gfx_GetStringWidth(list->label);
        uint16_t label_text_x, label_text_y, label_x_clip;

        switch (list->label_pos) {
            case TIGUI_LABEL_POS_TOP:
                label_text_x = draw_x + (list->w - label_w) / 2;
                label_text_y = draw_y - GRAPHX_TEXT_HEIGHT - 2;
                label_x_clip = draw_x + list->w;
                break;
            case TIGUI_LABEL_POS_BOTTOM:
                label_text_x = draw_x + (list->w - label_w) / 2;
                label_text_y = draw_y + list->h + 2;
                label_x_clip = draw_x + list->w;
                break;
            case TIGUI_LABEL_POS_LEFT:
                label_text_x = draw_x - label_w - 2;
                label_text_y = draw_y + (list->h - GRAPHX_TEXT_HEIGHT) / 2;
                label_x_clip = draw_x - 2;
                break;
            case TIGUI_LABEL_POS_RIGHT:
                label_text_x = draw_x + list->w + 2;
                label_text_y = draw_y + (list->h - GRAPHX_TEXT_HEIGHT) / 2;
                label_x_clip = draw_x + list->w + 2 + label_w;
                break;
            default:
                goto done;
        }

        err = draw_text(list->label, label_text_x, label_text_y, label_x_clip, label_text_y + GRAPHX_TEXT_HEIGHT, list->bdr_color, false, TIGUI_TEXT_LEFT_POINT); if (err != TIGUI_E_SUCCESS) return err;
    }

done:
    list->update = false;
    return TIGUI_E_SUCCESS;
}