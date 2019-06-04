// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_AUTOFILL_DIALOG_VIEW_IDS_H_
#define CHROME_BROWSER_UI_VIEWS_AUTOFILL_DIALOG_VIEW_IDS_H_

#include "components/autofill/core/browser/field_types.h"

// This defines an enumeration of IDs that can uniquely identify a view within
// the scope of the local and upload credit card save bubbles.

namespace autofill {

enum DialogViewId : int {
  VIEW_ID_NONE = 0,

  // The following are the important containing views of the bubble.
  MAIN_CONTENT_VIEW_LOCAL,   // The main content view, for a local save bubble
  MAIN_CONTENT_VIEW_UPLOAD,  // The main content view, for an upload save bubble
  FOOTNOTE_VIEW,             // Contains the legal messages for upload save
  SIGN_IN_PROMO_VIEW,        // Contains the sign-in promo view
  MANAGE_CARDS_VIEW,         // The manage cards view
  EXPIRATION_DATE_VIEW,      // Contains the dropdowns for expiration date

  // The sub-view that contains the sign-in button in the promo.
  SIGN_IN_VIEW,

  // The following are views::LabelButton objects (clickable).
  OK_BUTTON,            // Can say [Save], [Next], [Confirm],
                        // or [Done] depending on context
  CANCEL_BUTTON,        // Typically says [No thanks]
  CLOSE_BUTTON,         // Typically says [Close]
  MANAGE_CARDS_BUTTON,  // Typicall says [Manage cards]

  // The following are views::Link objects (clickable).
  LEARN_MORE_LINK,

  // The following are views::Textfield objects.
  CARDHOLDER_NAME_TEXTFIELD,  // Used for cardholder name entry/confirmation

  // The following are views::TooltipIcon objects.
  CARDHOLDER_NAME_TOOLTIP,  // Appears during cardholder name entry/confirmation

  // The following are views::Combobox objects.
  EXPIRATION_DATE_DROPBOX_MONTH,
  EXPIRATION_DATE_DROPBOX_YEAR,

  // The following are views::Label objects.
  EXPIRATION_DATE_LABEL,  // Appears during save offer bubble
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_VIEWS_AUTOFILL_DIALOG_VIEW_IDS_H_
