#ifndef TIGUI_H
#define TIGUI_H

#include <stdint.h>
#include <stdbool.h>
#include <ti/getcsc.h>

#define TIGUI_SCREEN_WIDTH  320
#define TIGUI_SCREEN_HEIGHT 240

/* Error codes, compatible with tigui_strerror(). */
typedef enum {
    TIGUI_E_SUCCESS                       = 0,  // No error.
    TIGUI_E_BAD_PARAM                     = -1, // Invalid pointer or other unexpected argument passed to a tigui function.
    TIGUI_E_COLORSPACE_FULL               = -2, // No more open palette slots.
    TIGUI_E_INVALID_NODESET               = -3, // NULL pointers are inside the nodeset.
    TIGUI_E_CANVAS_HAS_DUPLICATE_KEYS     = -4, // Duplicate keys set in the canvas.
    TIGUI_E_MULTIPLE_NODES_SELECTED       = -5, // Multiple nodes selected in the provided nodeset.
    TIGUI_E_ELEMENT_NOT_SELECTABLE        = -6, // Non-selectable element passed to the selection handler.
    TIGUI_E_UNKNOWN_ELEMENT               = -7, // Unknown element type passed to the internal drawer.
    TIGUI_E_USER_EXIT                     = -8, // User pressed the canvas's exit key (non-fatal).
    TIGUI_E_THIRDPARTY_ERROR              = -9, // Generic error.
    TIGUI_E_ORCHESTRATOR_ALERT_TRACK_FAIL = -10 // Failure to keep track of alerts. Core library error, should not happen.
} tigui_error_t;

/**
 * This is direct memory access to the live color palette.
 * 
 * You may parse through it, or even edit it if you wish! Be careful.
 * 
 * There are 256 entries, each an unsigned 16-bit int, for a total of 512 bytes of data.
 */
#define tigui_palette ((uint16_t*)0xE30200)

/* Color container used by various library elements. */
typedef struct {
    uint8_t r, g, b;
} tigui_color_t;
#define TIGUI_COLOR(r,g,b) ((tigui_color_t){(r),(g),(b)})

#define TIGUI_BLACK  TIGUI_COLOR(0,   0,   0)
#define TIGUI_RED    TIGUI_COLOR(255, 0,   0)
#define TIGUI_ORANGE TIGUI_COLOR(255, 127, 0)
#define TIGUI_YELLOW TIGUI_COLOR(255, 255, 0)
#define TIGUI_GREEN  TIGUI_COLOR(0,   255, 0)
#define TIGUI_BLUE   TIGUI_COLOR(0,   0,   255)
#define TIGUI_PURPLE TIGUI_COLOR(128, 0,   128)
#define TIGUI_WHITE  TIGUI_COLOR(255, 255, 255)
#define TIGUI_GRAY   TIGUI_COLOR(128, 128, 128)

#define tigui_element_init() {0} // Use this to initialize stuff!

/**
 * @brief Waits for any key to be pressed before continuing any execution.
 */
#define wait_for_keypress() while (!os_GetCSC())

/**
 * @brief Waits for a specific key to be pressed before continuing any execution.
 */
#define wait_for_specific_keypress(k) while (os_GetCSC() != k)

/**
 * This is the canvas.
 * 
 * You can think of the canvas like a slide on PowerPoint, or a ContentView in SwiftUI. It is the main container for all of the GUI elements
 * on that screen.
 * 
 * For every main interface, you will have one canvas. The nodes allow you to link individual UI elements together for navigation and interaction.
 */
typedef enum {
    TIGUI_ELEMENT_LINE,         // Default on_pressed action: No default action.
    TIGUI_ELEMENT_RECTANGLE,    // Default on_pressed action: No default action.
    TIGUI_ELEMENT_CIRCLE,       // Default on_pressed action: No default action.
    TIGUI_ELEMENT_ELLIPSE,      // Default on_pressed action: No default action.
    TIGUI_ELEMENT_TEXT,         // Default on_pressed action: No default action (not selectable).
    TIGUI_ELEMENT_TEXT_BOX,     // Default on_pressed action: No default action.
    TIGUI_ELEMENT_BUTTON,       // Default on_pressed action: No default action.
    TIGUI_ELEMENT_CHECKBOX,     // Default on_pressed action: .checked will toggle.
    TIGUI_ELEMENT_RADIO_BUTTON, // Default on_pressed action: .filled will be set to true, all other .filled of the same group ID will be set to false.
    TIGUI_ELEMENT_TOGGLE,       // Default on_pressed action: .on will toggle.
    TIGUI_ELEMENT_PROGRESS_BAR, // Default on_pressed action: No default action.
    TIGUI_ELEMENT_SPINNER,      // Default on_pressed action: No default action (not selectable).
    TIGUI_ELEMENT_SLIDER,       // Default on_pressed action: Key presses will be focused on sliding the slider left and right, updating its value as well.
    TIGUI_ELEMENT_BADGE,        // Default on_pressed action: No default action.
                                // If you DO have on_pressed set, the above action will run first, then your function pointer.
    TIGUI_ELEMENT_INPUT_FIELD,  // Default on_pressed action: The keypad will type into the input field and update the input buffer.
    TIGUI_ELEMENT_ALERT,        // Default on_pressed action: Alert disappears, will follow selection callbacks in struct.
    TIGUI_ELEMENT_LIST          // Default on_pressed action: Scrolls up and down in the list, firing item callbacks when hitting the canvas's enter key.
} tigui_element_type_t;

/**
 * @brief Callback template used by the canvas and nodes.
 * @param user_data Opaque pointer set inside of the canvas.
 * @return See comment inside of tigui_canvas_t's on_complete or tigui_canvas_node_t's on_pressed respectively.
 */
typedef bool (*tigui_interactivity_cb_t)(void* user_data);

typedef struct tigui_canvas_node_t {
    /* Element */
    void* element;                       // Pointer to the element struct, cannot be NULL.
    tigui_element_type_t type;           // Type of the element.

    // Placement/Navigation (up to all but one can be NULL, or else your selection cursor will become trapped).
    // Note about trapping only applies if the node is selectable (bool you see below).
    // 
    // If you're building a state-driven canvas (like a loading screen), none of the above applies to you, you may set all to NULL.
    // To exit the canvas, set is_running to false.
    struct tigui_canvas_node_t* up;      // Selectable node above this one.
    struct tigui_canvas_node_t* down;    // Selectable node below this one.
    struct tigui_canvas_node_t* left;    // Selectable node to the left of this one.
    struct tigui_canvas_node_t* right;   // Selectable node to the right of this one.
    
    /* Interaction */
    bool selectable;                     // Whether or not this node can be selected during navigation.
    tigui_interactivity_cb_t on_pressed; // Callback fired when the user presses this node, can be NULL. 
                                         // Return true to redraw the entire canvas (including background and all on-screen elements (regardless of the update flag).)
                                         // Return false just to redraw elements with their update flags set to true.
} tigui_canvas_node_t;

typedef struct {
    /* Appearance */
    tigui_color_t bg_color;                    // Color of the canvas's background.
    
    /* Management */
    bool is_running;                           // Whether or not to keep the orchestration loop running.
    tigui_canvas_node_t** nodeset;             // All nodes to draw to the canvas. Should look like: [&rectangle_node, &button1_node, &button2_node].
    size_t num_nodes;                          // Number of nodes in the nodeset.
    void* user_data;                           // Opaque pointer to be passed to the on_draw_complete() function.
    tigui_interactivity_cb_t on_draw_complete; // Callback fired every draw cycle, can be NULL. Return true to redraw all on-screen elements, or false to not redraw.
                                               // Use this as a free while loop which is synced with your GUI!
    sk_key_t exit_key;                         // Key that will trigger "back" or "exit."
    sk_key_t enter_key;                        // Key that will trigger "select" or "enter."
    sk_key_t delete_key;                       // Key that will trigger "delete" or "backspace."
    sk_key_t up_key;                           // Key to navigate up with.
    sk_key_t down_key;                         // Key to navigate down with.
    sk_key_t left_key;                         // Key to navigate left with.
    sk_key_t right_key;                        // Key to navigate right with.
} tigui_canvas_t;

/* Line */
typedef struct {
    /* Appearance */
    uint16_t x0, y0;              // Starting point of the line
    uint16_t x1, y1;              // Ending point of the line.
    tigui_color_t color;          // Color of the line.
    tigui_color_t selected_color; // Color of the line when selected.

    /* Management */
    bool selected;                // Whether or not to draw the line in the color specified by selected_color.
    bool update;                  // Whether or not to update the line's appearance on the next update. 
                                  // Checked and set to false by tigui_draw_line().
} tigui_line_t;

/* Rectangle */
typedef struct {
    /* Appearance */
    uint16_t x, y;                    // Coordinates to draw at, referenced from the center.
    uint16_t w, h;                    // Width and height.
    tigui_color_t filled_color;       // Color to fill the rectangle with.
    tigui_color_t bdr_color;          // Color of the border when not selected.
    tigui_color_t selected_bdr_color; // Color of the border when selected (should be different than bdr_color).

    /* Management */
    bool selected;                    // Whether or not to draw a border with the color specified by selected_bdr_color.
    bool filled;                      // Whether or not to draw the rectangle filled, color specified by filled_color.
    bool update;                      // Whether or not to update the rectangle's appearance on the next update. 
                                      // Checked and set to false by tigui_draw_rectangle().
} tigui_rectangle_t;

/* Circle */
typedef struct {
    /* Appearance */
    uint16_t x, y;                    // Coordinates to draw at, referenced from the center.
    uint16_t radius;                  // Radius.
    tigui_color_t filled_color;       // Color to fill the circle with.
    tigui_color_t bdr_color;          // Color of the border when not selected.
    tigui_color_t selected_bdr_color; // Color of the border when selected (should be different than bdr_color).

    /* Management */
    bool selected;                    // Whether or not to draw a border with the color specified by selected_bdr_color.
    bool filled;                      // Whether or not to draw the circle filled, color specified by filled_color.
    bool update;                      // Whether or not to update the circle's appearance on the next update. 
                                      // Checked and set to false by tigui_draw_circle().
} tigui_circle_t;

/* Ellipse */
typedef struct {
    /* Appearance */
    uint16_t x, y;                    // Coordinates to draw at, referenced from the center.
    uint16_t v_radius, h_radius;      // Vertical and horizontal radii.
    tigui_color_t filled_color;       // Color to fill the ellipse with.
    tigui_color_t bdr_color;          // Color of the border when not selected.
    tigui_color_t selected_bdr_color; // Color of the border when selected (should be different than bdr_color).

    /* Management */
    bool selected;                    // Whether or not to draw a border with the color specified by selected_bdr_color.
    bool filled;                      // Whether or not to draw the ellipse filled, color specified by filled_color.
    bool update;                      // Whether or not to update the button's appearance on the next update. 
                                      // Checked and set to false by tigui_draw_ellipse().
} tigui_ellipse_t;

/* Text */
typedef enum {
    TIGUI_TEXT_LEFT_POINT,
    TIGUI_TEXT_CENTER_POINT,
    TIGIU_TEXT_RIGHT_POINT
} tigui_text_reference_point_t;

typedef struct {
    /* Appearance */
    char* text;                                   // The text to display, can not be NULL.
    tigui_text_reference_point_t reference_point; // Point of the text that should lie at x.
    uint16_t x, y;                                // Coordinates to draw at, referenced from alignment.
    uint16_t x_clip, y_clip;                      // X and Y coordinates not to draw text past.
    tigui_color_t color;                          // Text color.

    /* Management */
    bool wrap;                                    // Whether or not to wrap text at x_clip.
    bool update;                                  // Whether or not to update the text's appearance on the next update.
                                                  // Checked and set to false by tigui_draw_text().
} tigui_text_t;

/* Text Box */
typedef struct {
    /* Appearance */
    char* text;                       // The text to display, can be NULL.
    uint16_t padding;                 // Padding between the border and the text.
    uint16_t x, y;                    // Coordinates to draw at, referenced from the center.
    uint16_t w, h;                    // Width and height of the box.
    tigui_color_t bg_color;           // Color of the box's background.
    tigui_color_t txt_fg_color;       // Text color.
    tigui_color_t bdr_color;          // Color of the border when not selected.
    tigui_color_t selected_bdr_color; // Color of the border when selected (should be different than bdr_color).

    /* Management */
    bool selected;                    // Whether or not to draw a border with the color specified by selected_bdr_color.
    bool wrap;                        // Whether or not to wrap text within the box.
    bool update;                      // Whether or not to update the text box's appearance on the next update.
                                      // Checked and set to false by tigui_draw_text_box().
} tigui_text_box_t;

/* Button */
typedef struct {
    /* Appearance */
    char* label;                      // Label of the button, can be NULL.
    uint16_t x, y;                    // Coordinates to draw at, referenced from the center.
    uint16_t w, h;                    // Width and height.
    tigui_color_t bg_color;           // Background color.
    tigui_color_t txt_fg_color;       // Text color.
    tigui_color_t bdr_color;          // Color of the border when not selected.
    tigui_color_t selected_bdr_color; // Color of the border when selected (should be different than bdr_color).
  
    /* Management */  
    bool selected;                    // Whether or not to draw a border with the color specified by selected_bdr_color.
    bool update;                      // Whether or not to update the button's appearance on the next update. 
                                      // Checked and set to false by tigui_draw_button().
} tigui_button_t;

/* Checkbox */
typedef struct {
    /* Appearance */
    char* label;                      // Label of the checkbox, can be NULL.
    uint16_t left_label_padding;      // Number of pixels in between the right border and beginning of the label.
    uint16_t x_clip;                  // X coordinate not to put text past.
    uint16_t x, y;                    // Coordinates to draw at, referenced from the center. Note that text will follow after going to the right.
    uint16_t size;                    // Length of the sides (square).
    tigui_color_t unchecked_bg_color; // Color to fill with when unchecked.
    tigui_color_t checked_bg_color;   // Color to fill with when checked.
    tigui_color_t txt_fg_color;       // Text color.
    tigui_color_t check_color;        // Color of the checkmark (should not be the same as checked_bg_color).
    tigui_color_t bdr_color;          // Color of the border when not selected.
    tigui_color_t selected_bdr_color; // Color of the border when selected (should be different than bdr_color);

    /* Management */
    bool selected;                    // Whether or not to draw a border with the color specified by selected_bdr_color.
    bool checked;                     // Whether or not to draw the checkbox with the checkmark.
    bool update;                      // Whether or not to update the checkbox's appearance on the next update. 
                                      // Checked and set to false by tigui_draw_checkbox().
} tigui_checkbox_t;

/* Radio Button */
typedef struct {
    /* Appearance */
    char* label;                          // Label of the radio button, can be NULL.
    uint16_t left_label_padding;          // Number of pixels in between the right edge of the circle and beginning of the label.
    uint16_t x_clip;                      // X coordinate to not put text past.
    uint16_t x, y;                        // Coordinates to draw at, referenced from the center. Note that text will follow after going to the right.
    uint16_t radius;                      // Radius of the outer circle.
    uint16_t dot_radius_reduction_factor; // Controls the amount of free space in between the edges of the inner dot and the outer circle (should be less than radius).
    tigui_color_t unfilled_bg_color;      // Color to fill the outer circle with when it's unfilled.
    tigui_color_t filled_bg_color;        // Color to fill the outer circle with when it's filled.
    tigui_color_t txt_fg_color;           // Text color.
    tigui_color_t dot_color;              // Color to fill the inner circle with.
    tigui_color_t bdr_color;              // Color of the outer circle's border when not selected.
    tigui_color_t selected_bdr_color;     // Color of the outer circle's border when selected (should be different than bdr_color).

    /* Management */
    uint8_t group_id;                     // Number to associate other radio buttons with. See the automatic handling description in tigui_element_type_t.
    bool selected;                        // Whether or not to draw the outer circle with the color specified by selected_bdr_color.
    bool filled;                          // Whether or not to draw the radio button with the inner dot.
    bool update;                          // Whether or not to update the radio button's appearance on the next update. 
                                          // Checked and set to false by tigui_draw_radio_button().
} tigui_radio_button_t;

/* Toggle Switch */
typedef struct {
    /* Appearance */
    char* label;                      // Label of the toggle, can be NULL. Drawn to the left of the toggle.
    uint16_t right_label_padding;     // Number of pixels between the right edge of the label and the left edge of the toggle.
    uint16_t x, y;                    // Coordinates to draw at, referenced from the center.
    tigui_color_t off_color;          // Color of the pill when off.
    tigui_color_t on_color;           // Color of the pill when on.
    tigui_color_t knob_color;         // Color of the sliding knob.
    tigui_color_t txt_fg_color;       // Text color.
    tigui_color_t bdr_color;          // Color of the border when not selected.
    tigui_color_t selected_bdr_color; // Color of the border when selected (should be different than bdr_color).

    /* Management */
    bool selected;                    // Whether or not to draw the border in the color specified by selected_bdr_color.
    bool on;                          // Whether or not the toggle is on.
    bool update;                      // Whether or not to update the toggle's appearance on the next update.
                                      // Checked and set to false by tigui_draw_toggle().
} tigui_toggle_t;

/* Progress Bar */
typedef enum {
    TIGUI_LABEL_POS_TOP,
    TIGUI_LABEL_POS_BOTTOM,
    TIGUI_LABEL_POS_LEFT,
    TIGUI_LABEL_POS_RIGHT
} tigui_label_pos_t;

typedef struct {
    /* Appearance */
    char* label;                      // Label of the progress bar, can be NULL.
    tigui_label_pos_t label_pos;      // Position of the label in relation to the progress bar.
    uint16_t label_padding;           // Padding to apply in between the label and the progress bar.
    uint16_t x, y;                    // Coordinates to draw at, referenced from the center.
    uint16_t w, h;                    // Width and height of the progress bar.
    tigui_color_t bg_color;           // Color of the unprogressed portion of the bar.
    tigui_color_t progressed_color;   // Color of the progressed portion of the bar.
    tigui_color_t txt_fg_color;       // Color of the label and progress printing inside the bar (centered) (should be different than bg_color and progressed_color).
    tigui_color_t bdr_color;          // Color of the border.
    tigui_color_t selected_bdr_color; // Color of the border when selected (should be different than bdr_color).

    /* Management */
    bool selected;                    // Whether or not to draw the border in the color specified by selected_bdr_color.
    float progress;                   // Progress value clamped between 0.0 and 1.0.
    bool update;                      // Whether or not to update the progress bar's appearance on the next update.
                                      // Checked and set to false by tigui_draw_progress_bar().
} tigui_progress_bar_t;

/* Spinner */
typedef struct {
    /* Appearance */
    char* label;                  // Label of the spinner, can be NULL. Will be centered in between all of the spinning dots.
    uint16_t x, y;                // Center of the spinner.
    uint16_t radius;              // Distance from center to dots.
    uint8_t dot_radius;           // Size of each dot.
    uint8_t num_dots;             // Number of dots in the ring.
    tigui_color_t txt_fg_color;   // Text color.
    tigui_color_t active_color;   // The bright dot color.
    tigui_color_t inactive_color; // The dim dot color.

    /* Management */
    uint8_t current_frame;        // [MANAGED INTERNALLY--LEAVE AT 0] Current active dot index.
    uint16_t tick;                // [MANAGED INTERNALLY--LEAVE AT 0] Internal tick counter.
    uint16_t speed;               // How many timer ticks between frame advances.
                                  // A good rule of thumb for setting this field is num_dots / 2. The lower the faster.
                                  // May not apply if you have a shorter orchestrator loop due to not having any navigational elements.
    bool update;                  // Whether or not to update the spinner's appearance on the next update.
                                  // Checked and set to false by tigui_draw_spinner().
} tigui_spinner_t;

/* Slider */
typedef struct {
    /* Appearance */
    char* label;                      // Label of the slider, can be NULL.
    tigui_label_pos_t label_pos;      // Position of the label in relation to the slider.
    uint16_t label_padding;           // Padding to apply in between the label and the slider.
    uint16_t x, y;                    // Coordinates to draw at, referenced from the center.
    uint16_t w, h;                    // Width and height of the slider.
    tigui_color_t bg_color;           // Color of the unslid portion of the slider.
    tigui_color_t slid_color;         // Color of the slid portion of the slider.
    tigui_color_t txt_fg_color;       // Color of the label.
    tigui_color_t bdr_color;          // Color of the border.
    tigui_color_t selected_bdr_color; // Color of the border when selected (should be different than bdr_color).

    /* Management */
    bool selected;                    // Whether or not to draw the border in the color specified by selected_bdr_color.
    float step;                       // How much to step the slid value by (should be no greater than 1.0).
    float slid;                       // Slid value clamped between 0.0 and 1.0.
    bool update;                      // Whether or not to update the slider's appearance on the next update.
                                      // Checked and set to false by tigui_draw_slider().
} tigui_slider_t;

/* Badge */
typedef struct {
    /* Appearance */
    unsigned int value;               // Value that goes inside the badge.
    uint16_t x, y;                    // Coordinates to draw at, referenced from the center.
    uint16_t radius;                  // Radius.
    tigui_color_t filled_color;       // Color to fill the badge with.
    tigui_color_t txt_fg_color;       // Color of the text inside the badge.
    tigui_color_t bdr_color;          // Color of the border when not selected.
    tigui_color_t selected_bdr_color; // Color of the border when selected (should be different than bdr_color).

    /* Management */
    bool selected;                    // Whether or not to draw a border with the color specified by selected_bdr_color.
    bool update;                      // Whether or not to update the badge's appearance on the next update. 
                                      // Checked and set to false by tigui_draw_badge().
} tigui_badge_t;

/* Input Field */
#define tigui_end_keymap_entries() {0, '\0'}

typedef struct {
    sk_key_t key; // A keypad scan code defined in getcsc.h.
    char c;       // The character to type when that key is pressed.
} tigui_keymap_entry_t;

typedef struct {
    char descriptor;               // Type of character that will be typed (e.g. 'a' for lowercase letters).
    tigui_keymap_entry_t* entries; // Keymap to read keypresses by.
} tigui_input_field_descriptor_t;

extern tigui_keymap_entry_t tigui_default_lowercase_ascii_input_descriptor_entries[];
extern tigui_input_field_descriptor_t tigui_default_lowercase_ascii_input_descriptor;

extern tigui_keymap_entry_t tigui_default_uppercase_ascii_input_descriptor_entries[];
extern tigui_input_field_descriptor_t tigui_default_uppercase_ascii_input_descriptor;

extern tigui_keymap_entry_t tigui_default_numeric_input_descriptor_entries[];
extern tigui_input_field_descriptor_t tigui_default_numeric_input_descriptor;

extern tigui_input_field_descriptor_t* tigui_default_input_field_descriptors[];
#define tigui_num_default_input_field_descriptors 3

typedef struct {
    /* Appearance */
    char* label;                                  // Label of the input field, can be NULL.
    char* placeholder;                            // Placeholder text inside the input field when empty.
    tigui_label_pos_t label_pos;                  // Position of the label in relation to the input field.
    uint16_t label_padding;                       // Padding to apply in between the label and the input field.
    uint16_t x, y;                                // Coordinates to draw at, referenced from the center.
    uint16_t w, h;                                // Width and height of the progress bar.
    tigui_color_t bg_color;                       // Color of the input field.
    tigui_color_t txt_fg_color;                   // Color of the label and input text inside the input field.
    tigui_color_t placeholder_txt_fg_color;       // Color of the placeholder text.
    tigui_color_t type_descriptor_bg_color;       // Color of the box that shows what type of characters will be typed.
    tigui_color_t type_descriptor_txt_fg_color;   // Color of the text inside the descriptor box (should be different than type_descriptor_bg_color).
    tigui_color_t bdr_color;                      // Color of the border.
    tigui_color_t selected_bdr_color;             // Color of the border when selected (should be different than bdr_color).

    /* Management */
    bool selected;                                // Whether or not to draw the border in the color specified by selected_bdr.
    tigui_input_field_descriptor_t** descriptors; // Input descriptors. Should look like: [&lowercase_descriptor, &uppercase_descriptor, &numeric_descriptor].
    size_t num_descriptors;                       // Number of descriptors in the input field.
    size_t current_descriptor_i;                  // [MANAGED INTERNALLY--LEAVE AT 0] Current descriptor index.
    sk_key_t descriptor_switch_key;               // Key to cycle through descriptors.
    char* input_buf;                              // Pointer to store inputted characters at.
    size_t input_buf_size;                        // Exact size of the input buffer.
    size_t input_buf_offset;                      // [MANAGED INTERNALLY--LEAVE AT 0] Offset of the input buf where the cursor is.
    bool draw_cursor;                             // [MANAGED INTERNALLY--LEAVE FALSE] Whether or not to draw the cursor.
    bool update;                                  // Whether or not to update the input field's appearance on the next update.
                                                  // Checked and set to false by tigui_draw_input_field().
} tigui_input_field_t;

/* Alert */
/**
 * @brief Callback template used by the alert.
 * @param user_data Opaque pointer set inside of the canvas.
 */
typedef void (*tigui_alert_cb_t)(void* user_data);

typedef struct {
    /* Appearance */
    char* title;                            // Title of the alert, should not be NULL but can be.
    char* body;                             // Body of the alert, should not be NULL but can be.
    uint16_t label_padding;                 // Padding to apply to the title and body.
    char* fail_button_label;                // Label of the "cancel" button.
    char* pass_button_label;                // Label of the "ok" button, should not be NULL but can be.
                                            // If left NULL, the pass button will take up the entire width of the alert.
                                            // If not NULL, the width of the buttons will be halved.
    uint16_t x, y;                          // Coordinates to draw at, referenced from the center of the title and body, plus the button height.
    uint16_t w, h;                          // Width and height of the title and body container.
    uint16_t button_h;                      // Height of the buttons, width will be determined based on the states of the labels (NULL or not).
    tigui_color_t bg_color;                 // Color of the background for all parts of the alert.
    tigui_color_t info_txt_fg_color;        // Text color of the title and body.
    tigui_color_t fail_button_txt_fg_color; // Text color of the failing button.
    tigui_color_t pass_button_txt_fg_color; // Text color of the passing button. If destructive, it's urged to use TIGUI_RED.
    tigui_color_t bdr_color;                // Color of all borders.
    tigui_color_t selected_bdr_color;       // Color of the button's border when selected (should be different than bdr_color).

    /* Management */
    bool is_showing;
    tigui_alert_cb_t pass_cb;              // Callback for if the pass button is pressed.
    tigui_alert_cb_t fail_cb;              // Callback for if the fail button is pressed.
                                           // If both callbacks are NULL, the alert will be dismissed on the press of either button.
    uint8_t selection_i;                   // [MANAGED INTERNALLY--LEAVE AT 0] Button selection index.
    bool update;                           // Whether or not to update the input field's appearance on the next update.
                                           // Checked and set to false by tigui_draw_alert().
} tigui_alert_t;

/* List */
/**
 * @brief Callback template used by the list items.
 * @param text Text of the item that was selected.
 * @param user_data Opaque pointer set inside of the canvas.
 */
typedef void (*tigui_list_item_cb_t)(char* text, void* user_data);

typedef struct {
    char* text;                                    // Text to display inside the row of the list.
    tigui_list_item_cb_t pressed_cb;               // Callback for if this selected item is pressed.
} tigui_list_item_t;

typedef struct {
    /* Appearance */
    char* label;                                           // Label of the list/frame, can be NULL.
    tigui_label_pos_t label_pos;                           // Position of the label in relation to the frame.
    tigui_text_reference_point_t item_txt_reference_point; // Point that text will be drawn at.
    uint16_t x, y;                                         // Coordinates to draw the frame at, referenced from the center.
    uint16_t w, h;                                         // Width and height of the frame.
                                                           // Note that x, y, and w of the items all match the above.
    uint16_t item_h;                                       // Height of each item in the frame.
    tigui_color_t bg_color;                                // Color of the list background (for when the items can't fill the frame).
    tigui_color_t bdr_color;                               // Color of the frame when not selected.
    tigui_color_t selected_bdr_color;                      // Color of the frame when selected (should be different than bdr_color).
    
    /* Management */
    bool selected;                                         // Whether or not to draw the frame in the color specified by selected_bdr_color.
    tigui_list_item_t* items;                              // Items in this list.
    size_t num_items;                                      // Number of items in this list.
    size_t scroll_offset_i;                                // [MANAGED INTERNALLY--LEAVE AT 0] Displayed rows list offset.
    size_t selected_i;                                     // [MANAGED INTERNALLY--LEAVE AT 0] Current selected item.
    bool is_browsing;                                      // [MANAGED INTERNALLY--LEAVE FALSE] Whether or not the user is browsing the list.
    bool update;                                           // Whether or not to update the spinner's appearance on the next update.
                                                           // Checked and set to false by tigui_draw_list().
} tigui_list_t;

/**
 * @brief Initializes the TIGUI library.
 * @note When you're done using the library, don't forget to call tigui_deinit().
 */
void tigui_init(void);

/**
 * @brief Deinitializes the TIGUI library.
 */
void tigui_deinit(void);

/**
 * @brief Returns a human-readable representation for a given error code.
 * @param error The error code.
 * @return A string.
 */
const char* tigui_strerror(tigui_error_t error);

/**
 * @brief Orchestrates a canvas. This includes full screen updates, input handling, and callback responsibility.
 * @param canvas Pointer to the canvas to let the user work with.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_orchestrate_canvas(tigui_canvas_t* canvas);

/**
 * @brief Gets the total number of palette slots.
 * @return An unsigned value.
 * @note This value will be constant throughout the entire program's lifetime.
 */
uint16_t tigui_get_total_palette_slots(void);

/**
 * @brief Gets the total number of used palette slots.
 * @return An unsigned value.
 */
uint16_t tigui_get_num_palette_slots_used(void);

/**
 * @brief Lossy converter of a packed color back into a tigui_color_t.
 * @param color The packed color to unpack.
 * @return A tigui_color_t with SOME percision loss.
 */
tigui_color_t tigui_1555_to_color(uint16_t color);

/**
 * @brief Gets the height of the font's characters.
 * @return Number of pixels high characters are.
 */
uint16_t tigui_get_font_character_height(void);

/**
 * @brief Fills the entire screen with the specified color.
 * @param color The color to fill all 320x240 pixels with.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_bgcolor(tigui_color_t color);

/**
 * @brief Draws a line to the screen.
 * @param line Pointer to the line to draw.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_draw_line(tigui_line_t* line);

/**
 * @brief Draws a rectangle to the screen.
 * @param rectangle Pointer to the rectangle to draw.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_draw_rectangle(tigui_rectangle_t* rectangle);

/**
 * @brief Draws a circle to the screen.
 * @param circle Pointer to the circle to draw.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_draw_circle(tigui_circle_t* circle);

/**
 * @brief Draws an ellipse to the screen.
 * @param ellipse Pointer to the ellipse to draw.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_draw_ellipse(tigui_ellipse_t* ellipse);

/**
 * @brief Draws text to the screen.
 * @param text Pointer to the text to draw.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_draw_text(tigui_text_t* text);

/**
 * @brief Draws a text box to the screen.
 * @param text_box Pointer to the text box to draw.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_draw_text_box(tigui_text_box_t* text_box);

/**
 * @brief Draws a button to the screen.
 * @param button Pointer to the button to draw.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_draw_button(tigui_button_t* button);

/**
 * @brief Draws a checkbox to the screen.
 * @param checkbox Pointer to the checkbox to draw.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_draw_checkbox(tigui_checkbox_t* checkbox);

/**
 * @brief Draws a radio button to the screen.
 * @param radio_button Pointer to the radio button to draw.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_draw_radio_button(tigui_radio_button_t* radio_button);

/**
 * @brief Draws a toggle to the screen.
 * @param toggle Pointer to the toggle to draw.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_draw_toggle(tigui_toggle_t* toggle);

/**
 * @brief Draws a progress bar to the screen.
 * @param progress_bar Pointer to the progress bar to draw.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_draw_progress_bar(tigui_progress_bar_t* progress_bar);

/**
 * @brief Draws a spinner to the screen.
 * @param spinner Pointer to the spinner to draw.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_draw_spinner(tigui_spinner_t* spinner);

/**
 * @brief Draws a slider to the screen.
 * @param slider Pointer to the slider to draw.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_draw_slider(tigui_slider_t* slider);

/**
 * @brief Draws a badge to the screen.
 * @param badge Pointer to the badge to draw.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_draw_badge(tigui_badge_t* badge);

/**
 * @brief Draws an input field to the screen.
 * @param input_field Pointer to the input field to draw.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_draw_input_field(tigui_input_field_t* input_field);

/**
 * @brief Draws an alert to the screen.
 * @param alert Pointer to the alert to draw.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_draw_alert(tigui_alert_t* alert);

/**
 * @brief Draws a list to the screen.
 * @param alert Pointer to the list to draw.
 * @return A tigui_error_t error code.
 */
tigui_error_t tigui_draw_list(tigui_list_t* list);

#endif