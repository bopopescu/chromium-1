# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Creates a SVG graph of the view hierarchy, when stopped in a
# method of an object that inherits from views::View. Requires
# graphviz.
#
# For more info see
# chromium/src/+/HEAD/docs/graphical_debugging_aid_chromium_views.md
#
# To make this command available, add the following to your ~/.lldbinit:
# source {Path to SRC Root}/tools/gdbviewg.gdb
#
# Usage: type `viewg` at the GDB prompt, given the conditions above.


define viewg
  if $argc != 0
    echo Usage: viewg
  else
    set pagination off
    set print elements 0
    set logging off
    set logging file ~/state.dot
    set logging overwrite on
    set logging redirect on
    set logging on
    printf "%s\n", view::PrintViewGraph(this).c_str()
    set logging off
    shell dot -Tsvg -o ~/state.svg ~/state.dot
    set pagination on
  end
end