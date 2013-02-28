<?php
function transmute_webform_admin_settings()
{
	$profile = variable_get('transmute_webform_profile_fields', array());
	
	$form['transmute_webform_target_id'] = array(
		'#type'				=> 'textfield',
		'#title'			=> t('Target webform ID'),
		'#default_value'	=> variable_get('transmute_webform_target_id', ''),
		'#required'			=> TRUE,
		'#description'		=> t('The form_id of the webform being used as the volunter survey.  Usually webform_client_form_%node_id where %node_id is the node the form is created on.'),
	);
	
	$form['transmute_webform_target_node_id'] = array(
		'#type'				=> 'textfield',
		'#title'			=> t('Target webform Node ID'),
		'#default_value'	=> variable_get('transmute_webform_target_node_id', ''),
		'#required'			=> TRUE,
		'#description'		=> t('The node_id of the webform being used as the volunter survey.'),
	);
	
	$form['transmute_webform_profile_name_field_id'] = array(
		'#type'				=> 'textfield',
		'#title'			=> t('Name form id'),
		'#default_value'	=> $profile['name']['id'],
		'#required'			=> TRUE,
		'#description'		=> t('The component_id of the text box in our form used for the user\'s name.'),
	);
	
	$form['transmute_webform_profile_name_field_name'] = array(
		'#type'				=> 'textfield',
		'#title'			=> t('Name profile id'),
		'#default_value'	=> $profile['name']['name'],
		'#required'			=> TRUE,
		'#description'		=> t('The machine name of the profile field where the user\'s real name is stored.  Available under User Management -> Profiles.'),
	);
	
	$form['transmute_webform_profile_skills_field_id'] = array(
		'#type'				=> 'textfield',
		'#title'			=> t('Skills form id'),
		'#default_value'	=> $profile['skills']['id'],
		'#required'			=> TRUE,
		'#description'		=> t('The component_id of the text box in our form used for the user\'s skills.'),
	);
	
	$form['transmute_webform_profile_skills_field_name'] = array(
		'#type'				=> 'textfield',
		'#title'			=> t('Skills profile id'),
		'#default_value'	=> $profile['skills']['name'],
		'#required'			=> TRUE,
		'#description'		=> t('The machine name of the profile field where the user\'s skills are stored.  Available under User Management -> Profiles.'),
	);
	
	$form['transmute_webform_profile_name_field_category'] = array(
		'#type'				=> 'textfield',
		'#title'			=> t('Profile field category'),
		'#default_value'	=> $profile['category'],
		'#required'			=> TRUE,
		'#description'		=> t('The category of the profile field where the user\'s real name is stored.  Available under User Management -> Profiles.'),
	);
	
	$forwarding = variable_get('transmute_webform_ticket_forwarding', array());
	$form['tickets'] = array(
		'#type' => 'fieldset',
		'#title'	=> t('Forwarding'),
		'#collapsible'	=> TRUE,
		
		'enabled' => array(
			'#type' => 'checkbox',
			'#title'	=> t('Enable forwarding to another page after the form is submitted.'),
			'#default_value' => check_plain($forwarding['enabled']),
			'#return_value' => TRUE,
		),
		
		'url' => array(
			'#type'				=> 'textfield',
			'#title'			=> t('Forwarded URL'),
			'#default_value'	=> $forwarding['url'],
			'#description'		=> t('The url to send the user to after they submit the form.'),
		),
		
		'passthrough' => array(
			'#type' => 'checkbox',
			'#title'	=> t('Enable Pass-through Link'),
			'#default_value' => check_plain($forwarding['passthrough']),
			'#return_value' => TRUE,
			'#description' => t('Add a link at the bottom of the form that lets people continue to the forwarded URL without submitting the form'),
		),
		
		'passthrough_text' => array(
			'#type'				=> 'textfield',
			'#title'			=> t('Pass-through Text'),
			'#default_value'	=> $forwarding['text'],
			'#description'		=> t('The text to display on the pass-through link.'),
		),
	);
	
	$form['emails'] = array(
		'#type'		=> 'fieldset',
		'#title'	=> t('Email Settings'),
		'#description'	=> t('Comma separated list of emails to notify daily when the corresponding team gets more volunteers'),
		'#collapsible'	=> TRUE,
	);
	
	$result = db_query('SELECT c.cid, c.name, m.emails FROM {webform_component} c LEFT JOIN {transmute_webform_mail} m ON c.cid = m.component_id WHERE type = "select" AND c.nid = %d', variable_get('transmute_webform_target_node_id', 0));
	
	$options = array();
	while($data = db_fetch_array($result))
	{
		$form['emails'][$data['cid']] = array(
			'#type'			=> 'textfield',
			'#title'		=> t($data['name']),
			'#default_value'=> check_plain($data['emails']),
			'#size'			=> 60,
			'#maxlength'	=> 200,
		);
	}
	
	$form['submit'] = array(
		'#type'		=> 'submit',
		'#value'	=> t('Save'),
	);
	
	return $form;
} //function transmute_webform_admin_settings

function transmute_webform_admin_settings_validate($form, &$form_state)
{
	if($form_state['values']['enabled'] == TRUE && empty($form_state['values']['url']))
		form_set_error('url', t('You must set a forwarding url to enable forwarding.'));
		
	if($form_state['values']['passthrough'] == TRUE && empty($form_state['values']['passthrough_text']))
		form_set_error('passthrough_text', t('You must add text to enable the pass-through link.'));
		
	if(!is_numeric($form_state['values']['transmute_webform_target_node_id']))
		form_set_error('transmute_webform_target_node_id', t('Node ID must be a number'));
} //function transmute_webform_admin_settings_validate

function transmute_webform_admin_settings_submit($form, &$form_state)
{
	variable_set('transmute_webform_target_id', check_plain(trim($form_state['values']['transmute_webform_target_id'])));
	variable_set('transmute_webform_target_node_id', check_plain(trim($form_state['values']['transmute_webform_target_node_id'])));
	variable_set('transmute_webform_profile_fields', array(
		'name' => array(
			'id' => check_plain(trim($form_state['values']['transmute_webform_profile_name_field_id'])),
			'name' => check_plain(trim($form_state['values']['transmute_webform_profile_name_field_name'])),
		),
		'skills' => array(
			'id' => check_plain(trim($form_state['values']['transmute_webform_profile_skills_field_id'])),
			'name' => check_plain(trim($form_state['values']['transmute_webform_profile_skills_field_name'])),
		),
		'category' => check_plain(trim($form_state['values']['transmute_webform_profile_name_field_category']))
	));
	variable_set('transmute_webform_ticket_forwarding', array(
		'enabled' => $form_state['values']['enabled'] == TRUE ? TRUE : FALSE,
		'url' => check_plain(trim($form_state['values']['url'])),
		'passthrough' => $form_state['values']['passthrough'] == TRUE ? TRUE : FALSE,
		'text' => trim($form_state['values']['passthrough_text'])
	));
	
	foreach($form_state['values'] as $k => $v)
	{
		if(!is_numeric($k) || empty($v))
			continue;
			
		$result = db_query('SELECT COUNT(*) AS not_empty FROM {transmute_webform_mail} WHERE component_id = %d', $k);
		
		if(db_fetch_object($result)->not_empty == 1)
		{
			db_query('UPDATE {transmute_webform_mail} SET emails = "%s" WHERE component_id = %d', $v, $k);
		}
		else
		{
			db_query('INSERT INTO {transmute_webform_mail} (component_id, emails) VALUES (%d, "%s")', $k, $v);
		}
	}
} //function transmute_webform_admin_settings_submit