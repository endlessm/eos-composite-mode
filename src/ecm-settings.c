
/* This file is hackily part of ecm-daemon.c */

#define COMPOSITE_MODE_RELOCATABLE_SCHEMA "com.endlessm.CompositeMode.mode"
#define GNOME_DESKTOP_INTERFACE_SCHEMA "org.gnome.desktop.interface"

#define INITIALIZED_KEY "initialized"
#define TEXT_SCALING_FACTOR_KEY "text-scaling-factor"

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
  return g_settings_new_with_path (COMPOSITE_MODE_RELOCATABLE_SCHEMA, get_path (state));
}

static void
ecm_settings_persist (EcmState old_state)
{
  g_autoptr (GSettings) interface_settings = g_settings_new (GNOME_DESKTOP_INTERFACE_SCHEMA);

  /* Persist old values in the old state */
  g_autoptr (GSettings) old_state_settings = get_state_settings (old_state);
  g_settings_set_double (old_state_settings,
                         TEXT_SCALING_FACTOR_KEY,
                         g_settings_get_double (interface_settings, TEXT_SCALING_FACTOR_KEY));
}

static void
ecm_settings_load (EcmState new_state)
{
  g_autoptr (GSettings) interface_settings = g_settings_new (GNOME_DESKTOP_INTERFACE_SCHEMA);

  /* Now load values from the new state into the session */
  g_autoptr (GSettings) new_state_settings = get_state_settings (new_state);

  if (!g_settings_get_boolean (new_state_settings, INITIALIZED_KEY))
    {
      /* If we've never used composite before, then load the default values for that. */
      if (new_state == ECM_STATE_COMPOSITE)
        {
          g_settings_set_double (new_state_settings, TEXT_SCALING_FACTOR_KEY, 1.1);
        }
      else
        ecm_settings_persist (new_state);

      g_settings_set_boolean (new_state_settings, INITIALIZED_KEY, TRUE);
    }

  g_settings_set_double (interface_settings,
                         TEXT_SCALING_FACTOR_KEY,
                         g_settings_get_double (new_state_settings, TEXT_SCALING_FACTOR_KEY));
}

static void
ecm_settings_reset (void)
{
  {
    g_autoptr (GSettings) interface_settings = g_settings_new (GNOME_DESKTOP_INTERFACE_SCHEMA);
    g_settings_reset (interface_settings, TEXT_SCALING_FACTOR_KEY);
  }
}
