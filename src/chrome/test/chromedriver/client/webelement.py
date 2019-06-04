# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from command_executor import Command


class WebElement(object):
  """Represents an HTML element."""
  def __init__(self, chromedriver, id_):
    self._chromedriver = chromedriver
    self._id = id_

  def _Execute(self, command, params=None):
    if params is None:
      params = {}
    params['id'] = self._id;
    return self._chromedriver.ExecuteCommand(command, params)

  def FindElement(self, strategy, target):
    return self._Execute(
        Command.FIND_CHILD_ELEMENT, {'using': strategy, 'value': target})

  def FindElements(self, strategy, target):
    return self._Execute(
        Command.FIND_CHILD_ELEMENTS, {'using': strategy, 'value': target})

  def GetText(self):
    return self._Execute(Command.GET_ELEMENT_TEXT)

  def GetAttribute(self,name):
    return self._Execute(Command.GET_ELEMENT_ATTRIBUTE, {'name': name})

  def GetProperty(self,name):
    return self._Execute(Command.GET_ELEMENT_PROPERTY, {'name': name})

  def HoverOver(self):
    self._Execute(Command.HOVER_OVER_ELEMENT)

  def Click(self):
    self._Execute(Command.CLICK_ELEMENT)

  def SingleTap(self):
    self._Execute(Command.TOUCH_SINGLE_TAP)

  def DoubleTap(self):
    self._Execute(Command.TOUCH_DOUBLE_TAP)

  def LongPress(self):
    self._Execute(Command.TOUCH_LONG_PRESS)

  def Clear(self):
    self._Execute(Command.CLEAR_ELEMENT)

  def SendKeys(self, *values):
    typing = []
    for value in values:
      if isinstance(value, int):
        value = str(value)
      for i in range(len(value)):
        typing.append(value[i])
    self._Execute(Command.SEND_KEYS_TO_ELEMENT, {'value': typing})

  def SendKeysW3c(self, text):
    self._Execute(Command.SEND_KEYS_TO_ELEMENT, {'text': text})

  def GetLocation(self):
    return self._Execute(Command.GET_ELEMENT_LOCATION)

  def GetRect(self):
    return self._Execute(Command.GET_ELEMENT_RECT)

  def IsDisplayed(self):
    return self._Execute(Command.IS_ELEMENT_DISPLAYED)

  def TakeElementScreenshot(self):
    return self._Execute(Command.ELEMENT_SCREENSHOT)
