#!/bin/bash
export PATH=/ucrt64/bin:/usr/bin:$PATH
export MSYSTEM=UCRT64
export GDK_BACKEND=win32

echo "--- GTK Test ---"
echo "gcc: $(gcc --version | head -1)"
echo "pkg-config gtk: $(pkg-config --modversion gtk+-3.0)"

# Write a minimal test
cat > /tmp/test_gtk.c << 'EOF'
#include <gtk/gtk.h>
#include <stdio.h>
int main(int argc, char **argv) {
    fprintf(stderr, "Before gtk_init\n");
    fflush(stderr);
    gtk_init(&argc, &argv);
    fprintf(stderr, "After gtk_init - SUCCESS\n");
    fflush(stderr);
    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "GTK Test");
    gtk_window_set_default_size(GTK_WINDOW(win), 400, 200);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(win);
    gtk_main();
    return 0;
}
EOF

gcc /tmp/test_gtk.c -o /tmp/test_gtk $(pkg-config --cflags --libs gtk+-3.0) -lm
echo "Compile result: $?"
echo "Running..."
/tmp/test_gtk.exe >> /c/Users/Prakhyath\ L/.gemini/antigravity/scratch/gtk_test.log 2>&1 &
sleep 3
echo "GTK test launched, check gtk_test.log"
cat /c/Users/Prakhyath\ L/.gemini/antigravity/scratch/gtk_test.log
