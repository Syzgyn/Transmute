<?php
function transmute_data_list($form, &$form_state)
{
	$result = db_query('SELECT name, team_id FROM {transmute_teams}');
	
	$options = array();
	while($team = $result->fetchAssoc())
	{
		$options[$team['team_id']] = check_plain($team['name']);
	}
	$form['teams'] = array(
		'#type'		=> 'checkboxes',
		'#title'	=> t('Select which teams to view'),
		'#options'	=> $options,
		'#default_value'	=> isset($form_state['values']['teams']) ? $form_state['values']['teams'] : array(),
	);
	
	$form['grouping'] = array(
		'#type'		=> 'radios',
		'#title'	=> t('Group results'),
		'#options'	=> array(t('Keep Separate'),t('Group together')),
		'#default_value'	=> isset($form_state['values']['grouping']) ? check_plain($form_state['values']['grouping']) : 0,
	);
	
	$form['format'] = array(
		'#type'		=> 'radios',
		'#title'	=> t('Format'),
		'#options'	=> array(t('Schedule'),t('Contact List')),
		'#default_value'	=> isset($form_state['values']['format']) ? check_plain($form_state['values']['format']) : 0,
	);
	
	$form['view_people'] = array(
		'#type'		=> 'submit',
		'#value'	=> t('View'),
		'#submit'	=> array('transmute_data_list_submit_print'),
	);
	
	$form['view_download'] = array(
		'#type'		=> 'submit',
		'#value'	=> t('Download'),
		'#submit'	=> array('transmute_data_list_submit_download'),
	);
	
	$form['data'] = array(
		'#markup'	=> '',
	);
	
	if(!$form_state['submitted'] || !$form_state['display_table'])
		return $form;
	//Should get this far only if the form has been submitted already.
	
	//teams array returns K->V pairs of 0 or 1.  Remove the zeroes to iterate only the teams we want to see.
	$view_teams = array_filter($form_state['values']['teams'], '_transmute_over_zero');
	
	//$form['data'] gets displayed at the end of the form, put our output there.
	if($form_state['values']['grouping'] == 0)
	{
		foreach($view_teams as $team_id)
		{
			
			$participants = _transmute_query_participant($form_state['values']['format'], $team_id);
			
			$form['data']['#markup'] .= $participants;
		}
	}
	else
	{
		$form['data']['#markup'] .= _transmute_query_participant($form_state['values']['format'], $view_teams, TRUE);
	}
	return $form;
} //function transmute_data_list

function transmute_data_list_submit_print($form, &$form_state)
{
	//submit and rebuild the form, the logic is at the end of transmute_data_list()
	$form_state['rebuild'] = TRUE;
	$form_state['display_table'] = TRUE;
}

function transmute_data_list_submit_download($form, &$form_state)
{
	$form_state['rebuild'] = TRUE;
	$form_state['display_table'] = FALSE;
	_transmute_download_csv($form_state);
}

function _transmute_query_participant($view_type, $team_id)
{
	$is_multiple = is_array($team_id);
	if($is_multiple)
	{
		$mult_team_id = implode(',', $team_id); //array_fill(0, count($team_id), ":team_id"));
		$team_name = t('Multiple Teams');
	}
	else
	{
		$team_result = db_query('SELECT name FROM {transmute_teams} WHERE team_id = :team_id', array(':team_id' =>$team_id))->fetchAssoc();
		$team_name = check_plain($team_result['name']);
	}
	
	switch($view_type)
	{
		case 0: //Schedule
			$result = db_query(
				'SELECT u.uid, ts.start_date, ts.length, tt.name, ts.title '.
				'FROM {users} u '.
					'LEFT JOIN {transmute_participants} tp ON tp.user_id = u.uid '.
					'LEFT JOIN {transmute_shifts} ts ON ts.shift_id = tp.shift_id '.
					'LEFT JOIN {transmute_teams} tt ON tt.team_id = ts.team_id '.
				'WHERE tt.team_id ' . ($is_multiple ? "IN ($mult_team_id) " : '= :team_id ') .
				'ORDER BY ts.start_date ASC',
				array(':team_id' =>$team_id)
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
				$user = user_load($data['uid']);
				$full_name = field_get_items('user', $user, 'field_real_name');
				
				$shift_label = date('g:ia', $data['start_date']) . ' - ' . date('g:ia', $data['start_date'] + $data['length'] * 60 * 60);
				$schedule[date('Y-m-d', $data['start_date'])][$shift_label][$data['name']][$data['title']][] = array(
					check_plain($user->name),
					$full_name[0]['safe_value'],
					check_plain($user->mail),
				);
				$k = date('Y-m-d', $data['start_date']);
				if(!isset($users_per_day[$k])) {$users_per_day[$k] = 0;}
				if(!isset($users_per_shift[$k])) {$users_per_shift[$k] = array();}
				if(!isset($users_per_shift[$k][$shift_label])) {$users_per_shift[$k][$shift_label] = 0;}
				if(!isset($users_per_team[$k])) {$users_per_team[$k] = array();}
				if(!isset($users_per_team[$k][$shift_label])) {$users_per_team[$k][$shift_label] = array();}
				if(!isset($users_per_team[$k][$shift_label][$data['name']])) {$users_per_team[$k][$shift_label][$data['name']] = 0;}
				//These are used to track the number of users per each grouping, since we can't use count() in the middle of the loop of each grouping.
				$users_per_day[$k]++;
				$users_per_shift[$k][$shift_label]++;
				$users_per_team[$k][$shift_label][$data['name']]++;
			}
			
			//These loops add the appropriate number for rowspan for each element to have, working outwards.
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
								if($new_team && $is_multiple)
								{
									array_unshift($user, array('data' => check_plain($team), 'rowspan' => $users_per_team[$day][$shift][$team]));
									$new_team = false;
								}
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
			
			$headers = array(t('Day'), t('Shift'));
			if($is_multiple)
				array_push($headers, t('Team'));
			$headers[] = t('Title');
			array_push($headers, t('User'), t('Name'), t('Email'));
			$output = t('<hr/><h3>%team</h3>', array('%team' => $team_name));
			$output .= theme('table', array('header' => $headers, 'rows' => $rows));
			
			return $output;
			break;
			
		case 1:	//Contact List
			$result = db_query(
				'SELECT u.uid, GROUP_CONCAT(DISTINCT tt.name SEPARATOR \', \') AS teams '.
				'FROM {users} u '.
					'LEFT JOIN {transmute_participants} tp ON tp.user_id = u.uid '.
					'LEFT JOIN {transmute_shifts} ts ON ts.shift_id = tp.shift_id '.
					'LEFT JOIN {transmute_teams} tt ON tt.team_id = ts.team_id '.
				'WHERE tt.team_id ' . ($is_multiple ? "IN ($mult_team_id)" : '= :team_id ') .
				'GROUP BY u.uid',
				array(':team_id' => $team_id)
			);
			
			$rows = array();
			while($data = $result->fetchAssoc())
			{
				//Get a user object for username, email, and real name
				$user = user_load($data['uid']);
				$full_name = field_get_items('user', $user, 'field_real_name');
				
				if($user)
				{
					$a = array($user->name, $full_name[0]['safe_value'], $user->mail);
					if($is_multiple)
						array_push($a, $data['teams']);
					
					$rows[] = $a;
				}
			}
			
			$output = t('<hr/><h3>%team</h3>', array('%team' => $team_name));
			$headers = array(t('User'), t('Name'), t('Email'));
			if($is_multiple)
				array_push($headers, t('Teams'));
			
			$output .= theme('table', array('header' => $headers, 'rows' =>$rows));
			return $output;
	}
}

function _transmute_download_csv($form_state)
{
	$output[] = array('Created ' . date('D, M j g:ia'));
	
	$view_teams = array_filter($form_state['values']['teams'], '_transmute_over_zero');
	
	if($form_state['values']['grouping'] == 0)
	{
		foreach($view_teams as $team_id)
		{
			_transmute_get_csv(&$output, $form_state['values']['format'], $team_id);
		}
	}
	else
	{
		_transmute_get_csv(&$output, $form_state['values']['format'], $view_teams);
	}
	
	$filename = $form_state['values']['format'] == 0 ? 'schedule-' . date('Y-m-d') . '.csv' : 'contacts-' . date('Y-m-d') . '.csv';
	
	//Headers
	header("Cache-Control: no-store, no-cache");  
	header('Content-Disposition: attachment; filename="' . $filename . '"');  
	
	$outstream = fopen('php://output', 'w');
	
	foreach($output as $row)
	{
		fputcsv($outstream, $row);
	}
	
	fclose($outstream);
exit();	
drupal_exit();
} //function _transmute_download_csv

function _transmute_get_csv(&$output, $view_type, $team_id)
{
	$is_multiple = is_array($team_id);
	if($is_multiple)
	{
		$mult_team_id = implode(',', $team_id);//array_fill(0, count($team_id), "%d"));
	}
	
	switch($view_type)
	{
		case 0: //Schedule
			$result = db_query(
				'SELECT u.uid, ts.start_date, ts.length, ts.title '.
				'FROM {transmute_teams} tt '.
					'LEFT JOIN {transmute_shifts} ts ON tt.team_id = ts.team_id '.
					'LEFT JOIN {transmute_participants} tp ON ts.shift_id = tp.shift_id '.
					'LEFT JOIN {users} u ON tp.user_id = u.uid '.
				'WHERE tt.team_id ' . ($is_multiple ? "IN ($mult_team_id)" : '= :team_id ') .
				'ORDER BY ts.start_date ASC, ts.title',
				array(':team_id' =>$team_id)
			);
			
			if($is_multiple)
			{
				$output[] = array(t('Multiple Teams'));
			}
			else
			{
				$team_result = db_query('SELECT name FROM {transmute_teams} WHERE team_id = :team_id', array(':team_id' =>$team_id))->fetchAssoc();
				$output[] = array(check_plain($team_result['name']));
			}
			
			$users_per_day = array();
			$schedule = array();
			//Array format:
			//Day
			//	Shift time
			//		User
			$output[] = array('Day', 'Shift', 'Shift Title', 'User', 'Name', 'Email');
			while($data = $result->fetchAssoc())
			{
				if($data['uid'] == NULL)
					continue;
				//Get a user object for username, email, and real name
				$user = user_load($data['uid']);
				$full_name = field_get_items('user', $user, 'field_real_name');
				
				$shift_label = date('g:ia', $data['start_date']) . ' - ' . date('g:ia', $data['start_date'] + $data['length'] * 60 * 60);
				$output[] = array(date('Y-m-d', $data['start_date']), $shift_label, $data['title'], $user->name, $full_name[0]['safe_value'], $user->mail);
			}
			
			$output[] = array('');
			break;
			
		case 1:	//Contact List
			$result = db_query(
				'SELECT u.uid FROM {transmute_teams} tt '.
					'LEFT JOIN {transmute_shifts} ts ON tt.team_id = ts.team_id '.
					'LEFT JOIN {transmute_participants} tp ON ts.shift_id = tp.shift_id '.
					'LEFT JOIN {users} u ON tp.user_id = u.uid '.
				'WHERE tt.team_id ' . ($is_multiple ? "IN ($mult_team_id)" : '= :team_id ') .
				'GROUP BY u.uid',
				array(':team_id' =>$team_id)
			);
			
			if($is_multiple)
			{
				$output[] = array(t('Multiple Teams'));
			}
			else
			{
				$team_result = db_query('SELECT name FROM {transmute_teams} WHERE team_id = :team_id', array(':team_id' => $team_id))->fetchAssoc();
				$output[] = array(check_plain($team_result['name']));
			}
			$output[] = array('User', 'Name', 'Email');
			while($data = $result->fetchAssoc())
			{
				//Get a user object for username, email, and real name
				$user = user_load($data['uid']);
				$full_name = field_get_items('user', $user, 'field_real_name');
				
				if($user)
					$output[] = array(check_plain($user->name), $full_name[0]['safe_value'], check_plain($user->mail));
			}
			
			$output[] = array('');
			break;
	}
}

function _transmute_over_zero($x)
{
	return($x > 0);
}
