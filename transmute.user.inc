<?php
function transmute_volunteer_page()
{
	$query_result = db_query("SELECT name, {transmute_teams}.team_id, SUM(participant_num) AS total_shifts ".
		"FROM {transmute_teams} LEFT JOIN {transmute_shifts} ON {transmute_teams}.team_id = {transmute_shifts}.team_id GROUP BY {transmute_teams}.team_id");
		
	while ($result = $query_result->fetchAssoc())
	{
		$user_result = db_query("SELECT COUNT(*) AS full_shifts ".
			"FROM {transmute_participants} LEFT JOIN {transmute_shifts} ON {transmute_participants}.shift_id = {transmute_shifts}.shift_id ".
			"WHERE team_id = :team_id", array(':team_id' => $result['team_id']))->fetchAssoc();
		
		$full_shifts = $user_result['full_shifts'];
		$total_shifts = isset($result['total_shifts']) ? $result['total_shifts'] : 0;
		
		$form[$result['team_id']]['name'] = array('#markup' => l(check_plain($result['name']), 'volunteer/'. $result['team_id']));
		$form[$result['team_id']]['shifts'] = array('#markup' => t(($total_shifts - $full_shifts) . '/' . $total_shifts . ' shifts open'));
		$form[$result['team_id']]['schedule'] = array('#markup' => l(t('View Schedule'), 'volunteer/'. $result['team_id'] . '/schedule'));
		//$form[$result['team_id']]['sign_up'] = array('#markup' => l(t('View Schedule'), 'volunteer/'. $result->team_id . '/schedule'));
	}
	
	return $form;
} //function transmute_volunteer_page

function theme_transmute_volunteer_page($variables)
{
	$form = $variables['form'];
	$rows = array();
	
	foreach (element_children($form) as $item)
	{
		if(!is_numeric($item)) //Use this to prevent extra rows from being added because of system form stuff added.
			continue;
	
		$row = &$form[$item];
			
		$rows[] = array(
			drupal_render($row['name']),
			drupal_render($row['shifts']),
			drupal_render($row['schedule'])
		);
		unset($form[$item]);
	}
	$header = array(t('Team'), array('data' => t('Shifts')), t('Schedule'));//, 'colspan' => 2));
	$output = "\r\n<!-- Custom volunteer interface written for Alchemy LLC.\r\n".
		"Contact Patrick Murphy at pmurphy@alchemyfestival.com for more info.  Thanks to Transformus for the inspiration! -->\r\n";
	$output .= '<div>' . variable_get('transmute_page_text', '') . '</div>';
	$output .= theme('table', array('header' => $header, 'rows' => $rows));
	$output .= drupal_render_children($form);

	return $output;
} //function theme_transmute_volunteer_page

function transmute_volunteer_shift_page($form, &$form_state, $team)
{
	global $user;
	$logged_in = true;
	
	if($user->uid == 0)
	{
		$logged_in = false;
		drupal_set_message('Notice: You must be logged in to sign up for shifts.', 'warning', false);
	}
	
	if(empty($team))
		return drupal_not_found();
	
	$form['description'] = array('#markup' => $team['description']);
	
	drupal_set_title(t($team['name'] . ' Shifts'));
	$query_result = db_query("SELECT ".
			"ts.title AS title, ".
			"ts.shift_id AS shift_id, ".
			"start_date, ".
			"length, ".
			"participant_num, ".
			"COUNT(tp.user_id) AS num_signed_up, ".
			"name, ".
			"u.user_id AS user_id ".
		"FROM ".
		"{transmute_shifts} ts ".
		"LEFT JOIN {transmute_participants} tp ".
		"ON ts.shift_id = tp.shift_id ".
		"LEFT JOIN {transmute_teams} tt ".
		"ON ts.team_id = tt.team_id ".
		"LEFT JOIN (".
			"SELECT * FROM ".
			"{transmute_participants}".
			" WHERE user_id = :uid".
		") AS u ".
		"ON u.shift_id = ts.shift_id ".
		"WHERE ts.team_id = :team_id ".
		"GROUP BY ts.shift_id ".
		"ORDER BY start_date ASC",
		array(
			':uid' => $user->uid, 
			':team_id' => $team['team_id']
		)
	);
	
	while ($result = $query_result->fetchAssoc())
	{
		$start_time = date('g:ia', strtotime($result['start_date']));
		$end_time = date('g:ia', strtotime($result['start_date']) + (60 * 60 * $result['length']));
		$submit_text = isset($result['user_id']) ? 'Cancel' : 'Sign Up';
		
		$form[$result['shift_id']]['title']		  = array('#markup' => check_plain($result['title']));		
		$form[$result['shift_id']]['start_date'] = array('#markup' => date('D, M j', strtotime($result['start_date'])));
		$form[$result['shift_id']]['time_range'] = array('#markup' => check_plain($start_time . ' - ' . $end_time . ' (' . $result['length'] . ' ' . transmute_plural($result['length'], 'hour') . ')'));
		//$form[$result['shift_id']]['length'] = array('#markup' => check_plain($result['length']));
		$form[$result['shift_id']]['shift_info'] = array('#markup' => check_plain(($result['participant_num']-$result['num_signed_up']) . '/' . $result['participant_num']));
		$form[$result['shift_id']]['sign_up'] = array(
			'#type' => 'submit', 
			'#name' => check_plain($result['shift_id']),
			'#start_date'	=> date('D, M j', strtotime($result['start_date'])),
			'#time_range'	=> check_plain($start_time . ' - ' . $end_time),
			'#title'		=> check_plain($result['name']),
		);
		if(isset($result['user_id']))
		{
			//Already signed up, show cancel button
			$form[$result['shift_id']]['sign_up']['#value'] = t('Cancel');
			$form[$result['shift_id']]['sign_up']['#attributes'] = array(
				'onclick' => 'return confirm("' . t('Are you sure you want to remove yourself from this volunteer shift?') . '");',
				'class' => array('cancel-button'),
			);
		}
		elseif($result['participant_num'] > $result['num_signed_up'])
		{
			//Not full, show signup button
			$form[$result['shift_id']]['sign_up']['#value'] = t('Sign Up');
		}
		else
		{
			//Full, don't show any button
			$form[$result['shift_id']]['sign_up']['#type'] = 'markup';
			$form[$result['shift_id']]['sign_up']['#markup'] = 'FULL';
		}
		
		if(!$logged_in)
			$form[$result['shift_id']]['sign_up']['#type'] = 'markup';
			$form[$result['shift_id']]['sign_up']['#markup'] = $form[$result['shift_id']]['sign_up']['#value'];
	}
	
	return $form;
} //function transmute_volunteer_shift_page

function theme_transmute_volunteer_shift_page($variables)
{
	$form = $variables['form'];
	$rows = array();
	$previous_date = "";
	$spacer = array('data' => '<hr>', 'colspan' => 5);
	$output = '';
	
	foreach (element_children($form) as $item)
	{
		if(!is_numeric($item)) //Use this to prevent extra rows from being added because of system form stuff added.
			continue;
	
		$row = &$form[$item];
		
		if($row['start_date']['#markup'] != $previous_date)
		{
			if($previous_date != "")
				$rows[] = array($spacer);
			$previous_date = $row['start_date']['#markup'];
		}
			
		$rows[] = array(
		//$rows[$row['start_date']['#value']][] = array(
			drupal_render($row['start_date']),
			drupal_render($row['time_range']),
			//drupal_render($row['length']),
			drupal_render($row['title']),
			drupal_render($row['shift_info']),
			drupal_render($row['sign_up'])
		);
		unset($form[$item]);
	}
	
	$output .= '<h3>'. t('Description'). ' </h3>';
	$output .= '<p>' . drupal_render($form['description']) . '</p>';
	$output .= '<h3>' . t('Available Shifts'). '</h3>';

	$header = array(
		t('Day'), 
		t('Time'), 
		//t('<abbr title="Shift length (in hours)">Length</abbr>'),
		t('Role'), 
		t('<abbr title="Remaining Shifts/Total Shifts">Rem/Tot</abbr>'), 
		t('Sign Up')
	);
	/*
	foreach($rows as $day)
	{
		$output .= theme('table', $header, $day);
	}*/
	$output .= theme('table', array('header' => $header, 'rows' => $rows));
	$output .= drupal_render_children($form);

	return $output;
} //function theme_transmute_volunteer_shift_page

function transmute_volunteer_shift_page_validate($form, &$form_state)
{
	global $user;
	$user_fields = user_load($user->uid);
	$full_name = field_get_items('user', $user_fields, 'field_real_name');
	if($full_name)
	{
		$full_name = $full_name[0]['value'];
	}	
	if($full_name == NULL)
		form_set_error('no_name', t('Sorry, you need to have your name in your profile in order to sign up to volunteer.  '.
			'This is so we know who you are when we contact you or put you on the schedule.  '.
			l(t('You can update your profile here'), 'user/' . $user->uid . '/edit/Personal Information')
		));
}

function transmute_volunteer_shift_page_submit($form, &$form_state)
{
	global $user;
	$uid = $user->uid;
	//Hackish way to get info passed based on the submit button clicked.
	$clicked_button = $form_state['clicked_button'];
	$shift_id = $clicked_button['#name']; 
	
	$result = db_query("SELECT COUNT(*) AS not_new FROM {transmute_participants} WHERE shift_id = :shift_id AND user_id = :uid", 
		array(':shift_id' => $shift_id, ':uid' => $uid))->fetchAssoc();
	
	if($result['not_new'] == 0)
	{
		db_insert('transmute_participants')
			->fields(array(
				'shift_id' => $shift_id,
				'user_id'  => $uid,
			))
			->execute();
		drupal_set_message(t('You are now signed up for a %name shift on %date from %time.  You can always check ' . 
			l('your profile', 'user/' . $uid) . 
			' to see all your volunteer information',
			array(
				'%name' => $form_state['clicked_button']['#title'],
				'%date' => $form_state['clicked_button']['#start_date'], 
				'%time' => $form_state['clicked_button']['#time_range']
			)
		));
	}
	else
	{
		db_delete('transmute_participants')
			->condition('shift_id', $shift_id, '=')
			->condition('user_id', $uid, '=')
			->execute();
		drupal_set_message(t('You have removed yourself from a %name shift on %date from %time.',
			array(
				'%name' => $form_state['clicked_button']['#title'],
				'%date' => $form_state['clicked_button']['#start_date'], 
				'%time' => $form_state['clicked_button']['#time_range']
			)
		));
	}
	
} //function transmute_volunteer_shift_page_submit

function transmute_volunteer_schedule_page($team)
{
	//var_dump(func_get_args());
	$a = array();
	//return $a;
	
	//global $user;
	
	if(empty($team))
		return drupal_not_found();
	
	//$form['description'] = array('#value' => $team['description']);
	
	drupal_set_title(t($team['name'] . ' Schedule'));
	
	$result = db_query(
		'SELECT u.uid, ts.start_date, ts.length, tt.name, ts.title '.
		'FROM {users} u '.
			'RIGHT JOIN {transmute_participants} tp ON tp.user_id = u.uid '.
			'RIGHT JOIN {transmute_shifts} ts ON ts.shift_id = tp.shift_id '.
			'LEFT JOIN {transmute_teams} tt ON tt.team_id = ts.team_id '.
		'WHERE tt.team_id = :team_id ' .
		'ORDER BY ts.start_date ASC',
		array(':team_id' => $team['team_id'])
	);
	/*
	SELECT u.uid, ts.start_date, ts.length, tt.name, ts.title 
	FROM users u
	LEFT JOIN transmute_participants tp ON tp.user_id = u.uid
	LEFT JOIN transmute_shifts ts ON ts.shift_id = tp.shift_id
	LEFT JOIN transmute_teams tt ON tt.team_id = ts.team_id
	WHERE tt.team_id = 2 ORDER BY ts.start_date ASC
	*/
	
	$users_per_day = array();
	$users_per_shift = array();
	$users_per_team = array();
	$schedule = array();
	//Array format:
	//Day
	//	Shift time
	//		Team Name
	//			Shift Title
	//				User
	while($data = $result->fetchAssoc())
	{
		//Get a user object for username, email, and real name
		$date = date('D, M j', strtotime($data['start_date']));
		$user = user_load($data['uid']);
		$full_name = field_get_items('user', $user, 'field_real_name');
		
		$shift_label = date('g:ia', strtotime($data['start_date'])) . ' - ' . date('g:ia', strtotime($data['start_date']) + $data['length'] * 60 * 60);
		$schedule[$date][$shift_label][$data['name']][$data['title']][] = array(
			check_plain($user->name),
			//check_plain($user->{profile_name}),
			//check_plain($user->mail),
		);
		
		//These are used to track the number of users per each grouping, since we can't use count() in the middle of the loop of each grouping.
		if(!isset($users_per_day[$date])) { $users_per_day[$date] = 0;}
		if(!isset($users_per_shift[$date])) { $users_per_shift[$date] = array();}
		if(!isset($users_per_shift[$date][$shift_label])) { $users_per_shift[$date][$shift_label] = 0;}
		if(!isset($users_per_team[$date])) { $users_per_team[$date] = array();}
		if(!isset($users_per_team[$date][$shift_label])) { $users_per_team[$date][$shift_label] = array();}
		if(!isset($users_per_team[$date][$shift_label][$data['title']])) { $users_per_team[$date][$shift_label][$data['title']] = 0;}

		$users_per_day[$date]++;
		$users_per_shift[$date][$shift_label]++;
		$users_per_team[$date][$shift_label][$data['title']]++;
	}
	
	//These loops add the appropriate number for rowspan for each element to have, working outwards.
	//var_dump($schedule);
	$rows = array();
	foreach($schedule as $day => $shift_array)
	{
		$new_day = true;
		$day_row_span = 0;
		
		foreach($shift_array as $shift => $team_array)
		{
			$new_shift = true;
			foreach($team_array as $team => $title_array)
			{
				$new_team = true;
				foreach($title_array as $title => $user_array)
				{
					$new_title = true;
					foreach($user_array as $user)
					{
						//Check if we're at the beginning of a new grouping, add the corresponding element to the beginning of the array with the correct rowspan value
						if($new_title)
						{
							array_unshift($user, array('data' => check_plain($title), 'rowspan' => count($user_array)));
							$new_title = false;
						}
//						if($new_team)
//						{
//							array_unshift($user, array('data' => check_plain($team), 'rowspan' => $users_per_team[$day][$shift][$team]));
//							$new_team = false;
//						}
						if($new_shift)
						{
							array_unshift($user, array('data' => check_plain($shift), 'rowspan' => $users_per_shift[$day][$shift]));
							$new_shift = false;
						}
						if($new_day)
						{
							array_unshift($user, array('data' => check_plain($day), 'rowspan' => $users_per_day[$day]));
							$new_day = false;
						}
						$rows[] = $user;
					}
				}
			}
		}
	}
	
	$headers = array(t('Day'), t('Shift'), t('Title'), t('User'));//, t('Name'), t('Email'));
	$output = theme('table', array('header' => $headers, 'rows' => $rows));
	
	return $output;
	
	/*
	$query_result = db_query("SELECT ".
			"ts.title AS title, ".
			"ts.shift_id AS shift_id, ".
			"start_date, ".
			"length, ".
			"participant_num, ".
			"COUNT(tp.user_id) AS num_signed_up, ".
			"name, ".
			"u.user_id AS user_id ".
		"FROM ".
		"{transmute_shifts} ts ".
		"LEFT JOIN {transmute_participants} tp ".
		"ON ts.shift_id = tp.shift_id ".
		"LEFT JOIN {transmute_teams} tt ".
		"ON ts.team_id = tt.team_id ".
		"LEFT JOIN (".
			"SELECT * FROM ".
			"{transmute_participants}".
			" WHERE user_id = %d".
		") AS u ".
		"ON u.shift_id = ts.shift_id ".
		"WHERE ts.team_id = %d ".
		"GROUP BY ts.shift_id ".
		"ORDER BY start_date ASC",
		$user->uid, 
		$team['team_id']
	);
	
	while ($result = db_fetch_object($query_result))
	{
		$start_time = date('g:ia', strtotime($result->start_date));
		$end_time = date('g:ia', strtotime($result->start_date) + (60 * 60 * $result->length));
		$submit_text = isset($result->user_id) ? 'Cancel' : 'Sign Up';
		
		
		$form[$result->shift_id]['start_date_hidden'] = array('#type' => 'hidden', '#value' => date('D, M j', strtotime($result->start_date)));
		$form[$result->shift_id]['time_range_hidden'] = array('#type' => 'hidden', '#value' => check_plain($start_time . ' - ' . $end_time));
		$form[$result->shift_id]['name_hidden']		  = array('#type' => 'hidden', '#value' => check_plain($result->name));
		
		$form[$result->shift_id]['title']		  = array('#value' => check_plain($result->title));		
		$form[$result->shift_id]['start_date'] = array('#value' => date('D, M j', strtotime($result->start_date)));
		$form[$result->shift_id]['time_range'] = array('#value' => check_plain($start_time . ' - ' . $end_time . ' (' . $result->length . ' ' . transmute_plural($result->length, 'hour') . ')'));
		//$form[$result->shift_id]['length'] = array('#value' => check_plain($result->length));
		$form[$result->shift_id]['shift_info'] = array('#value' => check_plain(($result->participant_num-$result->num_signed_up) . '/' . $result->participant_num));
		$form[$result->shift_id]['sign_up'] = array(
			'#type' => 'submit', 
			'#name' => check_plain($result->shift_id),
		);
		if(isset($result->user_id))
		{
			$form[$result->shift_id]['sign_up']['#value'] = t('Cancel');
			$form[$result->shift_id]['sign_up']['#attributes'] = array('onclick' => 'return confirm("' . t('Are you sure you want to remove yourself from this volunteer shift?') . '");');
		}
		else
			$form[$result->shift_id]['sign_up']['#value'] = t('Sign Up');
	}
	*/
} //function transmute_volunteer_schedule_page

function theme_transmute_volunteer_schedule_page($rows)
{
	//var_dump($rows);
	$headers = array(t('Day'), t('Shift'), t('Title'), t('User'), t('Name'), t('Email'));
	//$output = theme('table', array('header' => $headers, 'rows' => $rows));
	$output = 'a';
	return $output;
}
