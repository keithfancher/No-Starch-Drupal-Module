<?php
// $Id$


define("PREORDER_TAXONOMY", 18);


/*
 * Takes a node (note -- NOT a product, but the product's node) and returns
 * TRUE if it's a preorder and FALSE otherwise.
 */
function nostarch_is_preorder($product_node) {
  return isset($product_node->taxonomy[PREORDER_TAXONOMY]);
}


/*
 * Checks if the current cart contains any pre-orders... useful if we don't
 * have an order object handy. Can pass it an array of items, or it will just
 * use whatever's in the current shopping cart if no argument passed.
 */
function nostarch_cart_contains_preorders($items=NULL) {
  if(!$items) {
    $items = uc_cart_get_contents();
  }

  // note: $items is an array even with only a single product in cart
  if(is_array($items)) {
    foreach($items as $item) {
      // is it a pre-order?
      $product_node = node_load($item->nid);
      if(nostarch_is_preorder($product_node)) {
        return TRUE;
      }
    }
  }

  return FALSE;
}


/*
 * Checks if the current cart is a valid pre-order order... useful if we don't
 * have an order object handy. Can pass it an array of items, or it will just
 * use whatever's in the current shopping cart if no argument passed.
 */
function nostarch_cart_is_valid_preorder($items=NULL) {
  if(!$items) {
    $items = uc_cart_get_contents();
  }

  $cart_contains_pre_orders = FALSE;
  $product_count = 0;

  // note: $items is an array even with only a single product in cart
  if(is_array($items)) {
    foreach($items as $item) {
      // don't count coupons or other line items
      if($item->module == 'uc_product') {
        $product_count++;
      }

      // is it a pre-order?
      $product_node = node_load($item->nid);
      if(nostarch_is_preorder($product_node)) {
        $cart_contains_pre_orders = TRUE;
      }
    }
  }

  if($product_count == 1 && $cart_contains_pre_orders) {
    return TRUE;
  }
  else {
    return FALSE;
  }
}


/*
 * Returns TRUE if the given order contains one or more pre-order items, FALSE
 * otherwise.
 */
function nostarch_order_contains_preorders($order) {
  // note: $order->products is an array even with only one product
  if(is_array($order->products)) {
    foreach($order->products as $prod) {
      $prod_node = node_load($prod->nid);
      if(nostarch_is_preorder($prod_node)) {
        return TRUE;
      }
    }
  }

  return FALSE;
}


/*
 * Takes an order object and returns TRUE if it's a valid pre-order... right
 * now that means there is only one product and that product is a pre-order.
 */
function nostarch_is_valid_preorder($order) {
  // using count() instead of $order->product_count so each product is counted
  // only once, regardless of the quantity of that product in the order
  if(count($order->products) == 1) {
    $prod = $order->products[0]; // load the one product
    if(nostarch_is_preorder(node_load($prod->nid))) { // it's a pre-order
      return TRUE; // one product, a preorder
    }
  }

  return FALSE;
}


/*
 * Shows a message about... INVALID PREORDERS!
 */
function nostarch_invalid_preorder_message() {
  drupal_set_message('<strong>Please note that, due to technical limitations with our payment gateway, each pre-order item must be purchased separately from other items, in its own order. We apologize for any inconvenience this might cause&mdash;we\'re working on getting it fixed!</strong>', 'error', FALSE);
}
