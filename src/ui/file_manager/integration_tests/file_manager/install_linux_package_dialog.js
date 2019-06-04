// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
'use strict';

testcase.installLinuxPackageDialog = async function() {
  const fake = '#directory-tree .tree-item [root-type-icon="crostini"]';
  const real = '#directory-tree .tree-item [volume-type-icon="crostini"]';

  // The dialog has an INSTALL and OK button, both as .cr-dialog-ok, but only
  // one is visible at a time.
  const dialog = '#install-linux-package-dialog';
  const okButton = dialog + ' .cr-dialog-ok:not([hidden])';


  const {appId} = await setupAndWaitUntilReady(null, RootPath.DOWNLOADS, null);

  // Add entries to crostini volume, but do not mount.
  await addEntries(['crostini'], [ENTRIES.debPackage]);

  // Linux files fake root is shown.
  await remoteCall.waitForElement(appId, fake);

  // Mount crostini, and ensure real root and files are shown.
  remoteCall.callRemoteTestUtil('fakeMouseClick', appId, [fake]);
  await remoteCall.waitForElement(appId, real);
  const files = TestEntryInfo.getExpectedRows([ENTRIES.debPackage]);
  await remoteCall.waitForFiles(appId, files);

  // Open the deb package.
  await remoteCall.callRemoteTestUtil(
      'openFile', appId, [ENTRIES.debPackage.targetPath]);

  // Ensure package install dialog is shown.
  await remoteCall.waitForElement(appId, dialog);
  await repeatUntil(async () => {
    const elements = await remoteCall.callRemoteTestUtil(
        'queryAllElements', appId, ['.install-linux-package-details-frame']);
    // The details are in separate divs on multiple lines, which the test api
    // returns as a single string. These values come from
    // fake_cicerone_client.cc.
    return elements[0] &&
        elements[0].text ==
            ('Details' +
             'Application: Fake Package' +
             'Version: 1.0' +
             'Description: A package that is fake') ||
        pending('Waiting for installation to start.');
  });

  // Begin installation.
  await remoteCall.callRemoteTestUtil('fakeMouseClick', appId, [okButton]);

  // Wait for the installation to start (under test, we use a fake D-Bus
  // client, so it doesn't actually install anything).
  await repeatUntil(async () => {
    const elements = await remoteCall.callRemoteTestUtil(
        'queryAllElements', appId, ['.cr-dialog-text']);
    return elements[0] &&
        elements[0].text == 'Installation successfully started.' ||
        pending('Waiting for installation to start.');
  });

  // Dismiss dialog
  await remoteCall.callRemoteTestUtil('fakeMouseClick', appId, [okButton]);

  // Ensure dialog closes
  await remoteCall.waitForElementLost(appId, dialog);
};
