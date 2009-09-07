/*
 *      lxrandr.c - Easy-to-use XRandR GUI frontend for LXDE project
 *
 *      Copyright (C) 2008 Hong Jen Yee(PCMan) <pcman.tw@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <locale.h>
#include <stdio.h>
#include <string.h>

typedef struct _Monitor
{
    char* name;
    GSList* mode_lines;
    short active_mode;
    short active_rate;
    short pref_mode;
    short pref_rate;

    GtkCheckButton* enable;
    GtkComboBox* res_combo;
    GtkComboBox* rate_combo;
}Monitor;

static GSList* monitors = NULL;
static Monitor* LVDS = NULL;

static GtkWidget* dlg = NULL;

static void monitor_free( Monitor* m )
{
    g_free( m->name );
    g_slist_free( m->mode_lines );
    g_free( m );
}

static const char* get_human_readable_name( Monitor* m )
{
    if( m == LVDS )
        return _("Laptop LCD Monitor");
    else if( g_str_has_prefix( m->name, "VGA" ) || g_str_has_prefix( m->name, "Analog" ) )
        return _( LVDS ? "External VGA Monitor" : "VGA Monitor");
    else if( g_str_has_prefix( m->name, "DVI" ) || g_str_has_prefix(m->name, "TMDS") || g_str_has_prefix(m->name, "Digital") || g_str_has_prefix(m->name, "LVDS") )
        return _( LVDS ? "External DVI Monitor" : "DVI Monitor");
    else if( g_str_has_prefix( m->name, "TV" ) || g_str_has_prefix(m->name, "S-Video") )
        return _("TV");
    else if( strcmp( m->name, "default" ) == 0 )
        return _( "Default Monitor");

    return m->name;
}

static gboolean get_xrandr_info()
{
    GRegex* regex;
    GMatchInfo* match;
    int status;
    char* output = NULL;
    char* ori_locale;

    ori_locale = g_strdup( setlocale(LC_ALL, "") );

    // set locale to "C" temporarily to guarantee English output of xrandr
    setlocale(LC_ALL, "C");

    if( ! g_spawn_command_line_sync( "xrandr", &output, NULL, &status, NULL ) || status )
    {
        g_free( output );
        setlocale( LC_ALL, ori_locale );
        g_free( ori_locale );
        return FALSE;
    }

    regex = g_regex_new( "([a-zA-Z]+[-0-9]*) +connected .*((\n +[0-9]+x[0-9]+[^\n]+)+)",
                         0, 0, NULL );
    if( g_regex_match( regex, output, 0, &match ) )
    {
        do {
            Monitor* m = g_new0( Monitor, 1 );
            char *modes = g_match_info_fetch( match, 2 );
            char **lines, **line;
            int imode = 0;

            m->active_mode = m->active_rate = -1;
            m->pref_mode = m->pref_rate = -1;
            m->name = g_match_info_fetch( match, 1 );

            // check if this is the built-in LCD of laptop
            if( ! LVDS && (strcmp( m->name, "LVDS" ) == 0 || strcmp( m->name, "PANEL" ) == 0) )
                LVDS = m;

            lines = g_strsplit( modes, "\n", -1 );
            for( line = lines; *line; ++line )
            {
                char* str = strtok( *line, " " );
                int irate = 0;
                GPtrArray* strv;
                if( ! str )
                    continue;
                strv = g_ptr_array_sized_new(8);
                g_ptr_array_add( strv, g_strdup(str) );
                while( str = strtok( NULL, " ") )
                {
                    if( *str )
                    {
                        char *star, *plus;
                        str = g_strdup( str );

                        // sometimes, + goes after a space
                        if( 0 == strcmp( str, "+" ) )
                            --irate;
                        else
                            g_ptr_array_add( strv, str );

                        if( star = strchr( str, '*' ) )
                        {
                            m->active_mode = imode;
                            m->active_rate = irate;
                        }
                        if( plus = strchr( str, '+' ) )
                        {
                            m->pref_mode = imode;
                            m->pref_rate = irate;
                        }
                        if( star )
                            *star = '\0';
                        if( plus )
                            *plus = '\0';
                        ++irate;
                    }
                }
                g_ptr_array_add( strv, NULL );
                m->mode_lines = g_slist_append( m->mode_lines, g_ptr_array_free( strv, FALSE ) );
                ++imode;
            }
            g_strfreev( lines );
            g_free( modes );
            monitors = g_slist_prepend( monitors, m );
        }while( g_match_info_next( match, NULL ) );

        g_match_info_free( match );
    }
    g_regex_unref( regex );

    // restore the original locale
    setlocale( LC_ALL, ori_locale );
    g_free( ori_locale );

    return TRUE;
}

static void on_res_sel_changed( GtkComboBox* cb, Monitor* m )
{
    char** rate;
    int sel = gtk_combo_box_get_active( cb );
    char** mode_line = g_slist_nth_data( m->mode_lines, sel - 1 );
    gtk_list_store_clear( GTK_LIST_STORE(gtk_combo_box_get_model( m->rate_combo )) );

    gtk_combo_box_append_text( m->rate_combo, _("Auto") );
    if( sel >= 0 && mode_line && *mode_line )
    {
        for( rate = mode_line + 1; *rate; ++rate )
        {
            gtk_combo_box_append_text( m->rate_combo, *rate );
        }
    }
    gtk_combo_box_set_active( m->rate_combo, 0 );
}

static void open_url( GtkDialog* dlg, const char* url, gpointer data )
{
    /* FIXME: */
}

static void on_about( GtkButton* btn, gpointer parent )
{
    GtkWidget * about_dlg;
    const gchar *authors[] =
    {
        "洪任諭 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>",
        NULL
    };
    /* TRANSLATORS: Replace mw string with your names, one name per line. */
    gchar *translators = _( "translator-credits" );

//    gtk_about_dialog_set_url_hook( open_url, NULL, NULL);

    about_dlg = gtk_about_dialog_new ();

    gtk_container_set_border_width ( ( GtkContainer*)about_dlg , 2 );
    gtk_about_dialog_set_version ( (GtkAboutDialog*)about_dlg, VERSION );
    gtk_about_dialog_set_name ( (GtkAboutDialog*)about_dlg, _( "LXRandR" ) );
    //gtk_about_dialog_set_logo( (GtkAboutDialog*)about_dlg, gdk_pixbuf_new_from_file(  PACKAGE_DATA_DIR"/pixmaps/lxrandr.png", NULL ) );
    gtk_about_dialog_set_copyright ( (GtkAboutDialog*)about_dlg, _( "Copyright (C) 2008" ) );
    gtk_about_dialog_set_comments ( (GtkAboutDialog*)about_dlg, _( "Monitor configuration tool for LXDE" ) );
    gtk_about_dialog_set_license ( (GtkAboutDialog*)about_dlg, "This program is free software; you can redistribute it and/or\nmodify it under the terms of the GNU General Public License\nas published by the Free Software Foundation; either version 2\nof the License, or (at your option) any later version.\n\nmw program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License\nalong with mw program; if not, write to the Free Software\nFoundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA." );
    gtk_about_dialog_set_website ( (GtkAboutDialog*)about_dlg, "http://lxde.org/" );
    gtk_about_dialog_set_authors ( (GtkAboutDialog*)about_dlg, authors );
    gtk_about_dialog_set_translator_credits ( (GtkAboutDialog*)about_dlg, translators );

    gtk_window_set_transient_for( (GtkWindow*)about_dlg, (GtkWindow*)parent );
    gtk_dialog_run( ( GtkDialog*)about_dlg );
    gtk_widget_destroy( about_dlg );
}

static void set_xrandr_info()
{
    GSList* l;
    GString *cmd = g_string_sized_new( 1024 );

    g_string_assign( cmd, "xrandr" );

    for( l = monitors; l; l = l->next )
    {
        Monitor* m = (Monitor*)l->data;
        g_string_append( cmd, " --output " );
        g_string_append( cmd, m->name );
        g_string_append_c( cmd, ' ' );

        // if the monitor is turned on
        if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(m->enable) ) )
        {
            int sel_res = gtk_combo_box_get_active( m->res_combo );
            int sel_rate = gtk_combo_box_get_active( m->rate_combo );

            if( sel_res < 1 ) // auto resolution
            {
                g_string_append( cmd, "--auto" );
            }
            else
            {
                g_string_append( cmd, "--mode " );
                ++sel_res;  // the fist item in the combo box is "Auto", indecis of resolutions are 1, 2, 3...
                g_string_append( cmd, gtk_combo_box_get_active_text(m->res_combo) );

                if( sel_rate >= 1 ) // not auto refresh rate
                {
                    g_string_append( cmd, " --rate " );
                    g_string_append( cmd, gtk_combo_box_get_active_text(m->rate_combo) );
                }
            }

            g_string_append( cmd, "" );

        }
        else    // turn off
            g_string_append( cmd, "--off" );
    }

    g_spawn_command_line_sync( cmd->str, NULL, NULL, NULL, NULL );
    g_string_free( cmd, TRUE );
}

static void choose_max_resolution( Monitor* m )
{
    if( gtk_tree_model_iter_n_children( gtk_combo_box_get_model(m->res_combo), NULL ) > 1 )
        gtk_combo_box_set_active( m->res_combo, 1 );
}

static void on_quick_option( GtkButton* btn, gpointer data )
{
    GSList* l;
    int option = GPOINTER_TO_INT(data);
    switch( option )
    {
    case 1: // turn on both
        for( l = monitors; l; l = l->next )
        {
            Monitor* m = (Monitor*)l->data;
            choose_max_resolution( m );
            gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(m->enable), TRUE );
        }
        break;
    case 2: // external monitor only
        for( l = monitors; l; l = l->next )
        {
            Monitor* m = (Monitor*)l->data;
            choose_max_resolution( m );
            gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(m->enable), m != LVDS );
        }
        break;
    case 3: // laptop panel - LVDS only
        for( l = monitors; l; l = l->next )
        {
            Monitor* m = (Monitor*)l->data;
            choose_max_resolution( m );
            gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(m->enable), m == LVDS );
        }
        break;
    default:
        return;
    }
    gtk_dialog_response( GTK_DIALOG(dlg), GTK_RESPONSE_OK );
//    set_xrandr_info();
}

static void on_response( GtkDialog* dialog, int response, gpointer user_data )
{
    if( response == GTK_RESPONSE_OK )
    {
        GtkWidget* msg;
        GSList* l;
        for( l = monitors; l; l = l->next )
        {
            Monitor* m = (Monitor*)l->data;
            if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(m->enable) ) )
                return;
        }

        msg = gtk_message_dialog_new( GTK_WINDOW(dialog), 0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                     _("You cannot turn off all monitors. Otherwise, you will not be able to turn them on again since this tool is not accessable without monitor.") );
        gtk_dialog_run( GTK_DIALOG(msg) );
        gtk_widget_destroy( msg );

        // block the response
        g_signal_stop_emission_by_name( dialog, "response" );
    }
}

int main(int argc, char** argv)
{
    GtkWidget *notebook, *vbox, *frame, *label, *hbox, *check, *btn;
    GSList* l;

#ifdef ENABLE_NLS
	bindtextdomain ( GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR );
	bind_textdomain_codeset ( GETTEXT_PACKAGE, "UTF-8" );
	textdomain ( GETTEXT_PACKAGE );
#endif

    gtk_init( &argc, &argv );

    if( ! get_xrandr_info() )
    {
        dlg = gtk_message_dialog_new( NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                      _("Unable to get monitor information!"), NULL );
        gtk_dialog_run( (GtkDialog*)dlg );
        gtk_widget_destroy( dlg );
        return 1;
    }

    dlg = gtk_dialog_new_with_buttons( _("Display Settings"), NULL,
                                       GTK_DIALOG_NO_SEPARATOR,
                                       GTK_STOCK_OK, GTK_RESPONSE_OK,
                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL );
    g_signal_connect( dlg, "response", G_CALLBACK(on_response), NULL );
    gtk_container_set_border_width( GTK_CONTAINER(dlg), 8 );
    gtk_dialog_set_alternative_button_order( GTK_DIALOG(dlg), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1 );

    btn = gtk_button_new_from_stock( GTK_STOCK_ABOUT );
    gtk_box_pack_start( GTK_BOX(GTK_DIALOG(dlg)->action_area), btn, FALSE, TRUE, 0 );
    gtk_button_box_set_child_secondary( GTK_BUTTON_BOX(GTK_DIALOG(dlg)->action_area), btn, TRUE );
    g_signal_connect( btn, "clicked", G_CALLBACK(on_about), dlg );

    notebook = gtk_notebook_new();
    gtk_box_pack_start( GTK_BOX( GTK_DIALOG(dlg)->vbox ), notebook, TRUE, TRUE, 2 );

    // If this is a laptop and there is an external monitor, offer quick options
    if( LVDS && g_slist_length( monitors ) == 2 )
    {
        vbox = gtk_vbox_new( FALSE, 4 );
        gtk_container_set_border_width( GTK_CONTAINER(vbox), 8 );

        btn = gtk_button_new_with_label( _("Show the same screen on both laptop LCD and external monitor") );
        g_signal_connect( btn, "clicked", G_CALLBACK(on_quick_option), GINT_TO_POINTER(1) );
        gtk_box_pack_start( GTK_BOX(vbox), btn, FALSE, TRUE , 4);

        btn = gtk_button_new_with_label( _("Turn off laptop LCD and use external monitor only") );
        g_signal_connect( btn, "clicked", G_CALLBACK(on_quick_option), GINT_TO_POINTER(2) );
        gtk_box_pack_start( GTK_BOX(vbox), btn, FALSE, TRUE , 4);

        btn = gtk_button_new_with_label( _("Turn off external monitor and use laptop LCD only") );
        g_signal_connect( btn, "clicked", G_CALLBACK(on_quick_option), GINT_TO_POINTER(3) );
        gtk_box_pack_start( GTK_BOX(vbox), btn, FALSE, TRUE , 4);

        gtk_notebook_append_page( GTK_NOTEBOOK(notebook), vbox, gtk_label_new( _("Quick Options") ) );
    }
    else
    {
        gtk_notebook_set_show_tabs( GTK_NOTEBOOK(notebook), FALSE );
    }

    vbox = gtk_vbox_new( FALSE, 4 );
    gtk_container_set_border_width( GTK_CONTAINER(vbox), 8 );
    gtk_notebook_append_page( GTK_NOTEBOOK(notebook), vbox, gtk_label_new(_("Advanced")) );

    label = gtk_label_new("");
    gtk_misc_set_alignment( GTK_MISC(label), 0.0, 0.5 );
    gtk_label_set_markup( GTK_LABEL(label), ngettext( "The following monitor is detected:",
                                    "The following monitors are detected:",
                                    g_slist_length(monitors) ) );
    gtk_box_pack_start( GTK_BOX(vbox), label, FALSE, TRUE, 2 );

    int i;
    for( l = monitors, i = 0; l; l = l->next, ++i )
    {
        Monitor* m = (Monitor*)l->data;
        GSList* mode_line;

        frame = gtk_frame_new( get_human_readable_name(m) );
        gtk_box_pack_start( GTK_BOX(vbox), frame, FALSE, TRUE, 2 );

        hbox = gtk_hbox_new( FALSE, 4 );
        gtk_container_set_border_width( GTK_CONTAINER(hbox), 4 );
        gtk_container_add( GTK_CONTAINER(frame), hbox );

        check = gtk_check_button_new_with_label( _("Turn On") );
        m->enable = GTK_CHECK_BUTTON(check);

        // turn off screen is not allowed since there should be at least one monitor available.
        if( g_slist_length( monitors ) == 1 )
            gtk_widget_set_sensitive( GTK_WIDGET(m->enable), FALSE );

        gtk_box_pack_start( GTK_BOX(hbox), check, FALSE, TRUE, 6 );
        if( m->active_mode >= 0 )
            gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(check), TRUE );

        label = gtk_label_new( _("Resolution:") );
        gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, TRUE, 2 );

        m->res_combo = GTK_COMBO_BOX(gtk_combo_box_new_text());
        g_signal_connect( m->res_combo, "changed", G_CALLBACK(on_res_sel_changed), m );
        gtk_box_pack_start( GTK_BOX(hbox), GTK_WIDGET(m->res_combo), FALSE, TRUE, 2 );

        label = gtk_label_new( _("Refresh Rate:") );
        gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, TRUE, 2 );

        m->rate_combo = GTK_COMBO_BOX(gtk_combo_box_new_text());
        gtk_box_pack_start( GTK_BOX(hbox), GTK_WIDGET(m->rate_combo), FALSE, TRUE, 2 );

        gtk_combo_box_append_text( m->res_combo, _("Auto") );
        for( mode_line = m->mode_lines; mode_line; mode_line = mode_line->next )
        {
            char** strv = (char**)mode_line->data;
            gtk_combo_box_append_text( m->res_combo, strv[0] );
        }

        gtk_combo_box_set_active( m->res_combo, m->active_mode + 1 );
        gtk_combo_box_set_active( m->rate_combo, m->active_rate + 1 );
    }

    gtk_widget_show_all( dlg );

    if( gtk_dialog_run( (GtkDialog*)dlg ) == GTK_RESPONSE_OK )
        set_xrandr_info();

    gtk_widget_destroy( dlg );

	return 0;
}
