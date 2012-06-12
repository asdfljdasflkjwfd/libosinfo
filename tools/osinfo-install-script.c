/*
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * osinfo-install-script: generate an install script
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Authors:
 *   Daniel P. Berrange <berrange@redhat.com>
 */

#include <config.h>

#include <osinfo/osinfo.h>
#include <string.h>

static gchar *profile;

static OsinfoInstallConfig *config;

static gboolean handle_config(const gchar *option_name G_GNUC_UNUSED,
                          const gchar *value,
                          gpointer data G_GNUC_UNUSED,
                          GError **error)
{
    const gchar *val;
    gchar *key;
    gsize len;

    if (!(val = strchr(value, '='))) {
        g_set_error(error, 0, 0,
                    "Expected configuration key=value");
        return FALSE;
    }
    len = val - value;
    val++;
    key = g_strndup(value, len);

    osinfo_entity_set_param(OSINFO_ENTITY(config),
                            key,
                            val);
    g_free(key);
    return TRUE;
}


static GOptionEntry entries[] =
{
    { "profile", 'p', 0, G_OPTION_ARG_STRING, (void*)&profile,
      "Install script profile", NULL, },
    { "config", 'c', 0, G_OPTION_ARG_CALLBACK,
      handle_config,
      "Set configuration parameter", "key=value" },
    { NULL }
};


static OsinfoOs *find_os(OsinfoDb *db,
                         const char *idoruri)
{
    OsinfoOsList *oslist;
    OsinfoOsList *filteredList;
    OsinfoFilter *filter;
    OsinfoOs *os;

    os = osinfo_db_get_os(db, idoruri);

    if (os)
        return os;

    oslist = osinfo_db_get_os_list(db);
    filter = osinfo_filter_new();
    osinfo_filter_add_constraint(filter,
                                 OSINFO_PRODUCT_PROP_SHORT_ID,
                                 idoruri);

    filteredList = osinfo_oslist_new_filtered(oslist,
                                              filter);

    if (osinfo_list_get_length(OSINFO_LIST(filteredList)) > 0)
        os = OSINFO_OS(osinfo_list_get_nth(OSINFO_LIST(filteredList), 0));

    g_object_unref(oslist);
    g_object_unref(filteredList);
    g_object_unref(filter);

    return os;
}


static gboolean generate_script(OsinfoOs *os)
{
    OsinfoInstallScriptList *scripts = osinfo_os_get_install_script_list(os);
    OsinfoInstallScriptList *jeosScripts;
    OsinfoFilter *filter;
    OsinfoInstallScript *script;
    gboolean ret = FALSE;
    GError *error = NULL;
    gchar *data;

    filter = osinfo_filter_new();
    osinfo_filter_add_constraint(filter,
                                 OSINFO_INSTALL_SCRIPT_PROP_PROFILE,
                                 profile ? profile :
                                 OSINFO_INSTALL_SCRIPT_PROFILE_JEOS);

    jeosScripts = osinfo_install_scriptlist_new_filtered(scripts,
                                                         filter);
    if (osinfo_list_get_length(OSINFO_LIST(jeosScripts)) != 1) {
        g_printerr("Cannot find any install script for profile '%s'\n",
                   profile ? profile :
                   OSINFO_INSTALL_SCRIPT_PROFILE_JEOS);
        goto cleanup;
    }

    script = OSINFO_INSTALL_SCRIPT(osinfo_list_get_nth(OSINFO_LIST(jeosScripts), 0));
    if (!(data = osinfo_install_script_generate(script,
                                                os,
                                                config,
                                                NULL,
                                                &error))) {
        g_printerr("Unable to generate install script: %s\n",
                   error ? error->message : "unknown");
        goto cleanup;
    }

    g_print("%s\n", data);

    ret = TRUE;

 cleanup:
    g_free(data);
    g_object_unref(scripts);
    g_object_unref(jeosScripts);
    g_object_unref(filter);
    return ret;
}


gint main(gint argc, gchar **argv)
{
    GOptionContext *context;
    GError *error = NULL;
    OsinfoLoader *loader = NULL;
    OsinfoDb *db = NULL;
    OsinfoOs *os = NULL;
    gint ret = 0;

    g_type_init();

    config = osinfo_install_config_new("http://libosinfo.fedorahosted.org/config");

    context = g_option_context_new("- Generate an OS install script");
    /* FIXME: We don't have a gettext package to pass to this function. */
    g_option_context_add_main_entries(context, entries, NULL);
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_printerr("Error while parsing options: %s\n", error->message);
        g_printerr("%s\n", g_option_context_get_help(context, FALSE, NULL));

        ret = -1;
        goto EXIT;
    }

    if (argc < 2) {
        g_printerr("%s\n", g_option_context_get_help(context, FALSE, NULL));

        ret = -2;
        goto EXIT;
    }

    loader = osinfo_loader_new();
    osinfo_loader_process_default_path(loader, &error);
    if (error != NULL) {
        g_printerr("Error loading OS data: %s\n", error->message);

        ret = -3;
        goto EXIT;
    }

    db = osinfo_loader_get_db(loader);
    os = find_os(db, argv[1]);
    if (!os) {
        g_printerr("Error finding OS: %s\n", argv[1]);
        ret = -4;
        goto EXIT;
    }


    if (!generate_script(os)) {
        ret = -5;
        goto EXIT;
    }

EXIT:
    if (config)
        g_object_unref(config);
    g_clear_error(&error);
    g_clear_object(&loader);
    g_option_context_free(context);

    return ret;
}

/*
=pod

=head1 NAME

osinfo-install-script - generate a script for automated installation

=head1 SYNOPSIS

osinfo-install-script [OPTIONS...] OS-ID

=head1 DESCRIPTION

Generate a script suitable for performing an automated installation
of C<OS-ID>. C<OS-ID> should be a URI identifying the operating
system, or its short ID.

By default a script will be generated for a C<JEOS> style install.

=head1 OPTIONS

=over 8

=item B<--profile=NAME>

Choose the installation script profile. Defaults to C<jeos>, but
can also be C<desktop>, or a site specific profile name

=item B<--config=key=value>

Set the configuration parameter C<key> to C<value>.

=back

=head1 CONFIGURATION KEYS

The following configuration keys are available

=over 8

=item C<hardware-arch>

The hardware architecture

=item C<l10n-timezone>

The local timezone

=item C<l10n-keyboard>

The local keyboard layout

=item C<l10n-language>

The local language

=item C<admin-password>

The administrator password

=item C<user-password>

The user password

=item C<user-login>

The user login name

=item C<user-realname>

The user real name

=item C<user-autologin>

Whether to automatically login the user

=item C<user-admin>

Whether to give the user administrative privileges

=item C<reg-product-key>

The software registration key

=item C<reg-login>

The software registration user login

=item C<reg-password>

The software registration user password

=back

=head1 EXAMPLE USAGE

The following usage generates a Fedora 16 kickstart script

  # osinfo-install-script \
         --profile jeos \
         --config l10n-timezone=GMT \
         --config l10n-keyboard=uk \
         --config l10n-language=en_GB.UTF-8 \
         --config admin-password=123456 \
         --config user-login=berrange \
         --config user-password=123456 \
         --config user-realname="Daniel P Berrange" \
         fedora16

=head1 EXIT STATUS

The exit status will be 0 if an install script is generated,
or 1 on error

=head1 AUTHORS

Daniel P. Berrange <berrange@redhat.com>

=head1 COPYRIGHT

Copyright (C) 2012 Red Hat, Inc.

=head1 LICENSE

C<osinfo-install-script> is distributed under the termsof the GNU LGPL v2
license. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE

=cut
*/

/*
 * Local variables:
 *  indent-tabs-mode: nil
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 */