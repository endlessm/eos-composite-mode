
/* This file is hackily part of ecm-daemon.c */

static const char *
get_path (EcmState state)
{
  switch (state) {
  case ECM_STATE_HDMI: return "/com/endlessm/CompositeMode/hdmi/";
  case ECM_STATE_COMPOSITE: return "/com/endlessm/CompositeMode/composite/";
  default: g_assert_not_reached ();
  }
}

static GSettings *
get_state_settings (EcmState state)
{
  return g_settings_new_with_path ("com.endlessm.CompositeMode.mode", get_path (state));
}

static void
ecm_settings_persist (EcmState old_state)
{
  g_autoptr (GSettings) background_settings = g_settings_new ("org.gnome.desktop.background");
  g_autoptr (GSettings) interface_settings = g_settings_new ("org.gnome.desktop.interface");

  /* Persist old values in the old state */
  g_autoptr (GSettings) old_state_settings = get_state_settings (old_state);
  g_settings_set_string (old_state_settings, "picture-options", g_settings_get_string (background_settings, "picture-options"));
  g_settings_set_string (old_state_settings, "primary-color", g_settings_get_string (background_settings, "primary-color"));
  g_settings_set_string (old_state_settings, "secondary-color", g_settings_get_string (background_settings, "secondary-color"));
  g_settings_set_double (old_state_settings, "text-scaling-factor", g_settings_get_double (interface_settings, "text-scaling-factor"));
}

static void
ecm_settings_load (EcmState new_state)
{
  g_autoptr (GSettings) background_settings = g_settings_new ("org.gnome.desktop.background");
  g_autoptr (GSettings) interface_settings = g_settings_new ("org.gnome.desktop.interface");

  /* Now load values from the new state into the session */
  g_autoptr (GSettings) new_state_settings = get_state_settings (new_state);

  if (!g_settings_get_boolean (new_state_settings, "initialized"))
    {
      /* If we've never used composite before, then load the default values for that. */
      if (new_state == ECM_STATE_COMPOSITE)
        {
          g_settings_set_string (new_state_settings, "picture-options", "none");
          g_settings_set_string (new_state_settings, "primary-color", "#7a8aa2");
          g_settings_set_string (new_state_settings, "secondary-color", "#7a8aa2");
          g_settings_set_double (new_state_settings, "text-scaling-factor", 2.2);
        }
      else
        ecm_settings_persist (new_state);

      g_settings_set_boolean (new_state_settings, "initialized", TRUE);
    }

  g_settings_set_string (background_settings, "picture-options", g_settings_get_string (new_state_settings, "picture-options"));
  g_settings_set_string (background_settings, "primary-color", g_settings_get_string (new_state_settings, "primary-color"));
  g_settings_set_string (background_settings, "secondary-color", g_settings_get_string (new_state_settings, "secondary-color"));
  g_settings_set_double (interface_settings, "text-scaling-factor", g_settings_get_double (new_state_settings, "text-scaling-factor"));
}

static void
ecm_settings_reset (void)
{
  {
    g_autoptr (GSettings) background_settings = g_settings_new ("org.gnome.desktop.background");
    g_settings_reset (background_settings, "picture-options");
    g_settings_reset (background_settings, "primary-color");
    g_settings_reset (background_settings, "secondary-color");
  }

  {
    g_autoptr (GSettings) interface_settings = g_settings_new ("org.gnome.desktop.interface");
    g_settings_reset (interface_settings, "text-scaling-factor");
  }
}
