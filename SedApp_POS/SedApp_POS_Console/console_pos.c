#include <stdio.h>

/* Maximum items allowed in one order */
#define MAX_ITEMS 200

/* Maximum length of item name */
#define NAME_LEN 64

/*
 * ---------------------------------------------------------
 * BUSINESS RULES
 * ---------------------------------------------------------
 * • Subtotal >= 500.00  → 5% discount, no tax
 * • Subtotal <  500.00  → 10% tax, no discount
 * • Total = (Subtotal - Discount) + Tax
 */
#define TAX_RATE 0.10
#define DISCOUNT_RATE 0.05
#define DISCOUNT_LIMIT 500.0

/*
 * Item
 * -------------------------------------------------
 * Represents a single product in the order.
 *
 * name  → item name (example: "Burger")
 * qty   → quantity ordered (example: 2)
 * price → unit price (example: 15.50)
 */
typedef struct
{
    char name[NAME_LEN];
    int qty;
    double price;
} Item;

/*
 * Order
 * --------------------------------------
 * Holds all items and calculated figures.
 */
typedef struct
{
    Item items[MAX_ITEMS];
    int count;
    double subtotal;
    double discount;
    double tax;
    double total;
    int orderNo;
} Order;

/* ---------------- DAILY SUMMARY DATA ---------------- */

double dailyRevenue = 0;
double dailyTax = 0;
double dailyDiscount = 0;
int dailyOrders = 0;
int dailyItems = 0;

/*
 * Reset Order function
 * -------------------------------------------------
 * Clears the current order so a new customer can start.
 */
void reset_order(Order *o)
{
    o->count = 0;
    o->subtotal = 0;
    o->discount = 0;
    o->tax = 0;
    o->total = 0;
}

/*
 *  Read Options function
 *  -------------------------------------------------
 *  Read only based on chosen range. Handle if the user enter invalid inputs.
 */
int read_options(int min, int max)
{
    char buffer[32];
    int value;
    char extra;

    while (1)
    {
        printf("> Choose from given options(%d-%d): ", min, max);

        if (!fgets(buffer, sizeof buffer, stdin))
            continue;

        if (buffer[0] == '\n')
            continue;
        if (sscanf(buffer, "%d %c", &value, &extra) == 1 &&
            value >= min && value <= max)
        {
            return value;
        }
        else
        {
            printf("\n  -!- Invalid input. Enter a number between %d and %d. -!-\n\n",
                   min, max);
        }
    }
}

/*
 * Calculate Totals function
 * -------------------------------------------------
 * Recalculates subtotal, discount, tax, and total.
 * Called every time items added.
 */
void calculate_totals(Order *o)
{
    o->subtotal = 0;

    /* Calculate subtotal */
    for (int i = 0; i < o->count; i++)
    {
        o->subtotal += o->items[i].price * o->items[i].qty;
    }

    /* Apply discount or tax */
    if (o->subtotal >= DISCOUNT_LIMIT)
    {
        o->discount = o->subtotal * DISCOUNT_RATE;
        o->tax = 0;
    }
    else
    {
        o->discount = 0;
        o->tax = o->subtotal * TAX_RATE;
    }

    /* Final total */
    o->total = (o->subtotal - o->discount) + o->tax;
}

/*
 * Add Item function
 * -------------------------------------------------
 * Reads item name, price, and quantity from user and add new item.
 *
 * USER INPUT RULES:
 * • Name     : text (no newline issues)
 * • Price    : positive decimal (example: 9.99)
 * • Quantity : positive integer (example: 1)
 */
void add_item(Order *o)
{
    char choice;
    printf("\n|------------------------------ ADD ITEM -----------------------------|\n");
    do
    {

        if (o->count >= MAX_ITEMS)
        {
            printf("  -!- Item limit reached -!-\n");
            return;
        }

        int countNumber = o->count + 1;
        printf("\n|---------------------------- %d-ITEM DATA ----------------------------|\n", countNumber);
        printf("|---------------------------------------------------------------------|\n\n");

        Item *it = &o->items[o->count];

        while (1)
        {
            printf("> Item name   : ");
            scanf(" %63[^\n]", it->name);

            printf("> Unit price  : ");
            scanf("%lf", &it->price);

            printf("> Quantity    : ");
            scanf("%d", &it->qty);

            if (it->price <= 0 || it->qty <= 0)
            {
                printf("\n\n  -!- Invalid price or quantity. Please try again -!-\n\n");
                while (getchar() != '\n')
                    ;
                continue;
            }
            break;
        }

        o->count++;
        calculate_totals(o);

        printf("\n-*- Item added successfully -*-\n\n");
        printf("-> Do you want to add another item? (y/n): ");
        scanf(" %c", &choice);

    } while (choice == 'y' || choice == 'Y');
}

/*
 * View Order function
 * -------------------------------------------------
 * Displays the current order in table format.
 */
void view_order(Order *o)
{
    if (o->count == 0)
    {
        printf("\n  -!- Order is empty -!-\n");
        return;
    }

    printf("\n|---------------- CURRENT ORDER %d ----------------|\n\n", o->orderNo);
    printf("#  Item                       Qty   Price    Total\n");
    printf("--------------------------------------------------\n");

    for (int i = 0; i < o->count; i++)
    {
        double total = o->items[i].price * o->items[i].qty;
        printf("%-2d %-24s %4d %8.2f %8.2f\n",
               i + 1,
               o->items[i].name,
               o->items[i].qty,
               o->items[i].price,
               total);
    }

    printf("--------------------------------------------------\n");
    printf("Subtotal : %.2f\n", o->subtotal);
    printf("Discount : %.2f\n", o->discount);
    printf("Tax      : %.2f\n", o->tax);
    printf("TOTAL    : %.2f\n\n", o->total);
}

/*
 * Remove Item function
 * -------------------------------------
 * Removes an item based on item number.
 */
void remove_item(Order *o)
{
    printf("\n|------------------ REMOVE ITEM ------------------|\n");
    if (o->count == 0)
    {
        printf("\n -!- Item has not yet added to current order -!-\n");
        return;
    }
    int index;
    while (1)
    {
        view_order(o);

        printf("> Enter item number to remove (0 to cancel) ");
        index = read_options(0, o->count);

        if (index == 0)
            return;
        else
            break;
    }
    printf("\n-*- Item %d removed successfully. -*-\n", index);
    index--;

    /* Shift remaining items */
    for (int i = index; i < o->count - 1; i++)
    {
        o->items[i] = o->items[i + 1];
    }

    o->count--;
    calculate_totals(o);
}

/*
 * Finalize Order function
 * -------------------------------------------------
 * Prints receipt, updates daily stats, resets order.
 *
 * PAYMENT METHODS:
 * 1 = Cash
 * 2 = Card
 * 3 = Mobile
 */
void finalize_order(Order *o)
{

    printf("\n|----------------- FINALIZE ORDER ----------------|\n\n");
    if (o->count == 0)
    {
        printf("  -!- Order is empty -!-\n");
        return;
    }

    printf("       1.Cash    |    2.Card    |    3.Mobile        \n\n");
    int payment = read_options(1, 3);

    printf("\n============= RECEIPT #%d =============\n\n", o->orderNo);

    for (int i = 0; i < o->count; i++)
    {
        double line = o->items[i].price * o->items[i].qty;
        printf("%-24s x%-3d %8.2f\n",
               o->items[i].name,
               o->items[i].qty,
               line);
        dailyItems += o->items[i].qty;
    }

    printf("---------------------------------------\n");
    printf("Subtotal : %.2f\n", o->subtotal);
    printf("Discount : %.2f\n", o->discount);
    printf("Tax      : %.2f\n", o->tax);
    printf("TOTAL    : %.2f\n", o->total);
    printf("Payment  : %s\n",
           payment == 1 ? "Cash" : payment == 2 ? "Card"
                                                : "Mobile");
    printf("======================================\n\n");

    dailyRevenue += o->total;
    dailyTax += o->tax;
    dailyDiscount += o->discount;
    dailyOrders++;

    o->orderNo++;
    reset_order(o);
}

/*
 * Daily Summary function
 * -------------------------------------------------
 * Displays summary of all orders processed today.
 */
void daily_summary(void)
{
    printf("\n===== DAILY SUMMARY ====\n");
    printf("Orders   : %d\n", dailyOrders);
    printf("Items    : %d\n", dailyItems);
    printf("Discount : %.2f\n", dailyDiscount);
    printf("Tax      : %.2f\n", dailyTax);
    printf("Revenue  : %.2f\n", dailyRevenue);
    printf("========================\n\n");
}

/*
 * Main method
 * -------------------------------------------------
 * SedApp POS Program entry point..
 */
int main(void)
{
    Order order;
    order.orderNo = 1;
    reset_order(&order);

    while (1)
    {
        printf("\n|------------------------------------------------------ SEDAPP POS MENU ------------------------------------------------------|\n");
        printf("|-----------------------------------------------------------------------------------------------------------------------------|\n\n");
        printf("  1. Add item    ");
        printf("|  2. Remove item  ");
        printf("|  3. View order  ");
        printf("|  4. Finalize order  ");
        printf("|  5. Clear order  ");
        printf("|  6. Daily summary  ");
        printf("|  7. Exit\n\n");

        int choice;
        choice = read_options(1, 7);

        switch (choice)
        {
        case 1:
            add_item(&order);
            break;
        case 2:
            remove_item(&order);
            break;
        case 3:
            view_order(&order);
            break;
        case 4:
            finalize_order(&order);
            break;
        case 5:
            reset_order(&order);
            printf("\n  -*- Order cleared -*-\n");
            break;
        case 6:
            daily_summary();
            break;
        case 7:
            daily_summary();
            printf("Program closed...\n");
            printf("\n|--------------------------------------------------------- THANK YOU ---------------------------------------------------------|\n");
            printf("|-----------------------------------------------------------------------------------------------------------------------------|\n\n");
            return 0;
        default:
            printf("Invalid menu choice.\n");
        }
    }
}
