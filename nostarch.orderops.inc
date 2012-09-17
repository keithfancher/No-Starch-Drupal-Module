<?php
// $Id$


/*
 * Implemenation of hook_order_operations()
 *
 * This defines the functions that can be called from various views, including
 * the "order management" page. Makes processing orders way easier. Actual
 * implementation of these functions below.
 */
function nostarch_order_operations() {
  return array(
    'export_address_csv' => array(
      'label' => t("Export addresses as a CSV file"),
      'callback' => 'nostarch_orders_export_addresses_csv',
      'disabled' => TRUE,
    ),
    'export_orders_csv' => array(
      'label' => t("Export orders as a CSV file"),
      'callback' => 'nostarch_orders_export_orders_csv',
      'disabled' => TRUE,
    ),
    'set_orders_completed' => array(
      'label' => t("Set order status to Completed"),
      'callback' => 'nostarch_orders_set_status_completed',
      'disabled' => TRUE,
    ),
    'set_orders_pre_pre_ship' => array(
      'label' => t("Set order status to Pre-order pending"),
      'callback' => 'nostarch_orders_set_status_pre_pre_ship',
      'disabled' => TRUE,
    ),
    'set_orders_pre_ship' => array(
      'label' => t("Set order status to Pre-order awaiting shipment"),
      'callback' => 'nostarch_orders_set_status_pre_ship',
      'disabled' => TRUE,
    ),
    'set_orders_canceled' => array(
      'label' => t("Set order status to Canceled"),
      'callback' => 'nostarch_orders_set_status_canceled',
      'disabled' => TRUE,
    ),
  );
}


/*
 * Implementation of hook_action_info()
 *
 * Defines an action that adds ebooks to customers' accounts. It's annoying
 * that I have to do this, since I feel like this should be implemented in the
 * uc_file module. But it's not. Oh well!
 *
 * See also:
 *   nostarch_add_files_action_form
 *   nostarch_add_files_action_validate
 *   nostarch_add_files_action_submit
 *   nostarch_add_files_action
 */
function nostarch_action_info() {
  $info['nostarch_add_files_action'] = array(
    'type' => 'system',
    'description' => 'Add file(s) to users\' accounts',
    'configurable' => TRUE,
    'hooks' => array(
      'any' => TRUE,
    ),
  );

  return $info;
}


/*
 * The form that lets the user select the files to add to the specified
 * accounts.
 */
function nostarch_add_files_action_form($context) {

  // Rebuild the file list.
  uc_file_refresh();

  // Create the list of files able to be attached to this user.
  $available_files = array();

  // Build an array of available file ids and file names
  $files = db_query("SELECT * FROM {uc_files} ORDER BY filename ASC");
  while($file = db_fetch_object($files)) {
    if(substr($file->filename, -1) != '/' && substr($file->filename, -1) != '\\') {
      $available_files[$file->fid] = $file->filename;
    }
  }

  // Build the box to select which files to add
  if(count($available_files)) {
    $form['file_add'] = array(
      '#type' => 'select',
      '#multiple' => TRUE,
      '#size' => 15,
      '#title' => t('Add file(s)'),
      '#description' => t('Select a file to add as a download. Newly added files will inherit the settings at the !url.', array('!url' => l(t('Ubercart file product feature settings page'), 'admin/store/settings/products/edit/features'))),
      '#options' => $available_files,
//      '#tree' => TRUE, // this is annoying
    );
  }

  // TODO: A message that says: "Hey! No files!"

  return $form;
}


/*
 * Validate the form. This just screws things up when used through views bulk
 * operations, and running with an empty selection doesn't harm anything.
 * Leaving it commented out for now.
 */
function nostarch_add_files_action_validate($form, $form_state) {
//  if (!count($form_state['values']['file_add'])) {
//    form_set_error('file_add', 'Pick some files, man!');
//  }
}


/*
 * Submit the form values...
 */
function nostarch_add_files_action_submit($form, $form_state) {
  return array(
    'file_add' => $form_state['values']['file_add']
  );
}


/*
 * The Drupal action that adds files to selected users' accounts. This is where
 * the real magic happens.
 */
function nostarch_add_files_action($object, $context) {
  $user_id = $object->uid;
  $files_to_add = $context['file_add'];

  // load the user object
  $user = user_load(array('uid' => $user_id));

  // no limits!
  $limits['download_limit'] = 0;
  $limits['address_limit'] = 0;
  $limits['expiration_limit'] = 0;

  foreach($files_to_add as $file_id) {
    // Last value is $force_overwrite... keep TRUE if we want to overwrite this
    // assignment, reset the limits even if the user already has access?
    // Shouldn't matter in our case sense all the limits will always be 0.
    uc_file_user_renew($file_id, $user, NULL, $limits, TRUE);
  }
}


/*
 * Output order ID and shipping addresses for selected orders. Particularly
 * useful for making mailing labels when dealing with large amounts of orders.
 *
 * This spits out a CSV in sites/default/files/bulk_ops_output and exposes a
 * download link to it when finished. Note that this file is "safe" from prying
 * eyes due to the filename and location, but you should still delete it or
 * move it once it's retrieved.
 */
function nostarch_orders_export_addresses_csv($order_ids) {

  // open the file, give unique name w/ timestamp
  $fileName = "sites/default/files/bulk_ops_output/addresses-" . time() . ".csv";
  $fh = fopen($fileName, 'w') or die("oh junk guys, can't open the file");

  // write headings... used by ShipRush to make life easier
  $heading = "OrderID;ShippingCompany;ShippingContact;ShippingAddress1;ShippingAddress2;ShippingCity;ShippingState;ShippingCountry;ShippingPostalCode;TotalAmount;TotalWeight;Reference;TrackingNumber\n";
  fwrite($fh, $heading);

  foreach($order_ids as $order_id) {
    $order = uc_order_load($order_id);

    // uncomment when debugging
//    $results = print_r($order, true);
//    drupal_set_message($results);

    $company = $order->delivery_company;
    $name = $order->delivery_first_name . " " . $order->delivery_last_name;
    $address1 = $order->delivery_street1;
    $address2 = $order->delivery_street2;
    $city = $order->delivery_city;
    $state = uc_get_zone_code($order->delivery_zone); // states/provinces are stored by numeric code
    $postal_code = $order->delivery_postal_code;

    // getting the country is a bit more complex...
    $country_data = uc_get_country_data(array('country_id' => $order->delivery_country));
    $country = $country_data[0]['country_name'];

    // Output semicolon delimited, csv-style
    // Some of these ("TotalAmount" for instance) are mystery fields...
    $outputString = strtoupper(
                    $order_id . ";" . $company . ";" . $name . ";" .
                    $address1 . ";" . $address2 . ";" . $city . ";" .
                    $state . ";" . $country . ";" . $postal_code . ";" .
                    "" . ";" . "" . ";" . "Paperback book\n"
                    );

    // write one line
    fwrite($fh, $outputString);
  }

  // close the file
  fclose($fh);

  // download link
  $message = 'If all went well, you should be able to download the csv <a href="' . '/' . $fileName . '">here</a>.';
  drupal_set_message($message);
}


/*
 * Output some useful bullshit: order id, subtotal, coupon value, tax, total,
 * &c.
 *
 * This spits out a CSV in sites/default/files/bulk_ops_output and exposes a
 * download link to it when finished. Note that this file is "safe" from prying
 * eyes due to the filename and location, but you should still delete it or
 * move it once it's retrieved.
 */
function nostarch_orders_export_orders_csv($order_ids) {

  // open the file, give unique name w/ timestamp
  $fileName = "sites/default/files/bulk_ops_output/orders-" . time() . ".csv";
  $fh = fopen($fileName, 'w') or die("oh junk guys, can't open the file");

  // label the CSV columns
  $col_labels = "ORDER NUM;SUBTOTAL;COUPON AMT;TAX;SHIPPING;ORDER TOTAL;PRODUCTS\n";
  fwrite($fh, $col_labels);

  // loop through each order
  foreach($order_ids as $order_id) {
    $order = uc_order_load($order_id);

    // populate the ubercart token values
    $values = uc_order_token_values('order', $order);

    // used then stripped... stupid
    $context = array( 'location' => 'order-invoice-line-item',
                      'subject' => array(
                      'order' => $order,
                      ),
                    );

    // set default values for tax, shipping, and coupons... if the line items
    // exist, these values will be overwritten
    $tax = '$0.00';
    $shipping = '$0.00';
    $coupon = '$0.00';

    foreach ($order->line_items as $item) {

      // test for tax
      $tax_ret = stripos($item['title'], "tax");
      if($tax_ret !== false) { // there is tax!
        $tax = strip_tags(uc_price($item['amount'], $context));
      }

      // test for shipping
      $ship_ret = stripos($item['title'], "usps");
      if($ship_ret !== false) { // there is shipping!
        $shipping = strip_tags(uc_price($item['amount'], $context));
      }

      // test for coupon
      $coup_ret = stripos($item['title'], "coupon");
      if($coup_ret !== false) { // there is coupon!
        $coupon = strip_tags(uc_price($item['amount'], $context));
      }
    }

    // now loop through and grab products in order so we can output the SKUs
    $skus_in_order = ''; // a string, semicolon-delimited
    foreach($order->products as $product) {
      $skus_in_order = $skus_in_order . ';' . $product->model;
    }

    // output semicolon delimited, csv-style
    // ie: order id;subtotal;coupon;tax;shipping;order total
    $outputString = $values['order-id'] . ";" . $values['order-subtotal'] . ";" . $coupon . ";" . $tax . ";" . $shipping . ";" . $values['order-total'] . $skus_in_order . "\n";

    // write one line (order) to the output file
    fwrite($fh, $outputString);
  }

  // close the file
  fclose($fh);

  // download link
  $message = 'If all went well, you should be able to download the csv <a href="' . '/' . $fileName . '">here</a>.';
  drupal_set_message($message);
}


/*
 * Set order statuses in bulk
 *
 * TODO: make it easier to set arbitrary statuses rather than hard-coding them
 * like this.
 */
function nostarch_orders_set_status_completed($order_ids) {
  foreach($order_ids as $order_id) {
    uc_order_update_status($order_id, "completed");
  }
}
function nostarch_orders_set_status_pre_pre_ship($order_ids) {
  foreach($order_ids as $order_id) {
    uc_order_update_status($order_id, "pre-pre_ship");
  }
}
function nostarch_orders_set_status_pre_ship($order_ids) {
  foreach($order_ids as $order_id) {
    uc_order_update_status($order_id, "pre_ship");
  }
}
function nostarch_orders_set_status_canceled($order_ids) {
  foreach($order_ids as $order_id) {
    uc_order_update_status($order_id, "canceled");
  }
}
