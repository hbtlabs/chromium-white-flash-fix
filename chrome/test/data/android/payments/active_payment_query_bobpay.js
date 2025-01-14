/*
 * Copyright 2016 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* global PaymentRequest:false */
/* global print:false */

/**
 * Checks for existence of Bob Pay.
 */
function buy() {  // eslint-disable-line no-unused-vars
  try {
    var request = new PaymentRequest(
        [{supportedMethods: ['https://bobpay.com']}],
        {total: {label: 'Total', amount: {currency: 'USD', value: '5.00'}}});
    request.canMakeActivePayment()
        .then(function(result) {
          print(result);
        })
        .catch(function(error) {
          print(error);
        });
  } catch (error) {
    print(error.message);
  }
}
