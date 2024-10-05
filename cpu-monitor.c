#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Global widgets for CPU label and process list
GtkWidget *cpu_label;
GtkWidget *process_list;

// Function to calculate CPU usage
void get_cpu_usage(long *idle, long *total) {
    FILE *fp;
    char buffer[1024];
    long user, nice, system, idle_time, iowait, irq, softirq, steal;

    fp=fopen("/proc/stat", "r");
    if (fp == NULL) {
        perror("Error opening /proc/stat");
        exit(1);
    }

    // Read the first line of /proc/stat
    fgets(buffer, sizeof(buffer), fp);
    sscanf(buffer, "cpu  %ld %ld %ld %ld %ld %ld %ld %ld",
           &user, &nice, &system, &idle_time, &iowait, &irq, &softirq, &steal);

    *idle=idle_time + iowait;
    *total=user + nice + system + idle_time + iowait + irq + softirq + steal;

    fclose(fp);
}

// Function to update CPU usage label
gboolean update_cpu_usage(gpointer data) {
    static long prev_idle=0, prev_total=0;
    long idle, total;
    char cpu_usage_str[64];
    double cpu_usage;

    // Get current CPU usage
    get_cpu_usage(&idle, &total);

    // Calculate CPU usage percentage
    long idle_diff=idle - prev_idle;
    long total_diff=total - prev_total;
    cpu_usage=(1.0 - ((double)idle_diff / total_diff)) * 100;

    // Format and update label
    snprintf(cpu_usage_str, sizeof(cpu_usage_str), "CPU Usage: %.2f%%", cpu_usage);
    gtk_label_set_text(GTK_LABEL(cpu_label), cpu_usage_str);

    // Store previous values for next calculation
    prev_idle=idle;
    prev_total=total;

    return TRUE;  // Keep the timeout active
}

// Function to terminate a process based on the PID
void terminate_process(GtkButton *button, gpointer entry) {
    const char *pid_str=gtk_entry_get_text(GTK_ENTRY(entry));
    int pid=atoi(pid_str);

    if (pid > 0) {
        char command[64];
        snprintf(command, sizeof(command), "kill -9 %d", pid);
        system(command);
        gtk_entry_set_text(GTK_ENTRY(entry), "");
        g_print("Terminated process with PID: %d\n", pid);
    }
}

// Function to get the top CPU-consuming processes
void update_process_list() {
    char buffer[1024];
    FILE *fp=popen("ps -eo pid,comm,%cpu --sort=-%cpu | head -n 10", "r");
    if (fp == NULL) {
        perror("Error opening process list");
        return;
    }

    // Clear existing text
    gtk_label_set_text(GTK_LABEL(process_list), "");

    // Read the process list and update the label
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        // Append each line to the process list label
        const char *old_text=gtk_label_get_text(GTK_LABEL(process_list));
        char *new_text=g_strdup_printf("%s%s", old_text, buffer);
        gtk_label_set_text(GTK_LABEL(process_list), new_text);
        g_free(new_text);
    }

    pclose(fp);
}

// Function to show the About dialog
void show_about_dialog(GtkButton *button, gpointer window) {
    GtkWidget *dialog=gtk_message_dialog_new(GTK_WINDOW(window),
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "Author: Jay Mee @ J~Net 2024\nVersion: 1.0");
    gtk_window_set_title(GTK_WINDOW(dialog), "About CPU Monitor");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Build GUI
int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *entry;
    GtkWidget *terminate_button;
    GtkWidget *refresh_button;
    GtkWidget *about_button;

    gtk_init(&argc, &argv);

    // Create window
    window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "CPU Monitor");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    // Create grid
    grid=gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    // CPU usage label
    cpu_label=gtk_label_new("CPU Usage: Calculating...");
    gtk_grid_attach(GTK_GRID(grid), cpu_label, 0, 0, 2, 1);

    // Entry field to input PID for termination
    entry=gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter PID to terminate");
    gtk_grid_attach(GTK_GRID(grid), entry, 0, 1, 1, 1);

    // Terminate process button
    terminate_button=gtk_button_new_with_label("Terminate Process");
    g_signal_connect(terminate_button, "clicked", G_CALLBACK(terminate_process), entry);
    gtk_grid_attach(GTK_GRID(grid), terminate_button, 1, 1, 1, 1);

    // Process list label
    process_list=gtk_label_new("Loading process list...");
    gtk_grid_attach(GTK_GRID(grid), process_list, 0, 2, 2, 1);

    // Refresh process list button
    refresh_button=gtk_button_new_with_label("Refresh Processes");
    g_signal_connect(refresh_button, "clicked", G_CALLBACK(update_process_list), NULL);
    gtk_grid_attach(GTK_GRID(grid), refresh_button, 0, 3, 2, 1);

    // About button
    about_button=gtk_button_new_with_label("About");
    g_signal_connect(about_button, "clicked", G_CALLBACK(show_about_dialog), window);
    gtk_grid_attach(GTK_GRID(grid), about_button, 0, 4, 2, 1);

    // Signal to close window
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Update CPU usage every second
    g_timeout_add(1000, update_cpu_usage, NULL);

    // Initially update process list
    update_process_list();

    // Show everything
    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}

