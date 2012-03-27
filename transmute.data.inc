<?php
function transmute_data_list(&$form_state)
{
	$result = db_query('SELECT name, team_id FROM {transmute_teams}');
	
	$options = array();
	while($team = db_fetch_object($result))
	{
		$options[$team->team_id] = check_plain($team->name);
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
		'#name'		=> 'view',
	);
	
	$form['view_download'] = array(
		'#type'		=> 'submit',
		'#value'	=> t('Download'),
		'#name'		=> 'download',
	);
	
	$form['data'] = array(
		'#value'	=> '',
	);
	
	if(!$form_state['submitted'])
		return $form;
	//Should get this far only if the form has been submitted already.
	
	//teams array returns K->V pairs of 0 or 1.  Remove the zeroes to iterate only the teams we want to see.
	function over_zero($x)
	{
		return($x > 0);
	}
	$view_teams = array_filter($form_state['values']['teams'], 'over_zero');
	
	//$form['data'] gets displayed at the end of the form, put our output there.
	if($form_state['values']['grouping'] == 0)
	{
		foreach($view_teams as $team_id)
		{
			
			$participants = _transmute_query_participant($form_state['values']['format'], $team_id);
			
			$form['data']['#value'] .= $participants;
		}
	}
	else
	{
		$form['data']['#value'] .= _transmute_query_participant($form_state['values']['format'], $view_teams, TRUE);
	}
	return $form;
} //function transmute_data_list

function transmute_data_list_submit($form, &$form_state)
{
	$clicked_button = $form_state['clicked_button'];
	$view_type = $clicked_button['#name']; 
	
	if($view_type == 'download')
	{
		//If we're downloading, output the csv file with headers
		_transmute_download_csv($form_state);
	}
	else
	{
		//If not, submit and rebuild the form, the logic is at the end of transmute_data_list()
		$form_state['rebuild'] = TRUE;
	}
}

function _transmute_query_participant($view_type, $team_id)
{
	$is_multiple = is_array($team_id);
	if($is_multiple)
	{
		$mult_team_id = implode(',', array_fill(0, count($team_id), "%d"));
		$team_name = t('Multiple Teams');
	}
	else
	{
		$team_result = db_query('SELECT name FROM {transmute_teams} WHERE team_id = %d', $team_id);
		$team_name = check_plain(db_fetch_object($team_result)->name);
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
				'WHERE tt.team_id ' . ($is_multiple ? "IN ($mult_team_id)" : '= %d ') .
				'ORDER BY ts.start_date ASC',
				$team_id
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
			while($data = db_fetch_array($result))
			{
				//Get a user object for username, email, and real name
				$user = user_load($data['uid']);
				profile_load_profile($user);
				
				$shift_label = date('g:ia', strtotime($data['start_date'])) . ' - ' . date('g:ia', strtotime($data['start_date']) + $data['length'] * 60 * 60);
				$schedule[date('Y-m-d', strtotime($data['start_date']))][$shift_label][$data['name']][$data['title']][] = array(
					check_plain($user->name),
					check_plain($user->{profile_name}),
					check_plain($user->mail),
				);
				
				//These are used to track the number of users per each grouping, since we can't use count() in the middle of the loop of each grouping.
				  $users_per_day[date('Y-m-d', strtotime($data['start_date']))]++;
				$users_per_shift[date('Y-m-d', strtotime($data['start_date']))][$shift_label]++;
				 $users_per_team[date('Y-m-d', strtotime($data['start_date']))][$shift_label][$data['title']]++;
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
			$output .= theme('table', $headers, $rows);
			
			return $output;
			
		case 1:	//Contact List
			$result = db_query(
				'SELECT u.uid, GROUP_CONCAT(DISTINCT tt.name SEPARATOR \', \') AS teams '.
				'FROM {users} u '.
					'LEFT JOIN {transmute_participants} tp ON tp.user_id = u.uid '.
					'LEFT JOIN {transmute_shifts} ts ON ts.shift_id = tp.shift_id '.
					'LEFT JOIN {transmute_teams} tt ON tt.team_id = ts.team_id '.
				'WHERE tt.team_id ' . ($is_multiple ? "IN ($mult_team_id)" : '= %d ') .
				'GROUP BY u.uid',
				$team_id
			);
			
			$rows = array();
			while($data = db_fetch_array($result))
			{
				//Get a user object for username, email, and real name
				$user = user_load($data['uid']);
				profile_load_profile($user);
				
				if($user)
				{
					$a = array($user->name, $user->{profile_name}, $user->mail);
					if($is_multiple)
						array_push($a, $data['teams']);
					
					$rows[] = $a;
				}
			}
			
			$output = t('<hr/><h3>%team</h3>', array('%team' => $team_name));
			$headers = array(t('User'), t('Name'), t('Email'));
			if($is_multiple)
				array_push($headers, t('Teams'));
			
			$output .= theme('table', $headers, $rows);
			
			return $output;
			break;
	}
}

function _transmute_download_csv($form_state)
{
	$output = array();
	$output[] = array('Created ' . date('D, M j g:ia'));
	
	function over_zero($x)
	{
		return($x > 0);
	}
	$view_teams = array_filter($form_state['values']['teams'], 'over_zero');
	
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
	header("Content-type: text/csv");  
	header("Cache-Control: no-store, no-cache");  
	header('Content-Disposition: attachment; filename="' . $filename . '"');  
	
	$outstream = fopen('php://output', 'w');
	
	foreach($output as $row)
	{
		fputcsv($outstream, $row);
	}
	
	fclose($outstream);
} //function _transmute_download_csv

function _transmute_get_csv(&$output, $view_type, $team_id)
{
	$is_multiple = is_array($team_id);
	if($is_multiple)
	{
		$mult_team_id = implode(',', array_fill(0, count($team_id), "%d"));
	}
	
	switch($view_type)
	{
		case 0: //Schedule
			$result = db_query(
				'SELECT u.uid, ts.start_date, ts.length '.
				'FROM {transmute_teams} tt '.
					'LEFT JOIN {transmute_shifts} ts ON tt.team_id = ts.team_id '.
					'LEFT JOIN {transmute_participants} tp ON ts.shift_id = tp.shift_id '.
					'LEFT JOIN {users} u ON tp.user_id = u.uid '.
				'WHERE tt.team_id ' . ($is_multiple ? "IN ($mult_team_id)" : '= %d ') .
				'ORDER BY ts.start_date ASC',
				$team_id
			);
			
			if($is_multiple)
			{
				$output[] = array(t('Multiple Teams'));
			}
			else
			{
				$team_result = db_query('SELECT name FROM {transmute_teams} WHERE team_id = %d', $team_id);
				$output[] = array(check_plain(db_fetch_object($team_result)->name));
			}
			
			$users_per_day = array();
			$schedule = array();
			//Array format:
			//Day
			//	Shift time
			//		User
			$output[] = array('Day', 'Shift', 'User', 'Name', 'Email');
			while($data = db_fetch_array($result))
			{
				//Get a user object for username, email, and real name
				$user = user_load($data['uid']);
				profile_load_profile($user);
				
				$shift_label = date('g:ia', strtotime($data['start_date'])) . ' - ' . date('g:ia', strtotime($data['start_date']) + $data['length'] * 60 * 60);
				$output[] = array(date('Y-m-d', strtotime($data['start_date'])), $shift_label, $user->name, $user->{profile_name}, $user->mail);
			}
			
			$output[] = array('');
			break;
			
		case 1:	//Contact List
			$result = db_query(
				'SELECT u.uid FROM {transmute_teams} tt '.
					'LEFT JOIN {transmute_shifts} ts ON tt.team_id = ts.team_id '.
					'LEFT JOIN {transmute_participants} tp ON ts.shift_id = tp.shift_id '.
					'LEFT JOIN {users} u ON tp.user_id = u.uid '.
				'WHERE tt.team_id ' . ($is_multiple ? "IN ($mult_team_id)" : '= %d ') .
				'GROUP BY u.uid',
				$team_id
			);
			
			if($is_multiple)
			{
				$output[] = array(t('Multiple Teams'));
			}
			else
			{
				$team_result = db_query('SELECT name FROM {transmute_teams} WHERE team_id = %d', $team_id);
				$output[] = array(check_plain(db_fetch_object($team_result)->name));
			}
			$output[] = array('User', 'Name', 'Email');
			while($data = db_fetch_array($result))
			{
				//Get a user object for username, email, and real name
				$user = user_load($data['uid']);
				profile_load_profile($user);
				
				if($user)
					$output[] = array(check_plain($user->name), check_plain($user->{profile_name}), check_plain($user->mail));
			}
			
			$output[] = array('');
			break;
	}
}