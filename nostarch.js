(function($) {

  /*
   * Add colorbox to classed links for handy pseudo dialogs.
   */
  Drupal.behaviors.addColorBoxLink = function(context) {
    // .colorbox doesn't always exist, prevent exceptions
    if($.isFunction($.colorbox)) {
      $('a.colorbox-link').colorbox({iframe:true, innerWidth:425, innerHeight:200});
    }
  };

})(jQuery);
