
/* This file is hackily part of ecm-daemon.c */

typedef struct
{
  char *picture_options;
  char *primary_color;
  char *secondary_color;
  double text_scaling_factor;
} EcmSettings;

static const EcmSettings composite_settings = {
  .picture_options = "none",
  .primary_color = "#7a8aa2",
  .secondary_color = "#7a8aa2",
  .text_scaling_factor = 2.2,
};

static void
ecm_settings_clear (EcmSettings *settings)
{
  g_clear_pointer (&settings->picture_options, g_free);
  g_clear_pointer (&settings->primary_color, g_free);
  g_clear_pointer (&settings->secondary_color, g_free);
  settings->text_scaling_factor = 1.0;
}

static void
ecm_settings_load_from_gsettings (EcmSettings *settings)
{
  ecm_settings_clear (settings);

  {
    g_autoptr (GSettings) desktop_settings = g_settings_new ("org.gnome.desktop.background");
    settings->picture_options = g_settings_get_string (desktop_settings, "picture-options");
    settings->primary_color = g_settings_get_string (desktop_settings, "primary-color");
    settings->secondary_color = g_settings_get_string (desktop_settings, "secondary-color");
  }

  {
    g_autoptr (GSettings) interface_settings = g_settings_new ("org.gnome.desktop.interface");
    settings->text_scaling_factor = g_settings_get_double (interface_settings, "text-scaling-factor");
  }
}

static void
ecm_settings_save_to_gsettings (const EcmSettings *settings)
{
  {
    g_autoptr (GSettings) desktop_settings = g_settings_new ("org.gnome.desktop.background");
    g_settings_set_string (desktop_settings, "picture-options", settings->picture_options);
    g_settings_set_string (desktop_settings, "primary-color", settings->primary_color);
    g_settings_set_string (desktop_settings, "secondary-color", settings->secondary_color);
  }

  {
    g_autoptr (GSettings) interface_settings = g_settings_new ("org.gnome.desktop.interface");
    g_settings_set_double (interface_settings, "text-scaling-factor", settings->text_scaling_factor);
  }
}
