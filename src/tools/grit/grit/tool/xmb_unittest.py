#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''Unit tests for 'grit xmb' tool.'''

import os
import sys
if __name__ == '__main__':
  sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))

import unittest
import StringIO
import xml.sax

from grit import grd_reader
from grit import util
from grit.tool import xmb


class XmbUnittest(unittest.TestCase):
  def setUp(self):
    self.res_tree = grd_reader.Parse(
        StringIO.StringIO(u'''<?xml version="1.0" encoding="UTF-8"?>
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
        <release seq="3">
          <includes>
            <include type="gif" name="ID_LOGO" file="images/logo.gif" />
          </includes>
          <messages>
            <message name="GOOD" desc="sub" sub_variable="true">
              excellent
            </message>
            <message name="IDS_GREETING" desc="Printed to greet the currently logged in user">
              Hello <ph name="USERNAME">%s<ex>Joi</ex></ph>, are you doing [GOOD] today?
            </message>
            <message name="IDS_BONGOBINGO">
              Yibbee
            </message>
            <message name="IDS_UNICODE">
              Ol\xe1, \u4eca\u65e5\u306f! \U0001F60A
            </message>
          </messages>
          <structures>
            <structure type="dialog" name="IDD_SPACYBOX" encoding="utf-16" file="grit/testdata/klonk.rc" />
          </structures>
        </release>
      </grit>'''.encode('utf-8')), '.')
    self.xmb_file = StringIO.StringIO()

  def testNormalOutput(self):
    xmb.OutputXmb().Process(self.res_tree, self.xmb_file)
    output = self.xmb_file.getvalue().decode('utf-8')
    self.failUnless(output.count('Joi'))
    self.failUnless(output.count('Yibbee'))
    self.failUnless(output.count(u'Ol\xe1, \u4eca\u65e5\u306f! \U0001F60A'))

  def testLimitList(self):
    limit_file = StringIO.StringIO(
      'IDS_BONGOBINGO\nIDS_DOES_NOT_EXIST\nIDS_ALSO_DOES_NOT_EXIST')
    xmb.OutputXmb().Process(self.res_tree, self.xmb_file, limit_file, False)
    output = self.xmb_file.getvalue()
    self.failUnless(output.count('Yibbee'))
    self.failUnless(not output.count('Joi'))

  def testLimitGrd(self):
    limit_file = StringIO.StringIO('''<?xml version="1.0" encoding="UTF-8"?>
      <grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
        <release seq="3">
          <messages>
            <message name="IDS_GREETING" desc="Printed to greet the currently logged in user">
              Hello <ph name="USERNAME">%s<ex>Joi</ex></ph>, how are you doing today?
            </message>
          </messages>
        </release>
      </grit>''')
    tool = xmb.OutputXmb()
    class DummyOpts(object):
      extra_verbose = False
    tool.o = DummyOpts()
    tool.Process(self.res_tree, self.xmb_file, limit_file, True, dir='.')
    output = self.xmb_file.getvalue()
    self.failUnless(output.count('Joi'))
    self.failUnless(not output.count('Yibbee'))

  def testSubstitution(self):
    self.res_tree.SetOutputLanguage('en')
    os.chdir(util.PathFromRoot('.'))  # so it can find klonk.rc
    self.res_tree.RunGatherers()
    xmb.OutputXmb().Process(self.res_tree, self.xmb_file)
    output = self.xmb_file.getvalue()
    self.failUnless(output.count(
        '<ph name="GOOD_1"><ex>excellent</ex>[GOOD]</ph>'))

  def testLeadingTrailingWhitespace(self):
    # Regression test for problems outputting messages with leading or
    # trailing whitespace (these come in via structures only, as
    # message nodes already strip and store whitespace).
    self.res_tree.SetOutputLanguage('en')
    os.chdir(util.PathFromRoot('.'))  # so it can find klonk.rc
    self.res_tree.RunGatherers()
    xmb.OutputXmb().Process(self.res_tree, self.xmb_file)
    output = self.xmb_file.getvalue()
    self.failUnless(output.count('OK ? </msg>'))

  def testDisallowedChars(self):
    # Validate that the invalid unicode is not accepted. Since it's not valid,
    # we can't specify it in a string literal, so write as a byte sequence.
    bad_xml = StringIO.StringIO()
    bad_xml.write('''<?xml version="1.0" encoding="UTF-8"?>
      <grit latest_public_release="2" source_lang_id="en-US"
            current_release="3" base_dir=".">
        <release seq="3">
          <messages>
            <message name="ID_FOO">''')
    # UTF-8 corresponding to to \U00110000
    # http://apps.timwhitlock.info/unicode/inspect/hex/110000
    bad_xml.write(b'\xF4\x90\x80\x80')
    bad_xml.write('''</message>
          </messages>
        </release>
      </grit>''')
    bad_xml.seek(0)
    self.assertRaises(xml.sax.SAXParseException, grd_reader.Parse, bad_xml, '.')

if __name__ == '__main__':
  unittest.main()
