"""
  Copyright (c) 2010 Julien Lavergne <gilir@ubuntu.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
"""

import os
import apport.hookutils

def add_info(report):
    # Attach xrandr report
    report["xrandr_output"] = apport.hookutils.command_output(["udisks"])
    # If an autostart file exist, report it.
    if os.path.exists(os.path.join(os.path.expanduser("~/.config/autostart","lxrandr-autostart.desktop"))):
        report["autostart_desktop"] = apport.hookutils.read_file(os.path.expanduser("~/.config/autostart","lxrandr-autostart.desktop"))
