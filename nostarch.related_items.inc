<?php
// $Id$


/**
 * Implementation of hook_block().
 * @param string $op one of "list", "view", "save" and "configure"
 * @param integer $delta code to identify the block
 * @param array $edit only for "save" operation
*/
function nostarch_block($op = 'list', $delta = 0, $edit = array()) {
  $block = array();

  switch($op) {
    case 'list':
      $block[0]['info'] = 'You might also like...';
      break;

    case 'view':
      // only show this block on product nodes
      $nid = nostarch_get_current_node_id();
      if(nostarch_node_is_product($nid)) {
        // TODO: make themeable, configure number of related items to show, &c.
        $block_content = '';
        $block_content .= '<div class="related-items">';
        $block_content .= nostarch_show_related_nodes($nid, 5);
        $block_content .= '</div>';

        $block['subject'] = 'You might also like...';
        $block['content'] = $block_content;
      }
      break;

    case 'save':
      break;

    case 'configure':
      break;
  }

  return $block;
}


/*
 * Shows a list of "related" nodes... in this case, products that are in the
 * same "catalog" taxonomy (or taxonomies) as the specified node.
 *
 * The idea is that this can be easily stuck in a block and shown on product
 * pages with something like this:
 *
 * nostarch_show_related_nodes(nostarch_get_current_node_id(), 5);
 */
function nostarch_show_related_nodes($nid, $num_nodes) {
  // "catalog" is vocabulary number 2
  $terms = taxonomy_node_get_terms_by_vocabulary(node_load($nid), 2);
  $sql = "SELECT DISTINCT node.title, node.nid, node.title FROM node, term_node where (node.nid = term_node.nid) and (node.status = 1) AND (";
  $count = 0;

  // If the node doesn't have any taxonomy terms associated with it, just show
  // some programming books! (That's taxonomy 2.) We have quite a few
  // intentionally "orphaned" books, and this prevents any SQL unhappiness if a
  // user views those pages.
  if(empty($terms)) {
    $sql = $sql . "(term_node.tid = 2)";
  }
  else {
    // Loop through a product's terms, grab "associated" items
    foreach($terms as $term){
      if($count == 0){
        $sql = $sql . "(term_node.tid = " .$term->tid." )";
      }
      else {
        $sql = $sql . " OR (term_node.tid = " .$term->tid." )";
      }
      $count++;
    }
  }

  $end = ") AND (node.nid != $nid) ORDER BY RAND() LIMIT $num_nodes";
  $sql = $sql .$end;

  // query!
  $result = db_query($sql);

  // show some images
  return nostarch_node_image_list($result);
}


/*
 * Similar to Drupal's stock node_title_list() function, but displays images
 * instead. Like node_title_list(), takes a database query result. TODO:
 * themeable, &c.
 */
function nostarch_node_image_list($result) {
  $output = '';

  while($node = db_fetch_object($result)) {
    $node = node_load($node->nid); // load the *actual* node
    $path = '/sites/default/files/imagecache/product/';
    $image = $node->field_image_cache[0]['filename'];
    $output .= '<a href="' . $node->path . '"><img src="' . $path . $image . '" class="related-items-image" title="' . $node->title . '"/></a><br />';
  }

  return $output;
}


/*
 * Returns node id for currently viewed node
 */
function nostarch_get_current_node_id() {
  if(arg(0) == 'node' && is_numeric(arg(1))) {
    return arg(1);
  }
}


/*
 * Returns TRUE if $nid belongs to a product node, FALSE otherwise.
 */
function nostarch_node_is_product($nid) {
  $node = node_load($nid);
  if($node->type == 'product') {
    return TRUE;
  }

  return FALSE;
}
