/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * script-command.c: script commands
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-command.h"
#include "script-action.h"
#include "script-buffer.h"
#include "script-config.h"
#include "script-repo.h"


/*
 * script_command_action: run action
 */

void
script_command_action (struct t_gui_buffer *buffer, const char *action,
                       const char *action_with_args, int need_repository)
{
    struct t_repo_script *ptr_script;
    char str_action[4096];

    if (action_with_args)
    {
        /* action with arguments on command line */
        script_action_schedule (action_with_args, need_repository, 0);
    }
    else if (script_buffer && (buffer == script_buffer))
    {
        /* action on current line of script buffer */
        if ((weechat_strcasecmp (action, "show") == 0)
            && script_buffer_detail_script)
        {
            /* if detail on script is displayed, back to list */
            snprintf (str_action, sizeof (str_action),
                      "-q %s",
                      action);
            script_action_schedule (str_action, need_repository, 1);
        }
        else
        {
            /* if list is displayed, execute action on script */
            if (!script_buffer_detail_script)
            {
                ptr_script = script_repo_search_displayed_by_number (script_buffer_selected_line);
                if (ptr_script)
                {
                    snprintf (str_action, sizeof (str_action),
                              "-q %s %s",
                              action,
                              ptr_script->name_with_extension);
                    script_action_schedule (str_action, need_repository, 1);
                }
            }
        }
    }
}

/*
 * script_command_script: command to manage scripts
 */

int
script_command_script (void *data, struct t_gui_buffer *buffer, int argc,
                       char **argv, char **argv_eol)
{
    char *error;
    long value;
    int line;

    /* make C compiler happy */
    (void) data;

    if (argc == 1)
    {
        script_action_schedule ("buffer", 1, 0);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcasecmp (argv[1], "list") == 0)
    {
        script_action_schedule ("list", 1, 0);
        return WEECHAT_RC_OK;
    }

    if ((weechat_strcasecmp (argv[1], "load") == 0)
        || (weechat_strcasecmp (argv[1], "unload") == 0)
        || (weechat_strcasecmp (argv[1], "reload") == 0))
    {
        script_command_action (buffer,
                               argv[1],
                               (argc > 2) ? argv_eol[1] : NULL,
                               0);
        return WEECHAT_RC_OK;
    }

    if ((weechat_strcasecmp (argv[1], "install") == 0)
        || (weechat_strcasecmp (argv[1], "remove") == 0)
        || (weechat_strcasecmp (argv[1], "hold") == 0)
        || (weechat_strcasecmp (argv[1], "show") == 0))
    {
        script_command_action (buffer,
                               argv[1],
                               (argc > 2) ? argv_eol[1] : NULL,
                               1);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcasecmp (argv[1], "upgrade") == 0)
    {
        script_action_schedule ("upgrade", 1, 0);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcasecmp (argv[1], "update") == 0)
    {
        script_repo_file_update (0);
        return WEECHAT_RC_OK;
    }

    if (!script_buffer)
        script_buffer_open ();

    if (script_buffer)
    {
        weechat_buffer_set (script_buffer, "display", "1");

        if (argc > 1)
        {
            if (!script_buffer_detail_script
                && (script_buffer_selected_line >= 0)
                && (script_repo_count_displayed > 0))
            {
                if (strcmp (argv[1], "up") == 0)
                {
                    value = 1;
                    if (argc > 2)
                    {
                        error = NULL;
                        value = strtol (argv[2], &error, 10);
                        if (!error || error[0])
                            value = 1;
                    }
                    line = script_buffer_selected_line - value;
                    if (line < 0)
                        line = 0;
                    if (line != script_buffer_selected_line)
                    {
                        script_buffer_set_current_line (line);
                        script_buffer_check_line_outside_window ();
                    }
                    return WEECHAT_RC_OK;
                }
                else if (strcmp (argv[1], "down") == 0)
                {
                    value = 1;
                    if (argc > 2)
                    {
                        error = NULL;
                        value = strtol (argv[2], &error, 10);
                        if (!error || error[0])
                            value = 1;
                    }
                    line = script_buffer_selected_line + value;
                    if (line >= script_repo_count_displayed)
                        line = script_repo_count_displayed - 1;
                    if (line != script_buffer_selected_line)
                    {
                        script_buffer_set_current_line (line);
                        script_buffer_check_line_outside_window ();
                    }
                    return WEECHAT_RC_OK;
                }
            }
        }
    }

    script_buffer_refresh (0);

    return WEECHAT_RC_OK;
}

/*
 * scrit_command_init: init script commands (create hooks)
 */

void
script_command_init ()
{
    weechat_hook_command ("script",
                          N_("WeeChat scripts manager"),
                          N_("list || show <script>"
                             " || load|unload|reload <script> [<script>...]"
                             " || install|remove|hold <script> [<script>...]"
                             " || upgrade || update"),
                          N_("    list: list loaded scripts (all languages)\n"
                             "    show: show detailed info about a script\n"
                             "    load: load script(s)\n"
                             "  unload: unload script(s)\n"
                             "  reload: reload script(s)\n"
                             " install: install/upgrade script(s)\n"
                             "  remove: remove script(s)\n"
                             "    hold: hold/unhold script(s) (a script held "
                             "will not be upgraded any more and cannot be "
                             "removed)\n"
                             " upgrade: upgrade all installed scripts which "
                             "are obsolete (new version available)\n"
                             "  update: update local scripts cache\n\n"
                             "Without argument, this command opens a buffer "
                             "with list of scripts.\n\n"
                             "On script buffer, the possible status for each "
                             "script are:\n"
                             "  * i a H r N\n"
                             "  | | | | | |\n"
                             "  | | | | | obsolete (new version available)\n"
                             "  | | | | running (loaded)\n"
                             "  | | | held\n"
                             "  | | autoloaded\n"
                             "  | installed\n"
                             "  popular script\n\n"
                             "Keys on script buffer:\n"
                             "  alt+i    install script\n"
                             "  alt+r    remove script\n"
                             "  alt+l    load script\n"
                             "  alt+u    unload script\n"
                             "  alt+h    (un)hold script\n\n"
                             "Input allowed on script buffer:\n"
                             "  q        close buffer\n"
                             "  r        refresh buffer\n"
                             "  s:x,y    sort buffer using keys x and y (see /help "
                             "script.look.sort)\n"
                             "  s:       reset sort (use default sort)\n"
                             "  word(s)  filter scripts: search word(s) in "
                             "scripts (description, tags, ...)\n"
                             "  *        remove filter\n\n"
                             "Examples:\n"
                             "  /script install iset.pl buffers.pl\n"
                             "  /script remove iset.pl\n"
                             "  /script hold urlserver.py\n"
                             "  /script reload urlserver\n"
                             "  /script upgrade"),
                          "list"
                          " || show %(script_scripts)"
                          " || load %(script_files)|%*"
                          " || unload %(python_script)|%(perl_script)|"
                          "%(ruby_script)|%(tcl_script)|%(lua_script)|"
                          "%(guile_script)|%*"
                          " || reload %(python_script)|%(perl_script)|"
                          "%(ruby_script)|%(tcl_script)|%(lua_script)|"
                          "%(guile_script)|%*"
                          " || install %(script_scripts)|%*"
                          " || remove %(script_scripts_installed)|%*"
                          " || hold %(script_scripts)|%*"
                          " || update"
                          " || upgrade",
                          &script_command_script, NULL);
}
