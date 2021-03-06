<?php
/**
 * @file
 * Contains download interface of this module.
 */

/**
 * Function callback: construct export data form.
 */
function rawpheno_download($form, &$form_state) {
  // Attach CSS and JavaScript
  $path = drupal_get_path('module', 'rawpheno') . '/theme/';
  $form['#attached']['css'] = array($path . 'css/rawpheno.download.style.css');

  // Navigation button. Related page of download page is rawdata/summary page.
  $form['page_button'] = array(
    '#type' => 'markup',
    '#markup' => t('View Summary &#10095;'),
  );

  // PROJECT SELECT BOX.
  if (isset($form_state['values']['sel_project'])) {
    // Project selected.
    $project_selected = $form_state['values']['sel_project'];
  }

  $sql = "SELECT DISTINCT t1.project_id, t1.name
          FROM {project} AS t1 RIGHT JOIN pheno_plant_project AS t2 USING(project_id)
          ORDER BY t1.project_id DESC";

  $opt_project = chado_query($sql)
    ->fetchAllKeyed();

  if (count($opt_project) <= 0) {
    return $form;
  }

  // AJAX wrapper.
  // Main wrapper
  $form['ajax_container'] = array(
    '#type' => 'markup',
    '#prefix' => '<div id="download-ajax-wrapper">',
    '#suffix' => '</div>',
  );

  // This a hidden field containing all project id.
  // This field will allow callback functions to get all project ids which is
  // the equivalent of the option select all project from the project select box.
  $form['ajax_container']['txt_project'] = array(
    '#type' => 'hidden',
    '#value' => implode(',', array_keys($opt_project)),
  );

  $form['ajax_container']['sel_project'] = array(
    '#type' => 'select',
    '#title' => t('Project'),
    '#options' => array(0 => 'All Projects') + $opt_project,
    '#multiple' => FALSE,
    '#id' => 'download-sel-project',
    '#ajax' => array(
      'event' => 'change',
      'callback' => 'rawpheno_download_get_locations_traits',
      'wrapper' => 'download-ajax-wrapper',
      'progress' => array('type' => '', 'message' => '')
    ),
  );

  // This will reset the project select box on load and page refresh.
  drupal_add_js('jQuery(document).ready(function() {
    jQuery("#download-sel-project").val(0);
  })', 'inline');

  // Define the project ids required by the next field.
  if (isset($project_selected)) {
    // When a project is selected.
    // 0 is the first option in the select box and translates to all projects.
    $project_id = ($project_selected == 0) ? array_keys($opt_project) : $project_selected;
  }
  else {
    // No project select. This is the defaul select all project.
    $project_id = array_keys($opt_project);
  }

  // LOCATION SELECT BOX.
  $sql = "SELECT DISTINCT value, value AS prj_location
          FROM pheno_plantprop
          WHERE
            type_id = (SELECT cvterm_id FROM {cvterm} WHERE name = 'Location' LIMIT 1) AND
            plant_id IN (SELECT plant_id FROM pheno_plant_project WHERE project_id IN (:project_id))
          ORDER BY value ASC";

  $opt_location = chado_query($sql, array(':project_id' => $project_id))
    ->fetchAllKeyed();

  $form['ajax_container']['sel_location'] = array(
    '#type' => 'select',
    '#title' => t('Location'),
    '#options' => $opt_location,
    '#multiple' => TRUE,
    '#size' => 7,
    '#id' => 'download-sel-location',
    '#ajax' => array(
      'event' => 'change',
      'callback' => 'rawpheno_download_get_traits',
      'wrapper' => 'download-ajax-wrapper-traits',
      'progress' => array('type' => '', 'message' => '')
    ),
  );

  $form['ajax_container']['chk_select_all_locations'] = array(
    '#title' => t('Select all Locations'),
    '#type' => 'checkbox',
    '#default_value' => 0,
    '#ajax' => array(
      'event' => 'change',
      'callback' => 'rawpheno_download_get_locations_traits',
      'wrapper' => 'download-ajax-wrapper',
      'progress' => array('type' => '', 'message' => '')
    ),
    '#id' => 'chk-select-all-locations',
  );

  $location_id = $opt_location;

  // TRAITS.
  // Select traits wrapper.
  $form['ajax_container']['ajax_container_traits'] = array(
    '#type' => 'markup',
    '#prefix' => '<div id="download-ajax-wrapper-traits">',
    '#suffix' => '</div>',
  );

  // Get traits given a location and project.
  $opt_trait = rawpheno_download_load_traits($location_id, $project_id);

  $form['ajax_container']['ajax_container_traits']['sel_trait'] = array(
    '#type' => 'select',
    '#title' => t('@trait_count Traits available', array('@trait_count' => count($opt_trait))),
    '#options' => $opt_trait,
    '#multiple' => TRUE,
    '#size' => 15,
    '#id' => 'download-sel-trait',
  );

  $form['ajax_container']['chk_select_all_traits'] = array(
    '#title' => t('Select all Traits'),
    '#type' => 'checkbox',
    '#default_value' => 0,
    '#id' => 'chk-select-all-traits',
  );

  $form['div_buttons'] = array(
    '#prefix' => '<div id="div-button">',
    '#suffix' => '</div>',
  );

  $form['div_buttons']['chk_rfriendly'] = array(
    '#title' => t('<img src="@img" alt="Make R Friendly" title="Make R Friendly" align="absmiddle" /> (Make R Friendly)', array('@img' => '../../' . $path . 'img/r.gif')),
    '#type' => 'checkbox',
    '#default_value' => 0,
  );

  $form['div_buttons']['download_submit_download'] = array(
    '#type' => 'submit',
    '#value' => 'Download',
  );

  $form['#attached']['js']  = array($path . 'js/rawpheno.download.script.js');

  return $form;
}


/**
 * Function callback: AJAX update location and traits select boxes when project is selected.
 */
function rawpheno_download_get_locations_traits($form, $form_state) {
  return $form['ajax_container'];
}


/**
 * Function callback: AJAX update trait select box.
 */
function rawpheno_download_get_traits($form, $form_state) {
  $location = $form_state['values']['sel_location'];
  $project = $form_state['values']['sel_project'];

  // Determine if the selected project is all project.
  if ($project == 0) {
    // Yes, then read the value of the hidden field containing project ids.
    $t = $form_state['values']['txt_project'];
    $project = explode(',', $t);
  }

  // Get all traits given a location and project.
  $opt_trait = rawpheno_download_load_traits($location, $project);

  // Update the #options value of select a trait select box.
  $form['ajax_container']['ajax_container_traits']['sel_trait']['#options'] = $opt_trait;
  // Update the title.
  $form['ajax_container']['ajax_container_traits']['sel_trait']['#title'] = t('@count_trait Traits available', array('@count_trait' => count($opt_trait)));

  return $form['ajax_container']['ajax_container_traits'];
}


/**
 * Implements hook_form_submit().
 *
 * Generate a comma separated values (csv) file based on the location and trait set selected.
 */
function rawpheno_download_submit($form, &$form_state) {
  // Project select field.
  // Project by default is 0 - all projects then we want all project id field.
  // This is field is never an array.
  $prj = $form_state['values']['sel_project'];
  $all_prj = $form_state['values']['txt_project'];
  $prj = ($prj == 0) ? $all_prj : $prj;

  // Location select field.
  // Location select field is an empty array - all locations.
  // Otherwise, it will be an associative array where location is both key and value.
  // Convert this to comma separated string when there's anything else set to 0 - for all locations.
  $loc = $form_state['values']['sel_location'];
  $loc = (count($loc) > 0) ? implode(',', $loc) : 0;

  // Trait select field.
  // Trait select field is an empty array - all traits.
  // Otherwise, it will be an associative array where trait is both key and value.
  // Convert this to comma separated string when there's anything else set to 0 - for all traits.
  $trt = $form_state['values']['sel_trait'];
  $trt = (count($trt) > 0) ? implode(',', $trt) : 0;

  // Lastly, if user wants the R version.
  $rvr = $form_state['values']['chk_rfriendly'];

  // Contain all query parameters/string into one string.
  // Decode first when reading this string using base64_decode() function.
  $url = base64_encode('p=' . $prj . '&l=' . $loc . '&t=' . $trt . '&r=' . $rvr);

  // Format url for redirect.
  $form_state['redirect'] = array(
    '/phenotypes/raw/csv',
    array(
      'query' => array(
        'code' => $url,
      ),
    ),
  );
}
