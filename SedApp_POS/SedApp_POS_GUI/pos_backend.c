/*
 * ========================================================================
 * RESTAURANT POS SYSTEM - Simple & Creative Version
 * ========================================================================
 *
 * This is a Point-of-Sale system for restaurants that processes orders
 * and tracks daily financial data.
 *
 * ASSIGNMENT REQUIREMENTS MET:
 * ✅ System Navigation: Easy-to-use menus
 * ✅ Order Processing: Manages multiple customer orders
 * ✅ Item Entry: Continuous item entry with quantities
 * ✅ Financial Calculations: Subtotal, discount/tax, final total
 * ✅ Summary Report: Daily revenue and aggregated metrics
 * ✅ Uses: variables, data types, if-else, loops
 * ✅ Includes: Multiple user-defined functions
 *
 * CODE STRUCTURE:
 * - Lines 50-100:   Constants and data structures
 * - Lines 100-200:  Global variables (UI widgets and business data)
 * - Lines 200-400:  Helper functions
 * - Lines 400-600:  Button callback functions (main functionality)
 * - Lines 600-700:  UI setup
 * - Lines 700+:     Main function
 *
 * BUTTON FLOW EXPLANATION:
 *
 * [Add Item Button] → on_add_item() at line 450
 *   Flow: Read inputs → Validate → Add to order → Update totals
 *
 * [Finalize Order Button] → on_finalize_order() at line 500
 *   Flow: Check order → Get payment → Show receipt → Save stats → Reset
 *
 * [Clear Order Button] → on_clear_order() at line 580
 *   Flow: Clear all order data → Reset display
 *
 * [View Summary Button] → on_view_summary() at line 590
 *   Flow: Calculate stats → Show summary dialog
 *
 * [Close Day Button] → on_close_day() at line 620
 *   Flow: Generate report → Show → Exit
 */

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo-pdf.h>
#include <pango/pangocairo.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <locale.h>

/* Returns a newly-allocated directory path of the running executable. Caller must g_free(). */
static char *get_executable_dir(void)
{
    char buf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len < 0)
    {
        return g_get_current_dir();
    }
    buf[len] = '\0';
    return g_path_get_dirname(buf);
}

/* ========================================================================
 * CONSTANTS - Financial Rules
 * ======================================================================== */
#define TAX_RATE 0.10            /* 10% tax for orders below $500 */
#define DISCOUNT_RATE 0.05       /* 5% discount for orders $500+ */
#define DISCOUNT_THRESHOLD 500.0 /* Orders >= $500 get discount */

/* Column numbers for the order list */
enum
{
    COL_ITEM = 0, /* Column 0: Item name */
    COL_QTY,      /* Column 1: Quantity */
    COL_PRICE,    /* Column 2: Unit price */
    COL_LINE,     /* Column 3: Line total (price × qty) */
    NUM_COLS      /* Total columns */
};

/* ========================================================================
 * GLOBAL VARIABLES - UI Widgets
 * ======================================================================== */
static GtkWidget *item_entry;     /* Text box for item name */
static GtkWidget *price_entry;    /* Text box for item price */
static GtkWidget *qty_entry;      /* Text box for quantity (FIXED!) */
static GtkWidget *subtotal_label; /* Label showing subtotal */
static GtkWidget *discount_label; /* Label showing discount */
static GtkWidget *tax_label;      /* Label showing tax */
static GtkWidget *total_label;    /* Label showing total */
static GtkListStore *order_store; /* Data storage for order items */
static GtkWidget *order_view;     /* Tree view to display order items */
static GtkWidget *main_window;    /* Main window reference */
static GtkCssProvider *css_provider; /* For dynamic theme switching */

/* ========================================================================
 * GLOBAL VARIABLES - Business Data
 * ======================================================================== */
static double current_subtotal = 0.0; /* Sum of all items */
static double current_discount = 0.0; /* Discount amount */
static double current_tax = 0.0;      /* Tax amount */
static double current_total = 0.0;    /* Final total */
static int current_order_number = 1;  /* Order counter */

/* Daily statistics */
static double daily_revenue = 0.0;  /* Total money earned today */
static double total_discount = 0.0; /* Total discounts given */
static double total_tax = 0.0;      /* Total tax collected */
static int order_count = 0;         /* Number of orders */
static int total_items_sold = 0;    /* Total items sold */

/* ========================================================================
 * FUNCTION DECLARATIONS
 * ======================================================================== */
static void update_totals(void);
static void reset_current_order(void);
static void refresh_labels(void);
static void on_add_item(GtkButton *button, gpointer data);
static void on_finalize_order(GtkButton *button, gpointer data);
static void on_clear_order(GtkButton *button, gpointer data);
static void on_view_summary(GtkButton *button, gpointer data);
static void on_close_day(GtkButton *button, gpointer data);
static void remove_selected_item(void);
static gboolean on_order_view_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data);
static gboolean on_order_view_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data);
static void on_theme_toggle(GtkToggleButton *toggle, gpointer data);
static void on_print_pdf(GtkButton *button, gpointer data);
static GString *build_receipt_text(const char *payment_method);

/* ========================================================================
 * HELPER FUNCTIONS
 * ======================================================================== */

/**
 * get_double() - Convert text to number
 * Reads text from entry widget and converts to decimal number
 * Returns: The number, or 0.0 if invalid
 */
static double get_double(GtkWidget *entry)
{
    const char *txt = gtk_entry_get_text(GTK_ENTRY(entry));
    return atof(txt); /* Convert string to double */
}

/**
 * get_int() - Convert text to whole number
 * Reads text from entry widget and converts to integer
 * Returns: The number, or 0 if invalid
 */
static int get_int(GtkWidget *entry)
{
    const char *txt = gtk_entry_get_text(GTK_ENTRY(entry));
    return atoi(txt); /* Convert string to int */
}

/**
 * add_row_to_store() - Add item to order list display
 * Creates a new row in the order table showing item details
 */
static void add_row_to_store(const char *name, int qty, double price)
{
    GtkTreeIter iter;
    gtk_list_store_append(order_store, &iter); /* Add new row */
    double line = price * qty;                 /* Calculate: price × quantity */
    gtk_list_store_set(order_store, &iter,
                       COL_ITEM, name,   /* Column 0: Name */
                       COL_QTY, qty,     /* Column 1: Quantity */
                       COL_PRICE, price, /* Column 2: Price */
                       COL_LINE, line,   /* Column 3: Total */
                       -1);
}

/**
 * show_message() - Display popup message
 * Shows a dialog box with a message to the user
 */
static void show_message(const char *title, const char *message)
{
    GtkWidget *dialog = gtk_message_dialog_new(
        main_window ? GTK_WINDOW(main_window) : NULL,
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK, "%s", message);
    gtk_window_set_title(GTK_WINDOW(dialog), title);

    /* Heuristic coloring: classify dialog type from title */
    const char *t = title ? title : "";
    char lower[128];
    size_t i = 0, n = strlen(t);
    if (n >= sizeof(lower)) n = sizeof(lower) - 1;
    for (; i < n; ++i) lower[i] = (char)g_ascii_tolower(t[i]);
    lower[i] = '\0';

    GtkStyleContext *ctx = gtk_widget_get_style_context(dialog);
    gtk_style_context_add_class(ctx, "sedapp-dialog");
    if (strstr(lower, "error") || strstr(lower, "invalid") || strstr(lower, "warning") || strstr(lower, "failed"))
        gtk_style_context_add_class(ctx, "error");
    else if (strstr(lower, "success") || strstr(lower, "confirmed") || strstr(lower, "saved"))
        gtk_style_context_add_class(ctx, "success");
    else
        gtk_style_context_add_class(ctx, "info");

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/* ========================================================================
 * UI UPDATE FUNCTIONS
 * ======================================================================== */

/**
 * refresh_labels() - Update all money display labels
 * Updates the labels showing subtotal, discount, tax, and total
 * Called whenever totals change
 */
static void refresh_labels(void)
{
    char buf[256];

    /* Update subtotal label */
    snprintf(buf, sizeof(buf), "Subtotal: $%.2f", current_subtotal);
    gtk_label_set_text(GTK_LABEL(subtotal_label), buf);

    /* Update discount label - Green color for discount */
    if (current_discount > 0.0)
    {
        snprintf(buf, sizeof(buf), "<span foreground='#3DBE5A'><b>Discount: -$%.2f</b></span>", current_discount);
        gtk_label_set_markup(GTK_LABEL(discount_label), buf);
    }
    else
    {
        snprintf(buf, sizeof(buf), "Discount: -$%.2f", current_discount);
        gtk_label_set_text(GTK_LABEL(discount_label), buf);
    }

    /* Update tax label - Info blue when tax applies */
    if (current_tax > 0.0)
    {
        snprintf(buf, sizeof(buf), "<span foreground='#2F80ED'><b>Tax: $%.2f</b></span>", current_tax);
        gtk_label_set_markup(GTK_LABEL(tax_label), buf);
    }
    else
    {
        snprintf(buf, sizeof(buf), "Tax: $%.2f", current_tax);
        gtk_label_set_text(GTK_LABEL(tax_label), buf);
    }

    /* Update total label - Primary orange color, big and bold */
    snprintf(buf, sizeof(buf), "<span foreground='#E76F00'><b><big>TOTAL: $%.2f</big></b></span>", current_total);
    gtk_label_set_markup(GTK_LABEL(total_label), buf);
}

/**
 * update_totals() - Calculate all money totals
 *
 * This function:
 * 1. Loops through all items in order
 * 2. Adds up all line totals = subtotal
 * 3. Applies financial rule:
 *    - If subtotal >= $500: Apply 5% discount (no tax)
 *    - If subtotal < $500: Apply 10% tax (no discount)
 * 4. Calculates final total
 * 5. Updates display
 *
 * This is called every time an item is added or removed.
 */
static void update_totals(void)
{
    /* Step 1: Calculate subtotal by adding all line totals */
    current_subtotal = 0.0;
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(order_store), &iter);

    /* Loop through all items in the order */
    while (valid)
    {
        double line = 0.0;
        gtk_tree_model_get(GTK_TREE_MODEL(order_store), &iter, COL_LINE, &line, -1);
        current_subtotal += line; /* Add to running total */
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(order_store), &iter);
    }

    /* Step 2: Apply financial rules (discount OR tax, not both) */
    current_discount = 0.0;
    current_tax = 0.0;

    if (current_subtotal >= DISCOUNT_THRESHOLD)
    {
        /* Big order! Give 5% discount, no tax */
        current_discount = current_subtotal * DISCOUNT_RATE;
        current_total = current_subtotal - current_discount;
    }
    else
    {
        /* Small order, add 10% tax, no discount */
        current_tax = current_subtotal * TAX_RATE;
        current_total = current_subtotal + current_tax;
    }

    /* Step 3: Update display labels */
    refresh_labels();
}

/**
 * reset_current_order() - Clear current order
 * Removes all items, resets totals to zero, clears input fields
 * Called when starting a new order
 */
static void reset_current_order(void)
{
    gtk_list_store_clear(order_store); /* Clear order list */

    /* Reset all totals to zero */
    current_subtotal = 0.0;
    current_discount = 0.0;
    current_tax = 0.0;
    current_total = 0.0;

    /* Clear input fields */
    gtk_entry_set_text(GTK_ENTRY(item_entry), "");
    gtk_entry_set_text(GTK_ENTRY(price_entry), "");
    gtk_entry_set_text(GTK_ENTRY(qty_entry), "1"); /* Reset to 1 */

    refresh_labels(); /* Update display */
}

/* ========================================================================
 * REMOVE SELECTED ITEM SUPPORT
 * ======================================================================== */

/**
 * remove_selected_item() - Remove currently selected row from order
 * If a row is selected in the tree view, remove it and update totals.
 */
static void remove_selected_item(void)
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(order_view));
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(selection, NULL, &iter))
    {
        int qty = 0;
        gtk_tree_model_get(GTK_TREE_MODEL(order_store), &iter, COL_QTY, &qty, -1);
        gtk_list_store_remove(order_store, &iter);
        if (qty > 0 && total_items_sold >= qty)
            total_items_sold -= qty;
        update_totals();
    }
    else
    {
        show_message("Remove Item", "Please select an item to remove.");
    }
}

/**
 * on_order_view_key_press() - Handle Delete key to remove selected row
 */
static gboolean on_order_view_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    (void)widget;
    (void)data;
    if (event->keyval == GDK_KEY_Delete || event->keyval == GDK_KEY_KP_Delete)
    {
        remove_selected_item();
        return TRUE;
    }
    return FALSE;
}

/**
 * on_order_view_button_press() - Right-click context menu with Remove action
 */
static gboolean on_order_view_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    (void)widget;
    (void)data;
    if (event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
        GtkWidget *menu = gtk_menu_new();
        GtkWidget *remove_item_menu = gtk_menu_item_new_with_label("Remove Selected");
        g_signal_connect_swapped(remove_item_menu, "activate", G_CALLBACK(remove_selected_item), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), remove_item_menu);
        gtk_widget_show_all(menu);
        /* Popup menu at pointer - use legacy popup for GTK 3.20 compatibility */
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
        return TRUE;
    }
    return FALSE;
}

/* ========================================================================
 * BUTTON CALLBACK FUNCTIONS
 * ========================================================================
 * These functions are called when buttons are clicked.
 * Each function explains what happens step-by-step.
 */

/**
 * on_add_item() - "Add Item" Button Click Handler
 *
 * WHAT IT DOES:
 * When user clicks "Add Item" button, this function:
 * 1. Reads item name, price, and quantity from input fields
 * 2. Checks if inputs are valid (not empty, numbers > 0)
 * 3. Adds the item to the order list
 * 4. Clears input fields for next item
 * 5. Recalculates totals
 *
 * CODE FLOW:
 * Line 450: Function starts
 * Line 453: Read item name from text box
 * Line 454: Read price and convert to number
 * Line 455: Read quantity and convert to number (FIXED!)
 * Lines 458-465: Check if item name is empty → show error → exit
 * Lines 467-472: Check if price <= 0 → show error → exit
 * Lines 474-479: Check if quantity <= 0 → show error → exit
 * Line 482: All checks passed! Add item to order list
 * Lines 484-486: Clear input fields for next item
 * Line 489: Recalculate all totals (calls update_totals())
 * Line 492: Update item count for daily stats
 */
static void on_add_item(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;

    /* Read values from input fields */
    const char *name = gtk_entry_get_text(GTK_ENTRY(item_entry));
    double price = get_double(price_entry);
    int qty = get_int(qty_entry); /* FIXED: Now reads from text entry */

    /* Validation: Check item name */
    if (!name || name[0] == '\0')
    {
        show_message("Error", "Please enter an item name!");
        return;
    }

    /* Validation: Check price */
    if (price <= 0.0)
    {
        show_message("Error", "Price must be greater than $0.00!");
        return;
    }

    /* Validation: Check quantity */
    if (qty <= 0)
    {
        show_message("Error", "Quantity must be at least 1!");
        return;
    }

    /* All validations passed - add item to order */
    add_row_to_store(name, qty, price);

    /* Clear input fields for next item */
    gtk_entry_set_text(GTK_ENTRY(item_entry), "");
    gtk_entry_set_text(GTK_ENTRY(price_entry), "");
    gtk_entry_set_text(GTK_ENTRY(qty_entry), "1"); /* Reset quantity to 1 */

    /* Update totals and display */
    update_totals();

    /* Update daily statistics */
    total_items_sold += qty;
}

/**
 * on_finalize_order() - "Finalize Order" Button Click Handler
 *
 * WHAT IT DOES:
 * When user clicks "Finalize Order", this function:
 * 1. Checks if order has items
 * 2. Asks for payment method (Cash/Card/Digital)
 * 3. Creates a nice receipt
 * 4. Shows receipt to user
 * 5. Saves order to daily statistics
 * 6. Resets order for next customer
 * 7. Increases order number
 *
 * CODE FLOW:
 * Line 500: Function starts
 * Line 503: Check if order is empty → show error → exit
 * Lines 506-520: Create payment method dialog → show → get selection
 * Lines 523-540: Build receipt text with order details
 * Lines 542-550: Add all items to receipt
 * Lines 552-565: Add totals to receipt (subtotal, discount/tax, total)
 * Lines 567-573: Show receipt in dialog
 * Lines 576-579: Update daily statistics
 * Line 582: Reset order for next customer
 * Line 585: Increase order number
 */
static void on_finalize_order(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;

    /* Check if order has items */
    if (current_subtotal <= 0.0)
    {
        show_message("Empty Order", "Please add items before finalizing!");
        return;
    }

    /* Payment method selection */
    GtkWidget *payment_dialog = gtk_dialog_new_with_buttons(
        "Payment Method",
        NULL, GTK_DIALOG_MODAL,
        "Cash", 1,
        "Card", 2,
        "Digital", 3,
        NULL);
// printf "[Desktop Entry]\nVersion=1.0\nType=Application\nName=SEDAPP POS\nComment=Restaurant Point of Sale System\n
// Exec=/home/elshod-rakhmonov/Programming/DSA/CPP/POS_System/SedApp\nIcon=/home/elshod-rakhmonov/Programming/DSA/CPP/POS_System/icon\nTerminal=false\n
// Categories=Office;\n" > SedApp.desktop

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(payment_dialog));
    GtkWidget *label = gtk_label_new("How will the customer pay?");
    gtk_container_add(GTK_CONTAINER(content), label);
    gtk_widget_show_all(payment_dialog);

    /* Get selection and resolve payment method */
    gint payment = gtk_dialog_run(GTK_DIALOG(payment_dialog));
    gtk_widget_destroy(payment_dialog);

    const char *payment_method = "Cash";
    if (payment == 2)
        payment_method = "Card";
    else if (payment == 3)
        payment_method = "Digital";
    /* Build receipt */
    GString *receipt = g_string_new(NULL);
    g_string_append_printf(receipt,
                           "========================================\n"
                           "    RESTAURANT POS SYSTEM\n"
                           "    Order Receipt\n"
                           "========================================\n"
                           "Order #: %d\n"
                           "Payment: %s\n"
                           "----------------------------------------\n",
                           current_order_number, payment_method);

    /* Add all items to receipt */
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(order_store), &iter);
    int item_num = 1;

    while (valid)
    {
        gchar *name = NULL;
        int qty = 0;
        double price = 0, line = 0;

        gtk_tree_model_get(GTK_TREE_MODEL(order_store), &iter,
                           COL_ITEM, &name,
                           COL_QTY, &qty,
                           COL_PRICE, &price,
                           COL_LINE, &line,
                           -1);

        g_string_append_printf(receipt, "%d. %s x%d @ $%.2f = $%.2f\n",
                               item_num++, name, qty, price, line);
        g_free(name);
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(order_store), &iter);
    }

    /* Add totals to receipt */
    g_string_append_printf(receipt, "----------------------------------------\n");
    g_string_append_printf(receipt, "Subtotal:        $%.2f\n", current_subtotal);

    if (current_discount > 0.0)
    {
        g_string_append_printf(receipt, "Discount (5%%):   -$%.2f\n", current_discount);
    }

    if (current_tax > 0.0)
    {
        g_string_append_printf(receipt, "Tax (10%%):       $%.2f\n", current_tax);
    }

    g_string_append_printf(receipt, "----------------------------------------\n");
    g_string_append_printf(receipt, "TOTAL:           $%.2f\n", current_total);
    g_string_append_printf(receipt, "========================================\n");
    g_string_append_printf(receipt, "\nThank you! Come again!\n");

    /* Show receipt */
    GtkWidget *receipt_dialog = gtk_message_dialog_new(
        NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK, "%s", receipt->str);
    gtk_window_set_title(GTK_WINDOW(receipt_dialog), "Order Receipt");
    gtk_window_set_default_size(GTK_WINDOW(receipt_dialog), 400, 500);
    gtk_dialog_run(GTK_DIALOG(receipt_dialog));
    gtk_widget_destroy(receipt_dialog);
    g_string_free(receipt, TRUE);

    /* Update daily statistics */
    order_count++;
    daily_revenue += current_total;
    total_discount += current_discount;
    total_tax += current_tax;

    /* Reset for next order */
    reset_current_order();
    current_order_number++;
}

/**
 * on_clear_order() - "Clear Order" Button Click Handler
 *
 * WHAT IT DOES:
 * Clears the current order without finalizing it.
 * Used when user wants to start over.
 *
 * CODE FLOW:
 * Line 580: Function starts
 * Line 581: Call reset_current_order() which clears everything
 */
static void on_clear_order(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;
    reset_current_order();
}

/**
 * on_view_summary() - "View Summary" Button Click Handler
 *
 * WHAT IT DOES:
 * Shows current day statistics without closing the system.
 * Useful for checking progress during the day.
 *
 * CODE FLOW:
 * Line 590: Function starts
 * Line 593: Calculate average order value
 * Lines 595-605: Build summary text with all statistics
 * Lines 607-611: Show summary in dialog
 */
static void on_view_summary(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;

    double avg_order = order_count > 0 ? daily_revenue / order_count : 0.0;

    char summary[512];
    snprintf(summary, sizeof(summary),
             "========================================\n"
             "    DAILY SUMMARY REPORT\n"
             "========================================\n"
             "Orders Processed:     %d\n"
             "Items Sold:           %d\n"
             "Total Revenue:        $%.2f\n"
             "Total Discounts:      $%.2f\n"
             "Total Tax:            $%.2f\n"
             "Average Order Value:  $%.2f\n"
             "========================================\n",
             order_count, total_items_sold, daily_revenue,
             total_discount, total_tax, avg_order);

    GtkWidget *dialog = gtk_message_dialog_new(
        NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK, "%s", summary);
    gtk_window_set_title(GTK_WINDOW(dialog), "Daily Summary");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/**
 * on_close_day() - "Close Day" Button Click Handler
 *
 * WHAT IT DOES:
 * Generates final end-of-day report and closes the system.
 * Called at the end of business day.
 *
 * CODE FLOW:
 * Line 620: Function starts
 * Line 623: Calculate average order value
 * Lines 625-640: Build comprehensive end-of-day report
 * Lines 642-646: Show report in dialog
 * Line 649: Exit application
 */
static void on_close_day(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;

    double avg_order = order_count > 0 ? daily_revenue / order_count : 0.0;

    char summary[1024];
    snprintf(summary, sizeof(summary),
             "========================================\n"
             "    END OF DAY REPORT\n"
             "    Restaurant POS System\n"
             "========================================\n\n"
             "ORDERS SUMMARY:\n"
             "  Total Orders:        %d\n"
             "  Total Items Sold:    %d\n\n"
             "FINANCIAL SUMMARY:\n"
             "  Total Revenue:       $%.2f\n"
             "  Total Discounts:     $%.2f\n"
             "  Total Tax:           $%.2f\n\n"
             "PERFORMANCE:\n"
             "  Average Order:       $%.2f\n"
             "  Items per Order:     %.1f\n"
             "========================================\n\n"
             "Great work today! See you tomorrow!\n",
             order_count, total_items_sold,
             daily_revenue, total_discount, total_tax,
             avg_order,
             order_count > 0 ? (double)total_items_sold / order_count : 0.0);

    GtkWidget *dialog = gtk_message_dialog_new(
        NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK, "%s", summary);
    gtk_window_set_title(GTK_WINDOW(dialog), "End of Day Report");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 400);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    gtk_main_quit(); /* Exit application */
}

/* ========================================================================
 * CSS STYLING FUNCTION
 * ======================================================================== */

/**
 * apply_css_theme() - Apply colorful theme to the application
 * Sets up CSS styling with custom color scheme
 */
static void apply_css_theme(gboolean dark)
{
    /* SedApp palette */
    /* Orange #E76F00, Deep Orange #C85D00, Yellow #FFEB34, Soft Gold #FFD469,
       Warm White #FFF8E6, Charcoal #2C2C2C, Success #3DBE5A, Error #F34242, Info #2F80ED */

    const char *css_light =
        "/* Base */\n"
        ".main-window { background-color: #FFF8E6; }\n"
        "GtkLabel { color: #2C2C2C; }\n"
        ".header-label { color: #E76F00; font-size: 22px; }\n"
        "GtkFrame { background-color: #FFF8E6; border: 2px solid #FFD469; border-radius: 8px; }\n"
        "GtkEntry { background-color: #FFFFFF; border: 1px solid #FFD469; border-radius: 6px; padding: 6px; }\n"
        "GtkEntry:focus { border: 2px solid #E76F00; }\n"
        ".primary-button, .success-button, .danger-button, .info-button, .secondary-button { border-radius: 10px; padding: 10px 14px; color: #FFFFFF; font-weight: 600; }\n"
        ".primary-button { background-color: #E76F00; } .primary-button:hover { background-color: #C85D00; } .primary-button:active { background-color: #A14A00; }\n"
        ".success-button { background-color: #3DBE5A; } .success-button:hover { background-color: #2DAE4A; } .success-button:active { background-color: #1D9E3A; }\n"
        ".danger-button { background-color: #F34242; } .danger-button:hover { background-color: #E33232; } .danger-button:active { background-color: #D32222; }\n"
        ".info-button { background-color: #2F80ED; } .info-button:hover { background-color: #1F70DD; } .info-button:active { background-color: #0F60CD; }\n"
        ".secondary-button { background-color: #FFEB34; color: #2C2C2C; } .secondary-button:hover { background-color: #FFD469; } .secondary-button:active { background-color: #FFC700; }\n"
        "GtkTreeView { background-color: #FFFFFF; border: 1px solid #FFD469; }\n"
        "GtkTreeView:selected { background-color: #FFF1CC; }\n"
        "treeview header button { background-color: #FFEB34; border: 0; }\n"
        "treeview header button:hover { background-color: #FFD469; }\n"
        "treeview.view row:hover { background-color: #FFF1CC; }\n"
        "/* Menus */\n"
        "menu { background-color: #FFF8E6; border: 1px solid #FFD469; }\n"
        "menu separator { background-color: #FFD469; }\n"
        "menu menuitem:hover { background-color: #FFD469; color: #2C2C2C; }\n"
        "/* Dialogs */\n"
        "GtkMessageDialog.sedapp-dialog { background-color: #FFF8E6; border: 1px solid #FFD469; }\n"
        "GtkMessageDialog.sedapp-dialog.info GtkLabel { color: #2C2C2C; }\n"
        "GtkMessageDialog.sedapp-dialog.error GtkLabel { color: #F34242; }\n"
        "GtkMessageDialog.sedapp-dialog.success GtkLabel { color: #3DBE5A; }\n"
        "/* Tooltips */\n"
        "tooltip { background-color: #FFF8E6; color: #2C2C2C; border: 1px solid #FFD469; }\n";

    const char *css_dark =
        "/* Base */\n"
        ".main-window { background-color: #2C2C2C; }\n"
        "GtkLabel { color: #FFF8E6; }\n"
        ".header-label { color: #FFEB34; font-size: 22px; }\n"
        "GtkFrame { background-color: #333333; border: 1px solid #444444; border-radius: 8px; }\n"
        "GtkEntry { background-color: #2C2C2C; color: #FFF8E6; border: 1px solid #FFD469; border-radius: 6px; padding: 6px; }\n"
        "GtkEntry:focus { border: 2px solid #FFEB34; }\n"
        ".primary-button, .success-button, .danger-button, .info-button, .secondary-button { border-radius: 10px; padding: 10px 14px; color: #FFF8E6; font-weight: 600; }\n"
        ".primary-button { background-color: #E76F00; } .primary-button:hover { background-color: #C85D00; } .primary-button:active { background-color: #A14A00; }\n"
        ".success-button { background-color: #3DBE5A; } .success-button:hover { background-color: #2DAE4A; } .success-button:active { background-color: #1D9E3A; }\n"
        ".danger-button { background-color: #F34242; } .danger-button:hover { background-color: #E33232; } .danger-button:active { background-color: #D32222; }\n"
        ".info-button { background-color: #2F80ED; } .info-button:hover { background-color: #1F70DD; } .info-button:active { background-color: #0F60CD; }\n"
        ".secondary-button { background-color: #FFD469; color: #2C2C2C; } .secondary-button:hover { background-color: #FFEB34; } .secondary-button:active { background-color: #FFC700; }\n"
        "GtkTreeView { background-color: #2C2C2C; color: #FFF8E6; border: 1px solid #444444; }\n"
        "GtkTreeView:selected { background-color: #404040; }\n"
        "treeview header button { background-color: #3A3A3A; border: 0; }\n"
        "treeview header button:hover { background-color: #4A4A4A; }\n"
        "treeview.view row:hover { background-color: #3A3A3A; }\n"
        "/* Menus */\n"
        "menu { background-color: #2C2C2C; border: 1px solid #444444; }\n"
        "menu separator { background-color: #444444; }\n"
        "menu menuitem:hover { background-color: #404040; color: #FFF8E6; }\n"
        "/* Dialogs */\n"
        "GtkMessageDialog.sedapp-dialog { background-color: #2C2C2C; border: 1px solid #444444; }\n"
        "GtkMessageDialog.sedapp-dialog.info GtkLabel { color: #FFF8E6; }\n"
        "GtkMessageDialog.sedapp-dialog.error GtkLabel { color: #F34242; }\n"
        "GtkMessageDialog.sedapp-dialog.success GtkLabel { color: #3DBE5A; }\n"
        "/* Tooltips */\n"
        "tooltip { background-color: #3A3A3A; color: #FFF8E6; border: 1px solid #444444; }\n";

    if (!css_provider)
    {
        css_provider = gtk_css_provider_new();
        GtkStyleContext *context = gtk_widget_get_style_context(main_window);
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    const char *css = dark ? css_dark : css_light;
    GError *error = NULL;
    gtk_css_provider_load_from_data(css_provider, css, -1, &error);
    if (error)
    {
        g_printerr("CSS Error: %s\n", error->message);
        g_error_free(error);
    }
}

static void on_theme_toggle(GtkToggleButton *toggle, gpointer data)
{
    (void)data;
    gboolean active = gtk_toggle_button_get_active(toggle);
    apply_css_theme(active);
}

static GString *build_receipt_text(const char *payment_method)
{
    if (!payment_method)
        payment_method = "N/A";

    GString *receipt = g_string_new(NULL);
    g_string_append_printf(receipt,
                           "========================================\n"
                           "    RESTAURANT POS SYSTEM\n"
                           "    Order Receipt\n"
                           "========================================\n"
                           "Order #: %d\n"
                           "Payment: %s\n"
                           "----------------------------------------\n",
                           current_order_number, payment_method);

    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(order_store), &iter);
    int item_num = 1;
    while (valid)
    {
        gchar *name = NULL;
        int qty = 0;
        double price = 0, line = 0;
        gtk_tree_model_get(GTK_TREE_MODEL(order_store), &iter,
                           COL_ITEM, &name,
                           COL_QTY, &qty,
                           COL_PRICE, &price,
                           COL_LINE, &line,
                           -1);
        g_string_append_printf(receipt, "%d. %s x%d @ $%.2f = $%.2f\n",
                               item_num++, name, qty, price, line);
        g_free(name);
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(order_store), &iter);
    }

    g_string_append_printf(receipt, "----------------------------------------\n");
    g_string_append_printf(receipt, "Subtotal:        $%.2f\n", current_subtotal);
    if (current_discount > 0.0)
        g_string_append_printf(receipt, "Discount (5%%):   -$%.2f\n", current_discount);
    if (current_tax > 0.0)
        g_string_append_printf(receipt, "Tax (10%%):       $%.2f\n", current_tax);
    g_string_append_printf(receipt, "----------------------------------------\n");
    g_string_append_printf(receipt, "TOTAL:           $%.2f\n", current_total);
    g_string_append_printf(receipt, "========================================\n\nThank you! Come again!\n");
    return receipt;
}

static void on_print_pdf(GtkButton *button, gpointer data)
{
    (void)button;
    (void)data;
    if (current_subtotal <= 0.0)
    {
        show_message("No Items", "Please add items before printing a receipt.");
        return;
    }

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Save Receipt as PDF",
                                                    GTK_WINDOW(main_window),
                                                    GTK_FILE_CHOOSER_ACTION_SAVE,
                                                    "Cancel", GTK_RESPONSE_CANCEL,
                                                    "Save", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    char default_name[64];
    snprintf(default_name, sizeof(default_name), "receipt_Order_%d.pdf", current_order_number);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), default_name);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "PDF files");
    gtk_file_filter_add_pattern(filter, "*.pdf");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (!g_str_has_suffix(filename, ".pdf"))
        {
            char *with_ext = g_strconcat(filename, ".pdf", NULL);
            g_free(filename);
            filename = with_ext;
        }

        GString *receipt = build_receipt_text("PDF");

        cairo_surface_t *surface = cairo_pdf_surface_create(filename, 595, 842);
        cairo_t *cr = cairo_create(surface);
        PangoLayout *layout = pango_cairo_create_layout(cr);
        PangoFontDescription *desc = pango_font_description_from_string("Monospace 10");
        pango_layout_set_font_description(layout, desc);
        pango_font_description_free(desc);
        pango_layout_set_width(layout, 540 * PANGO_SCALE);
        pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
        pango_layout_set_text(layout, receipt->str, -1);
        cairo_translate(cr, 30, 30);
        pango_cairo_show_layout(cr, layout);
        g_object_unref(layout);
        cairo_destroy(cr);
        cairo_surface_finish(surface);
        cairo_surface_destroy(surface);
        g_string_free(receipt, TRUE);
        show_message("Saved", "Receipt saved as PDF successfully.");
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

/* ========================================================================
 * UI SETUP FUNCTION
 * ======================================================================== */

/* Resolve the directory where the executable resides (Linux) */

/**
 * setup_treeview() - Configure order items display table
 * Sets up columns: Item Name, Quantity, Price, Line Total
 */
static void setup_treeview(GtkWidget *view)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;

    /* Column 1: Item Name */
    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Item Name", renderer, "text", COL_ITEM, NULL);
    gtk_tree_view_column_set_expand(col, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

    /* Column 2: Quantity */
    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Qty", renderer, "text", COL_QTY, NULL);
    gtk_tree_view_column_set_alignment(col, 0.5);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

    /* Column 3: Price */
    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Price", renderer, "text", COL_PRICE, NULL);
    gtk_tree_view_column_set_alignment(col, 1.0);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

    /* Column 4: Line Total */
    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Total", renderer, "text", COL_LINE, NULL);
    gtk_tree_view_column_set_alignment(col, 1.0);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
}

/* ========================================================================
 * MAIN FUNCTION - Program Entry Point
 * ========================================================================
 *
 * WHAT IT DOES:
 * 1. Starts GTK library
 * 2. Loads UI from XML file
 * 3. Gets references to all UI widgets (buttons, labels, text boxes)
 * 4. Sets up data storage
 * 5. Connects buttons to functions (when clicked → call function)
 * 6. Shows window
 * 7. Starts main loop (program runs until closed)
 *
 * CODE FLOW:
 * Line 700: Program starts
 * Line 703: Initialize GTK
 * Line 706: Load UI file
 * Line 709: Get main window
 * Lines 712-717: Get input widgets (text boxes)
 * Lines 720-724: Get label widgets (for displaying totals)
 * Lines 727-731: Get button widgets
 * Line 734: Get order list widget
 * Lines 737-741: Create data storage for order items
 * Line 744: Connect data storage to display
 * Line 747: Setup table columns
 * Line 750: Set default quantity to 1
 * Lines 753-761: Connect buttons to functions
 *   - When "Add Item" clicked → call on_add_item()
 *   - When "Finalize Order" clicked → call on_finalize_order()
 *   - When "Clear Order" clicked → call on_clear_order()
 *   - When "View Summary" clicked → call on_view_summary()
 *   - When "Close Day" clicked → call on_close_day()
 * Line 764: Initialize display labels
 * Line 767: Show window
 * Line 770: Start main loop (program runs here until closed)
 * Lines 773-774: Cleanup and exit
 */
int main(int argc, char *argv[])
{
    /* Ensure UTF-8 locale so GtkBuilder parses UI with emoji safely */
    if (!setlocale(LC_ALL, ""))
    {
        setlocale(LC_ALL, "C.UTF-8");
    }

    /* Initialize GTK library */
    gtk_init(&argc, &argv);

    /* Build absolute path to UI so running from any directory works */
    char *exe_dir = get_executable_dir();
    char *ui_path = g_build_filename(exe_dir, "pos_frontend.ui", NULL);

    /* Load UI from XML file (absolute path first, then fallback) */
    GtkBuilder *builder = gtk_builder_new();
    GError *error = NULL;

    if (!gtk_builder_add_from_file(builder, ui_path, &error))
    {
        g_printerr("Error loading UI from '%s': %s\n", ui_path, error->message);
        g_error_free(error);
        error = NULL;
        /* Try relative path as a fallback */
        if (!gtk_builder_add_from_file(builder, "pos_frontend.ui", &error))
        {
            g_printerr("Error loading UI from 'pos_frontend.ui': %s\n", error->message);
            g_error_free(error);
            g_free(ui_path);
            g_free(exe_dir);
            return 1;
        }
    }

    /* Get main window */
    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    main_window = window;
    /* Set window icon from icon.png if present */
    char *icon_path = g_build_filename(exe_dir, "icon.png", NULL);
    GError *icon_err = NULL;
    GdkPixbuf *icon = gdk_pixbuf_new_from_file(icon_path, &icon_err);
    if (icon)
    {
        gtk_window_set_icon(GTK_WINDOW(window), icon);
        g_object_unref(icon);
    }
    if (icon_err)
        g_clear_error(&icon_err);
    g_free(icon_path);
    /* Get input widgets */
    item_entry = GTK_WIDGET(gtk_builder_get_object(builder, "item_entry"));
    price_entry = GTK_WIDGET(gtk_builder_get_object(builder, "price_entry"));
    qty_entry = GTK_WIDGET(gtk_builder_get_object(builder, "qty_entry")); /* FIXED! */

    /* Get label widgets */
    subtotal_label = GTK_WIDGET(gtk_builder_get_object(builder, "subtotal_label"));
    discount_label = GTK_WIDGET(gtk_builder_get_object(builder, "discount_label"));
    tax_label = GTK_WIDGET(gtk_builder_get_object(builder, "tax_label"));
    total_label = GTK_WIDGET(gtk_builder_get_object(builder, "total_label"));

    /* Get button widgets */
    GtkWidget *add_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_button"));
    GtkWidget *finalize_button = GTK_WIDGET(gtk_builder_get_object(builder, "finalize_button"));
    GtkWidget *clear_button = GTK_WIDGET(gtk_builder_get_object(builder, "clear_button"));
    GtkWidget *summary_button = GTK_WIDGET(gtk_builder_get_object(builder, "summary_button"));
    GtkWidget *close_day_button = GTK_WIDGET(gtk_builder_get_object(builder, "close_day_button"));

    /* Get order list widget */
    order_view = GTK_WIDGET(gtk_builder_get_object(builder, "order_view"));
    GtkWidget *print_button = GTK_WIDGET(gtk_builder_get_object(builder, "print_button"));
    GtkWidget *theme_toggle = GTK_WIDGET(gtk_builder_get_object(builder, "theme_toggle"));

    /* Create data storage for order items */
    order_store = gtk_list_store_new(NUM_COLS,
                                     G_TYPE_STRING,  /* Item name */
                                     G_TYPE_INT,     /* Quantity */
                                     G_TYPE_DOUBLE,  /* Price */
                                     G_TYPE_DOUBLE); /* Line total */

    /* Connect data storage to display */
    gtk_tree_view_set_model(GTK_TREE_VIEW(order_view), GTK_TREE_MODEL(order_store));

    /* Setup table columns */
    setup_treeview(order_view);

    /* Set default quantity */
    gtk_entry_set_text(GTK_ENTRY(qty_entry), "1");

    /* Enable events and connect removal shortcuts */
    gtk_widget_add_events(order_view, GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK);
    g_signal_connect(order_view, "key-press-event", G_CALLBACK(on_order_view_key_press), NULL);
    g_signal_connect(order_view, "button-press-event", G_CALLBACK(on_order_view_button_press), NULL);

    /* Connect buttons to functions */
    g_signal_connect(add_button, "clicked", G_CALLBACK(on_add_item), NULL);
    g_signal_connect(finalize_button, "clicked", G_CALLBACK(on_finalize_order), NULL);
    g_signal_connect(clear_button, "clicked", G_CALLBACK(on_clear_order), NULL);
    g_signal_connect(summary_button, "clicked", G_CALLBACK(on_view_summary), NULL);
    g_signal_connect(close_day_button, "clicked", G_CALLBACK(on_close_day), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    if (print_button)
        g_signal_connect(print_button, "clicked", G_CALLBACK(on_print_pdf), NULL);
    if (theme_toggle)
        g_signal_connect(theme_toggle, "toggled", G_CALLBACK(on_theme_toggle), NULL);

    /* Apply theme (start in light mode) */
    apply_css_theme(FALSE);

    /* Initialize display */
    refresh_labels();

    /* Show window and start program */
    gtk_widget_show_all(window);
    gtk_main();

    /* Cleanup */
    g_free(ui_path);
    g_free(exe_dir);
    g_object_unref(order_store);
    g_object_unref(builder);

    return 0;
}