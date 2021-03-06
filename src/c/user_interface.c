/*---------------------------------------------------------------------------

Authors:  Adrien Thabuis
          Antoine Laurens
          Jean-François Burnier
          Hugo Dupont
          Hugo Viard

Version: 1.0
Date: 11.2016

user_interface.c

---------------------------------------------------------------------------*/

#include <pebble.h>
#include "user_interface.h"
#include "step_frequency.h"
#include "acceleration_process.h"

#define STRIDE_L_FACTOR 0.00414 //average factor to have the stride length with the height of the user in m


// Declare the windows and layers
Window *main_window;
Window *reset_window;
Window *step_display_window;
Window *user_height_window;

MenuLayer *main_menu_layer;
TextLayer *reset_layer;
TextLayer *height_m_layer;
TextLayer *height_cm_layer;
TextLayer *steps_layer;
TextLayer *background_layer;

// Menu entry labels
char* menu_label[4] = {"Start","Step count","Change height","Reset"};

// Global varables
int user_height[2] = {1,70};  // height of user, [meters][centimeters]
int distance_travelled = 0;   // in meters
bool first_call = true;       //pedometer first call (after a reset)

// enum to distinguish between the different option of menu
enum Menu_title {START_COUNT, CHANGE_HEIGHT, RESET, MAX_MENU};

/*
 * \brief  Update user height display
 */
void update_user_height_display(void);

/*
 * \brief  Callback for reset event
 */
void reset_callback(void);

/*
 * \brief  Callback for down click event
 */
void down_single_click_handler(ClickRecognizerRef recognizer, void *context);

/*
 * \brief  Callback for select click event
 */
void select_single_click_handler(ClickRecognizerRef recognizer, void *context);

/*
 * \brief  Callback for up click event
 */
void up_single_click_handler(ClickRecognizerRef recognizer, void *context);

/*
 * \brief  Configure click callbacks
 */
void config_provider(Window *window);

/*
 * \brief   Callback for menu layer
 * \return  number of rows for menu layer
 */
uint16_t get_num_rows_callback( MenuLayer *menu_layer,  uint16_t section_index, void *context);

/*
 * \brief   Callback for menu layer
 * \details Draws the row for a menu layer
 */
void draw_row_callback( GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context);

/*
 * \brief   Callback for menu layer
 * \return  Height of cells
 */
int16_t get_cell_height_callback( struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context);

/*
 * \brief     Callback select click
 * \details   Callback for the menu layer, called
 *            when a select click event occurs
 */
void select_callback( struct MenuLayer *menu_layer,  MenuIndex *cell_index, void *context);


//---------------------------------
// Public functions implementation
//---------------------------------

void open_reset_window()
{
    // Create reset Window element and assign to pointer
    reset_window = window_create();
    Layer *window_layer = window_get_root_layer(reset_window);  
    GRect bounds = layer_get_bounds(window_layer);
  
    // Create background Layer
    background_layer = text_layer_create(GRect( 0, 0, bounds.size.w, bounds.size.h));
    // Setup background layer color (black)
    text_layer_set_background_color(background_layer, GColorBlack);
  
    // Create the TextLayer with specific bounds
    reset_layer = text_layer_create(GRect( 0, 0, bounds.size.w, bounds.size.h));
  
    // Setup layer Information
    text_layer_set_background_color(reset_layer, GColorClear);
    text_layer_set_text_color(reset_layer, GColorWhite);  
    text_layer_set_font(reset_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text_alignment(reset_layer, GTextAlignmentCenter);
  
    // Add to the Window
    layer_add_child(window_layer, text_layer_get_layer(background_layer));
    layer_add_child(window_layer, text_layer_get_layer(reset_layer));
    
    // Add text and register callback
    text_layer_set_text(reset_layer, "Reset Done!");
    app_timer_register(1000, reset_callback, NULL); // window will be closed after 1s
  
    // Show the window on the watch, with animated = true
    window_stack_push(reset_window, true);
    return;
}

void close_reset_window(void)
{
    // Destroy the reset text layer
    text_layer_destroy(reset_layer);

    // Show the window on the watch, with animated = false
    window_stack_push(reset_window, false);
  
    // Destroy the reset window
    window_destroy(reset_window);
    return;
}

void open_step_display_window(void)
{
  
    // Create step display Window element and assign to pointer
    step_display_window = window_create();
    Layer *window_layer = window_get_root_layer(step_display_window);  
    GRect bounds = layer_get_bounds(window_layer);
  
    // Create background Layer
    background_layer = text_layer_create(GRect( 0, 0, bounds.size.w, bounds.size.h));
    // Setup background layer color (black)
    text_layer_set_background_color(background_layer, GColorBlack);
  
    // Create the TextLayer with specific bounds
    steps_layer = text_layer_create(GRect( 0, 0, bounds.size.w, bounds.size.h));
  
    // Setup layer Information
    text_layer_set_background_color(steps_layer, GColorClear);
    text_layer_set_text_color(steps_layer, GColorWhite);  
    text_layer_set_font(steps_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text_alignment(steps_layer, GTextAlignmentCenter);
  
    // Add to the Window
    layer_add_child(window_layer, text_layer_get_layer(background_layer));
    layer_add_child(window_layer, text_layer_get_layer(steps_layer));
  
    // Update number of step display
    uint16_t n_step = get_n_steps();
    update_number_steps_display(n_step);
  
    // Show the window on the watch, with animated = true
    window_stack_push(step_display_window, true);
    return;
}

void close_step_display_window(void)
{
    // Destroy the steo layer
    text_layer_destroy(steps_layer);

    // Show the window on the watch, with animated = false
    window_stack_push(step_display_window, false);
  
    // Destroy the step display window
    window_destroy(step_display_window);
    return;
}

void open_user_height_window(void)
{
    // Create user height Window element and assign to pointer
    user_height_window = window_create();
    Layer *window_layer = window_get_root_layer(user_height_window);  
    GRect bounds = layer_get_bounds(window_layer);
  
    // Create background Layer
    background_layer = text_layer_create(GRect( 0, 0, bounds.size.w, bounds.size.h));
    // Setup background layer color (black)
    text_layer_set_background_color(background_layer, GColorWhite);
  
    int padding = 5; // Layer padding
    int width = (bounds.size.w-2*padding); // layer width
    int height = bounds.size.h/3; // layer height
    
    // Create the TextLayer with specific bounds
    height_m_layer  = text_layer_create(GRect(padding,padding,width,height));
    height_cm_layer = text_layer_create(GRect(padding,2*padding+height,width,height));
  
    // Setup layer Information
    text_layer_set_background_color(height_m_layer, GColorBlack);
    text_layer_set_text_color(height_m_layer, GColorWhite); 
    text_layer_set_font(height_m_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_text_alignment(height_m_layer, GTextAlignmentCenter);
  
    text_layer_set_background_color(height_cm_layer, GColorBlack);
    text_layer_set_text_color(height_cm_layer, GColorWhite);  
    text_layer_set_font(height_cm_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_text_alignment(height_cm_layer, GTextAlignmentCenter);
  
    // Add to the Window
    layer_add_child(window_layer, text_layer_get_layer(background_layer));
    layer_add_child(window_layer, text_layer_get_layer(height_m_layer));
    layer_add_child(window_layer, text_layer_get_layer(height_cm_layer));
    
    // Update height display
    update_user_height_display();
  
    // Configure click callbacks
    window_set_click_config_provider(user_height_window, (ClickConfigProvider) config_provider);
  
    // Show the window on the watch, with animated = true
    window_stack_push(user_height_window, true);
                        
    return;
}

void close_user_height_window()
{
    // Destroy the height layers
    text_layer_destroy(height_m_layer);
    text_layer_destroy(height_cm_layer);
  
    // Show the window on the watch, with animated = false
    window_stack_push(user_height_window, false);

    // Destroy the user height window
    window_destroy(user_height_window);
  
    return;
}

void open_main_window(void)
{
    // Create main Window element and assign to pointer
    main_window = window_create();
    Layer *window_layer = window_get_root_layer(main_window);  
    GRect bounds = layer_get_bounds(window_layer);
  
    // Create the MenuLayer
    main_menu_layer = menu_layer_create(bounds);

    // Let it receive click events
    menu_layer_set_click_config_onto_window(main_menu_layer, main_window);

    // Set the callbacks for behavior and rendering
    menu_layer_set_callbacks(main_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_rows = get_num_rows_callback,
    .draw_row = draw_row_callback,
    .get_cell_height = get_cell_height_callback,
    .select_click = select_callback,
    });

    // Add to the Window
    layer_add_child(window_layer, menu_layer_get_layer(main_menu_layer));
  
    // Show the window on the watch, with animated = true
    window_stack_push(main_window, true);
    return;
}

void close_main_window(void)
{
    // Destroy the menu layer
    menu_layer_destroy(main_menu_layer);

    // Destroy the main window
    window_destroy(main_window);
  
    return;
}

//---------------------------------
// Private functions implementation
//---------------------------------

void update_number_steps_display(const uint16_t steps_number)
{
    static char nb_steps_display[60];

    distance_travelled = (int) (steps_number * (user_height[H_M]*100 + user_height[H_CM]) * STRIDE_L_FACTOR);
    
    if (distance_travelled>=1000)
    {
      int kmetres = distance_travelled/1000;
      int metres = distance_travelled-(1000*kmetres);

      if (metres<10)
      {
        snprintf(nb_steps_display, 60, "Steps:\n%u\nDistance :\n%d.00%d km", steps_number, kmetres, metres);
      }
      else if (metres<100)
      {
        snprintf(nb_steps_display, 60, "Steps:\n%u\nDistance :\n%d.0%d km", steps_number, kmetres, metres);
      }
      else
      {
        snprintf(nb_steps_display, 60, "Steps:\n%u\nDistance :\n%d.%d km", steps_number, kmetres, metres);
      }
    }
    else
    {
      snprintf(nb_steps_display, 60, "Steps:\n%u\nDistance :\n%d m", steps_number, distance_travelled);
    }

    // Write the distance in the step layer
    text_layer_set_text(steps_layer, nb_steps_display);
}

void update_user_height_display(void)
{
    static char m[5];
    static char cm[5];
  
    snprintf(m, 5, "%dm",user_height[H_M]);
    
    if (user_height[H_CM] < 10)
    {
      snprintf(cm, 5, "0%d",user_height[H_CM]);
    }
    else
    {
      snprintf(cm, 5, "%d",user_height[H_CM]);
    }
  
    // Write the height in the height layer
    text_layer_set_text(height_m_layer, m);
    text_layer_set_text(height_cm_layer, cm);

    return;
}

void reset_callback(void)
{
    close_reset_window();

    // Show the window on the watch, with animated = true
    window_stack_push(main_window, true);
    return;
}

void down_single_click_handler(ClickRecognizerRef recognizer, void *context)
{  
    int user_min_height = 70;
  
    if (user_height[H_M]*100+user_height[H_CM] <= user_min_height)
    {
        // Height cannot be less than min height
        return;  
    }

    if (user_height[H_M] >= 0 && user_height[H_CM]-5 < 0)
    {
        if (user_height[H_M] == 0)
        {
            user_height[H_CM] = 0;
        }
        else
        {
          user_height[H_CM] = 95;
          user_height[H_M]--;
        }
    }
    else
    {
        user_height[H_CM] -= 5;  
    }

    update_user_height_display();

    return;
}

void select_single_click_handler(ClickRecognizerRef recognizer, void *context)
{
    close_user_height_window();

    // Show the window on the watch, with animated = true
    window_stack_push(main_window, true);

    return;
}

void up_single_click_handler(ClickRecognizerRef recognizer, void *context)
{
    if(user_height[H_M]!= 3 && user_height[H_CM]+5 >= 100)
    {
        if(user_height[H_M] == 2)
        {
            // Height cannont exceed 2.95m
            user_height[H_CM] = 95;
        }
        else
        {
          user_height[H_CM] = 0;
          user_height[H_M]++;
        }
    }
    else
    {
        user_height[H_CM] += 5;
    }

    update_user_height_display();

    return;
}

void config_provider(Window *window) 
{
  // Single click callback
  window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);

  return;
}

uint16_t get_num_rows_callback( MenuLayer *menu_layer,  uint16_t section_index, void *context)
{
  const uint16_t num_rows = MAX_MENU;
  return num_rows;
}

void draw_row_callback( GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context)
{
  static char s_buff[16];
  
  switch (cell_index->row)
  {
      case START_COUNT:
          snprintf(s_buff, sizeof(s_buff), first_call ? menu_label[0]:menu_label[1]);
          break;
      
      case CHANGE_HEIGHT:
          snprintf(s_buff, sizeof(s_buff), menu_label[2]);
          break;
    
      case RESET:
          snprintf(s_buff, sizeof(s_buff), menu_label[3]);
          break;
    
      default:
          snprintf(s_buff, sizeof(s_buff), "Row %d", (int)cell_index->row);
          break;
  }
  
  // Draw this row's index
  menu_cell_basic_draw(ctx, cell_layer, s_buff, NULL, NULL);
  return;
}

int16_t get_cell_height_callback( struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context)
{
    const int16_t cell_height = 44;
    return cell_height;
}

void select_callback( struct MenuLayer *menu_layer,  MenuIndex *cell_index, void *context)
{
  switch (cell_index->row)
  {
    case START_COUNT:
      open_step_display_window();
      if(first_call == true)
      {
        init_accel();
        first_call = false;
      }
      break;

    case CHANGE_HEIGHT:
      open_user_height_window(); 
      break;

    case RESET:
      first_call = true ;
      reset_n_steps();
      accel_data_service_unsubscribe();//Stop Accelerometer
      distance_travelled = 0;
      open_reset_window();
      break;

    default:
      // Do nothing
      break;
  }

  return;
}