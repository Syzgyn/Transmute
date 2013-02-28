<?php
function transmute_admin_list()
{
	$query_result = db_query("SELECT name, team_id FROM {transmute_teams}");
	
	while ($result = db_fetch_object($query_result))
	{ 
		$form[$result->team_id]['name'] = array('#value' => check_plain($result->name));
		$form[$result->team_id]['edit'] = array('#value' => l(t('Edit'), 'admin/settings/transmute/'. $result->team_id));
		$form[$result->team_id]['shifts'] = array('#value' => l(t('Shifts'), 'admin/settings/transmute/'. $result->team_id . '/shifts'));
		$form[$result->team_id]['delete'] = array('#value' => l(t('Delete'), 'admin/settings/transmute/'. $result->team_id . '/delete'));
	}
	
	return $form;
} //function transmute_admin_list

function theme_transmute_admin_list($form)
 {
	$rows = array();
	
	foreach (element_children($form) as $item)
	{
		if(!is_numeric($item)) //Use this to prevent extra rows from being added because of system form stuff added.
			continue;
	
		$row = &$form[$item];
			
		$rows[] = array(
			drupal_render($row['name']),
			drupal_render($row['edit']),
			drupal_render($row['shifts']),
			drupal_render($row['delete'])
		);
		unset($form[$name]);
	}
	$header = array(t('Name'), array('data' => t('Operations'), 'colspan' => 3));
	$output = theme('table', $header, $rows);
	$output .= drupal_render($form);

	return $output;
} //function theme_transmute_admin_list

function transmute_admin_team($form_state, $team = NULL)
{
	if(!isset($team))
	{
		$team = array('name' => '', 'desc' => '');
		drupal_set_title(t('Adding New Team'));
	}
	else
	{
		drupal_set_title(t('Editing ' . $team['name']));
		
		$form['team_id'] = array(
			'#type' 	=> 'hidden',
			'#value'	=> $team['team_id'],
		);
	}
	
	$form['name'] = array(
		'#type' 		=> 'textfield',
		'#title' 		=> t('Team Name'),
		'#default_value'	=> check_plain($team['name']),
		'#description' 		=> t('Specify a name for the team.'),
		'#required' 		=> TRUE,
	);
	
	$form['desc'] = array(
		'#type'			=> 'text_format',
		'#base_type'		=> 'textarea',
		'#title'		=> t('Description'),
		'#default_value'	=> check_markup($team['description']),
		'#description'		=> t('Describe what volunteering for this team involves.'),
		'#required'		=> TRUE,
	);
	
	
	$form['submit'] = array(
		'#type' 		=> 'submit', 
		'#value' 		=> t('Save Team')
	);
	
	return $form;
} //function transmute_admin_team

function transmute_admin_team_validate($form, &$form_state)
{
	$name = trim($form_state['values']['name']);
	$result = db_fetch_object(db_query('SELECT team_id FROM {transmute_teams} WHERE name="%s"', $name));
	if($result && $result->team_id != $form_state['values']['team_id'])
	{
		form_set_error('name', t('There is already a team named %name. Please pick another name.', array('%name' => $name)));
	}
	
	if(isset($form_state['values']['team_id']) && !is_numeric($form_state['values']['team_id']))
		form_set_error('team_id', t('Team ID is invalid.  You\'re probably trying to break something.  Bad.'));
	
} //function transmute_admin_team_validate

function transmute_admin_team_submit($form, &$form_state)
{
	$team_id = $form_state['values']['team_id'];
	
	if(isset($team_id))	//Existing team getting updated
	{
		db_query("UPDATE {transmute_teams} SET name = '%s', description = '%s' WHERE team_id = %d", $form_state['values']['name'], $form_state['values']['desc'], $team_id);
		drupal_set_message(t('%team updated.', array('%team' => $form_state['values']['name'])));
		$form_state['redirect'] = 'admin/settings/transmute/';
	}
	else //New team getting created
	{
		db_query("INSERT INTO {transmute_teams} (name, description) VALUES ('%s', '%s')", $form_state['values']['name'], $form_state['values']['desc']);
		$team_id = db_last_insert_id('transmute_teams', 'teams_id');
		drupal_set_message(t('%team created.', array('%team' => $form_state['values']['name'])));
		
		//If a new team is added, go the shift list.  Otherwise, stay on the page and show the changes.
		$form_state['redirect'] = t('admin/settings/transmute/@team_id/shifts', array('@team_id' => $team_id));
	}
	
	
	return;
} //function transmute_admin_team_submit

function transmute_admin_settings()
{
	$form['transmute_default_shift_length'] = array(
		'#type' => 'textfield',
		'#title' => t('Default shift length'),
		'#default_value' => variable_get('transmute_default_shift_length', 3),
		'#size' => 2,
		'#maxlength' => 2,
		'#description' => t("The default length (in hours) when a shift is added."),
		'#required' => TRUE,
	);
	
	$form['transmute_default_shift_participants'] = array(
		'#type' => 'textfield',
		'#title' => t('Default number of participants'),
		'#default_value' => variable_get('transmute_default_shift_participants', 3),
		'#size' => 2,
		'#maxlength' => 2,
		'#description' => t("The default number of participants for a volunteer shift."),
		'#required' => TRUE,
	);
	
	$form['transmute_event_start_date'] = array(
		'#type'			=> 'date',
		'#title'		=> t('Event Start Date'),
		'#default_value'	=> variable_get('transmute_event_start_date', array()),
		'#description'		=> t('The first day to have schedules for.'),
		'#required'		=> TRUE,
		);
	
	$form['transmute_event_end_date'] = array(
		'#type'			=> 'date',
		'#title'		=> t('Event Finish Date'),
		'#default_value'	=> variable_get('transmute_event_end_date', array()),
		'#description'		=> t('The last day to have schedules for.'),
		'#required'		=> TRUE,
		);
		
	$form['transmute_page_text'] = array(
		'#type'			=> 'text_format',
		'#base_type'		=> 'textarea',
		'#title'		=> t('Main page text'),
		'#default_value'	=> check_markup(variable_get('transmute_page_text', '')),
		'#rows'			=> 20,
		'#description'		=> t("Text to display on the front volunteer page."),
	);
	
	return system_settings_form($form);
} //function transmute_admin_settings

function transmute_admin_settings_validate($form, &$form_state)
{
	//transmute_default_shift_length
	$shift_length 		= $form_state['values']['transmute_default_shift_length'];
	$shift_participants = $form_state['values']['transmute_default_shift_participants'];
	$start_date			= $form_state['values']['transmute_event_start_date'];
	$end_date	 		= $form_state['values']['transmute_event_end_date'];
	
	if(!is_numeric($shift_length) || $shift_length <= 0)
	{
		form_set_error('transmute_shift_length', t('Shift length must be a number greater than zero.'));
	}
	
	if(!is_numeric($shift_participants) || $shift_participants <= 0)
	{
		form_set_error('transmute_shift_participants', t('The number of participants must be a number greater than zero.'));
	}

	if(strtotime($start_date['year'] . '-' . $start_date['month'] . '-' . $start_date['day']) > 
		strtotime($end_date['year'] . '-' . $end_date['month'] . '-' . $end_date['day']))
	{
		form_set_error('transmute_event_end_date', t('The Finish date must be after the Start date'));
	}
} //function transmute_admin_settings_validate

function transmute_admin_shift_list($form_state, $team = NULL)
{
	if(!isset($team))
		return drupal_not_found();
	
	drupal_set_title(t('Editing ' . $team['name'] . ' Shifts'));
	$query_result = db_query("SELECT * FROM {transmute_shifts} WHERE team_id = %d ORDER BY start_date ASC", $team['team_id']);
	
	while ($result = db_fetch_object($query_result))
	{
		$form[$result->shift_id]['title'] = array('#value' => check_plain($result->title));
		$form[$result->shift_id]['start_date'] = array('#value' => date('D, M j', strtotime($result->start_date)));
		$form[$result->shift_id]['start_time'] = array('#value' => date('g:ia', strtotime($result->start_date)));
		$form[$result->shift_id]['length'] = array('#value' => $result->length);
		$form[$result->shift_id]['participant_num'] = array('#value' => $result->participant_num);
		$form[$result->shift_id]['configure'] = array('#value' => l(t('Edit Shift'), 'admin/settings/transmute/'. $result->team_id . '/shifts/' . $result->shift_id));
		$form[$result->shift_id]['delete'] = array('#value' => l(t('Delete'), 'admin/settings/transmute/'. $result->team_id . '/shifts/' . $result->shift_id . '/delete'));
	}
	
	$form['add'] = array(
		//'#type'				=> 'button',
		//'#value'			=> t('Add Shift'),
		'#value'			=> '<input type="button" class="form-submit" value="' . t('Add Shift') . '" '.
			'onClick = \'location.href = "' . url('admin/settings/transmute/' . $team['team_id'] . '/shifts/add') . '";\'>',
		//'#submit'			=> array('onClick' => 'parent.location("' . url('admin/settings/transmute/' . $result->team_id . '/shifts/add') . '"); return false;'),
	);
	
	return $form;
} //function transmute_admin_shift_list

function theme_transmute_admin_shift_list($form)
 {
	$rows = array();
	$previous_date = "";
	$spacer = array('data' => '<hr>', 'colspan' => 7);
	
	foreach (element_children($form) as $item)
	{
		if(!is_numeric($item)) //Use this to prevent extra rows from being added because of system form stuff added.
			continue;
	
		$row = &$form[$item];
		
		if($row['start_date']['#value'] != $previous_date)
		{
			if($previous_date != "")
				$rows[] = array($spacer);
			$previous_date = $row['start_date']['#value'];
		}
		
		$rows[] = array(
			drupal_render($row['start_date']),
			drupal_render($row['start_time']),
			drupal_render($row['title']),
			drupal_render($row['length']),
			drupal_render($row['participant_num']),
			drupal_render($row['configure']),
			drupal_render($row['delete'])
		);
		unset($form[$item]);
	}
	$header = array(t('Start Date'), t('Start Time'), t('Title'), t('Length'), t('# Participants'), array('data' => t('Operations'), 'colspan' => 2));
	$output = theme('table', $header, $rows);
	$output .= drupal_render($form);

	return $output;
} //function theme_transmute_admin_list

function transmute_admin_shift($form_state, $shift)
{
	drupal_add_js(drupal_get_path('module', 'transmute') . '/scripts/jquery.timePicker.min.js');
	drupal_add_css(drupal_get_path('module', 'transmute') . '/scripts/timePicker.css');
	
	drupal_add_js('$(function(){$(".time-picker").timePicker({show24Hours: false});})', 'inline');
	
	$start_date = variable_get('transmute_event_start_date', array());
	$start_time = strtotime($start_date['year'] . '-' . $start_date['month'] . '-' . $start_date['day']);
	$end_date = variable_get('transmute_event_end_date', array());
	$end_time = strtotime($end_date['year'] . '-' . $end_date['month'] . '-' . $end_date['day']);
	
	$participant_list = '';
	
	$date_choices = array();
	for($i = $start_time; $i <= $end_time; $i += 60*60*24)
	{
		$date_choices[$i] = date('D, M j, Y', $i);
	}
	
	$form['team_id'] = array(
		'#type' 		=> 'hidden',
		'#value'		=> $shift['team_id'],
	);
	
	if(isset($shift['shift_id']))	//Editing an existing shift
	{
		$form['shift_id'] = array(
			'#type' 		=> 'hidden',
			'#value'		=> $shift['shift_id'],
		);
		
		//Get the list of participants
		$query_result = db_query('SELECT name FROM {users} u LEFT JOIN {transmute_participants} tp ON tp.user_id = u.uid WHERE tp.shift_id = %d', $shift['shift_id']);
		while($result = db_fetch_object($query_result))
		{
			$participant_list .= $result->name . "\n";
		}
	}
	else	//New shift creation
	{
		$shift['title'] = "";
		$shift['start_date'] = date('Y-m-d H:i:s', $start_time);
		$shift['length'] = variable_get('transmute_default_shift_length', 3);
		$shift['participant_num'] = variable_get('transmute_default_shift_participants', 3);
	}
	/*
	list($y, $m, $d) = explode(',', date('Y,n,j', strtotime($shift['start_date'])));
	$form['start_date'] = array(
		'#type'				=> 'date',
		'#title'			=> t('Start Date'),
		'#default_value'	=> array('year' => $y, 'month' => $m, 'day' => $d),
		'#description'		=> t('The day this shift starts on.'),
		'#required'			=> TRUE,
	);
	*/
	$form['title'] = array(
		'#type' 			=> 'textfield',
		'#title' 			=> t('Shift Title'),
		'#default_value'	=> check_plain($shift['title']),
		'#description' 		=> t('Optional custom title for the shift.'),
		'#required' 		=> FALSE,
		'#size'				=> 32,
		'#max_length'		=> 32,
	);
	
	$form['start_date'] = array(
		'#type'				=> 'select',
		'#title'			=> t('Start Date'),
		'#default_value'	=> strtotime(substr($shift['start_date'], 0, 10)),
		'#options'			=> $date_choices,
		'#multiple'			=> FALSE,
		'#description'		=> t('The day this shift starts on.'),
		'#required'			=> TRUE,
	);
	
	$form['use_custom_date'] = array(
		'#type'				=> 'checkbox',
		'#title'			=> t('Use custom date'),
		'#default_value'	=> array_key_exists($form['start_date']['#default_value'], $date_choices) ? 0 : 1,
		'#description'		=> t('Optionally, set a custom date for this shift with the box below.'),
	);
	
	$form['custom_start_date'] = array(
		'#type' 			=> 'textfield',
		'#title' 			=> t('Custom Start Date'),
		'#default_value'	=> date('d-m-Y', $form['start_date']['#default_value']),
		'#description' 		=> t('Format: YYYY-MM-DD'),
		'#size'				=> 20,
		'#max_length'		=> 10,
	);
	
	$form['start_time'] = array(
		'#type' 			=> 'textfield',
		'#title' 			=> t('Start Time'),
		'#default_value'	=> date('g:ia', strtotime($shift['start_date'])),
		'#description' 		=> t('The time the shift starts at.  Use format HH:MM am/pm.  You can change the time manually or use the dropdown selection.'),
		'#required' 		=> TRUE,
		'#size'				=> 7,
		'#attributes'		=> array('class' => 'time-picker'),
	);
	
	$form['length'] = array(
		'#type' 			=> 'textfield',
		'#title' 			=> t('Shift Length'),
		'#default_value'	=> check_plain($shift['length']),
		'#description' 		=> t('Shift length, in hours. Does not have to be whole numbers.'),
		'#required' 		=> TRUE,
		'#size'				=> 10,
	);
	
	$form['participant_num'] = array(
		'#type' 			=> 'textfield',
		'#title' 			=> t('Number of Participants'),
		'#default_value'	=> check_plain($shift['participant_num']),
		'#description' 		=> t('Number of people required for this shift'),
		'#required' 		=> TRUE,
		'#size'				=> 2,
	);
	
	if(isset($shift['shift_id']))	//Editing an existing shift
	{
		$form['modify_participant_list'] = array(
			'#type'				=> 'checkbox',
			'#title'			=> t('Modify the participant list below.'),
			'#default_value'	=> 0,
			'#description'		=> t('WARNING: This could possibly break things or screw up people\'s schedules.  Do not use unless you need to!'),
		);
		
		$form['participant_list'] = array(
			'#type' 			=> 'textarea',
			'#title' 			=> t('Participant List'),
			'#default_value'	=> $participant_list,
			'#description' 		=> t('The users currently signed up for this shift.  If the above box is checked, any changes made to this will be used in the database, including emptying the list to remove all participants.<br>
										One username per line, the usernames must be spelled exactly right.'),
			'#rows'				=> 10,
			'#cols'				=> 20,
			'#resizable' => FALSE,
		);
	}
	$form['submit'] = array(
		'#type' 			=> 'submit', 
		'#value' 			=> t('Save Shift')
	);
	global $user;
	//var_dump($user);
	
	return $form;
} //function transmute_admin_shift

function transmute_admin_shift_validate($form, &$form_state)
{
	if(isset($form_state['values']['shift_id']) && !is_numeric($form_state['values']['shift_id']))
		form_set_error('shift_id', t('Internal Error: shift_id invalid.'));
		
	
	if(!isset($form_state['values']['team_id']) || !is_numeric($form_state['values']['team_id']))
		form_set_error('team_id', t('Internal Error: team_id invalid.'));

	$time = $form_state['values']['start_time'];
	
	if(!strtotime($time))
		form_set_error('start_time', t('There is an error with the time.  Please check your formatting.'));
		
	if($form_state['values']['use_custom_date'])
	{
		if(empty($form_state['values']['custom_start_date']))
			form_set_error('custom_start_date', 'You must enter a custom date or uncheck the box.');
		elseif(!strtotime($form_state['values']['custom_start_date']))
			form_set_error('custom_start_date', 'You must enter a valid date.  Please check your formatting.');
	}
		
	if(!is_numeric($form_state['values']['length']))
		form_set_error('length', t('The length you have entered, %length, is not a number.', array('%length' => $form_state['values']['length'])));
	
	if(!is_numeric($form_state['values']['participant_num']))
		form_set_error('participant_num', t('The number of participants you have entered, %length, is not a number.', array('%length' => $form_state['values']['participant_num'])));		
	
} //function transmute_admin_shift_validate

function transmute_admin_shift_submit($form, &$form_state)
{
	$title = $form_state['values']['title'];
	$shift_id = $form_state['values']['shift_id'];
	$time = $form_state['values']['start_time'];
	
	if($form_state['values']['use_custom_date'])
		$date = strtotime($form_state['values']['custom_start_date']);
	else
		$date = $form_state['values']['start_date'];
	
	$final_date = date('Y-m-d H:i:s', strtotime(date('Y-m-d', $date) . ' ' . $time));
	
	if(isset($shift_id)) //Existing shift getting updated
	{
		db_query("UPDATE {transmute_shifts} SET ".
				"title = '%s', ".
				"start_date = '%s', ".
				"length = %f, ".
				"participant_num = %d ".
				"WHERE shift_id = %d",
				$title,
				$final_date, 
				$form_state['values']['length'], 
				$form_state['values']['participant_num'], 
				$shift_id);
		drupal_set_message(t('Shift updated. @date', array('@date' => $final_date)));
		
		//Modify the participant list if the associated box is checked
		if($form_state['values']['modify_participant_list'] == 1)
		{
			db_query("DELETE FROM {transmute_participants} WHERE shift_id = %d", $shift_id);
			
			$users = explode("\n", trim($form_state['values']['participant_list']));
			foreach($users as $k => $v)
			{
				$user = user_load_by_name(trim($v));
				if($user && $user->uid != 0)
				{
					db_query("INSERT INTO {transmute_participants} (shift_id, user_id) VALUES (%d, %d)", $shift_id, $user->uid);
				}
			}
		}
	}
	else //New team getting created
	{
		db_query("INSERT INTO {transmute_shifts} (team_id, title, start_date, length, participant_num) VALUES (%d, '%s', '%s', %f, %d)",
			$form_state['values']['team_id'], 
			$title,
			$final_date, 
			$form_state['values']['length'], 
			$form_state['values']['participant_num']);
		drupal_set_message(t('Shift created.'));
	}
	
	
	
	$form_state['redirect'] = t('admin/settings/transmute/@team_id/shifts', array('@team_id' => $form_state['values']['team_id']));
} //function transmute_admin_shift_submit

function transmute_delete_confirm(&$form_state, $id, $type)
{
	$form['element_id'] = array(
		'#type' => 'value',
		'#value' => $id,
		);
	
	$form['type'] = array(
		'#type' => 'value',
		'#value' => $type,
	);
	
	switch($type)
	{
		case 'team':
			$result = db_query('SELECT name FROM {transmute_teams} WHERE team_id = %d', $id);
			$confirm_text = t('Are you sure you want to delete %name?', array('%name' => db_fetch_object($result)->name));
			$additional_text = t('This will delete all associated shift and participant data.<br>This action cannot be undone.');
			break;
		case 'shift':
			$result = db_query('SELECT tt.name AS name, ts.start_date AS start_date, ts.length AS length '.
				'FROM {transmute_shifts} ts LEFT JOIN {transmute_teams} tt ON ts.team_id = tt.team_id WHERE ts.shift_id = %d', $id);
			$shift = db_fetch_object($result);
			
			$start_time = date('g:ia', strtotime($shift->start_date));
			$end_time = date('g:ia', strtotime($shift->start_date) + (60 * 60 * $result->length));
			$shift_date = date('D, M j', strtotime($shift->start_date));
			
			$confirm_text = t(
				'Are you sure you want to delete %name shift on %date from %time?',
				array('%name' => $shift->name, '%date' => $shift_date, '%time' => $start_time . ' to ' . $end_time)
			);
			$additional_text = t('This will delete all participant records for this shift.<br>This action cannot be undone.');
			
			break;
	}

	return confirm_form(
		$form,
		$confirm_text,
		'admin/settings/transmute',
		$additional_text,
		t('Delete'),
		t('Cancel')
	);
} //function transmute_delete_confirm

function transmute_delete_confirm_submit($form, &$form_state)
{
	if ($form_state['values']['confirm'])
	{
		$type = $form_state['values']['type'];
	
		switch($type)
		{
			case 'team':
				db_query('DELETE tp, ts, tt '.
					'FROM {transmute_teams} tt '.
					'LEFT JOIN {transmute_shifts} ts '.
						'ON tt.team_id = ts.team_id '.
					'LEFT JOIN {transmute_participants} tp '.
						'ON tp.shift_id = ts.shift_id '.
					'WHERE tt.team_id = %d',
					$form_state['values']['element_id']['team_id']
				);
				
				$redirect = 'admin/settings/transmute';
				break;
			case 'shift':
				db_query('DELETE tp, ts '.
					'FROM {transmute_shifts} ts '.
					'LEFT JOIN {transmute_participants} tp '.
						'ON tp.shift_id = ts.shift_id '.
					'WHERE ts.shift_id = %d',
					$form_state['values']['element_id']['shift_id']
				);
				
				$redirect = 'admin/settings/transmute/' . $form_state['values']['element_id']['team_id'] . '/shifts';
				break;
		}
	}

	$form_state['redirect'] = $redirect;
} //function transmute_delete_confirm_submit
