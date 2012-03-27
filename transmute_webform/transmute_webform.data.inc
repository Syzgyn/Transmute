<?php
function transmute_webform_data_list(&$form_state)
{
	$result = db_query('SELECT cid, pid, name FROM {webform_component} WHERE type = "select" AND nid = %d ORDER BY weight ASC', variable_get('transmute_webform_target_node_id', 0));
	
	//add javascript to the page to handle the select all checkbox  
	drupal_add_js("function toggleSelectAll(cbx)
	{
		if(\$(cbx).attr('checked') == true)
		{
			\$(\$(cbx).parents('form').get(0)).find('input[@type=checkbox]').each(function()
			{
				if(!this.checked){
					this.checked = true;
				}
			});
		}
		else
		{
			\$(\$(cbx).parents('form').get(0)).find('input[@type=checkbox]').each(function()
			{
				if(this.checked)
				{
					this.checked = false;
				}
			});
		}
	} 
	",'inline');  

	//add the select all checkbox to the form  
	
	$options = array();
	while($data = db_fetch_array($result))
	{
		$options[$data['pid']][$data['cid']] = $data['name'];
	}
	
	$form['teams'] = array(
		'#type'		=> 'fieldset',
		'#title'	=> t('Select which teams to view'),
		/*'#options'	=> $options,
		'#default_value'	=> isset($form_state['values']['teams']) ? $form_state['values']['teams'] : array(),*/
	);
	
	$result = db_query('SELECT cid, name FROM {webform_component} WHERE type = "fieldset" AND nid = %d ORDER BY weight ASC', variable_get('transmute_webform_target_node_id', 0));
	$groups = array();
	while($data = db_fetch_array($result))
	{
		$groups[] = $data['cid']; //For use when combining arrays below
		
		$form['teams'][$data['cid']] = array(
			'#type'		=> 'checkboxes',
			'#title'	=> check_plain($data['name']),
			'#options'	=> $options[$data['cid']],
			'#default_value'	=> isset($form_state['values'][$data['cid']]) ? $form_state['values'][$data['cid']] : array(),
		);
		/*
		$form['teams']['select_all_' . $data['cid']] = array(  
			'#type' => 'checkboxes',  
			'#options' => array("Select All " . $data['name']),
			'#attributes' => array('onchange' => "toggleSelectAll(this);"),  
		);*/
	}
	
	$form['teams']['select_all'] = array(  
		'#type' => 'checkboxes',  
		'#options' => array("Select All"),  
		'#attributes' => array('onchange' => "toggleSelectAll(this);"),  
	);  
	
	
	$form['grouping'] = array(
		'#type'		=> 'radios',
		'#title'	=> t('Group results'),
		'#options'	=> array(t('Keep Separate'),t('Group together')),
		'#default_value'	=> isset($form_state['values']['grouping']) ? check_plain($form_state['values']['grouping']) : 0,
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
	
	$view_teams = array();
	foreach($groups as $cid)
	{
		$view_teams = $view_teams + $form_state['values'][$cid];
	}
	
	//teams array returns K->V pairs of 0 or 1.  Remove the zeroes to iterate only the teams we want to see.
	function over_zero($x)
	{
		return($x > 0);
	}
	$view_teams = array_filter($view_teams, 'over_zero');
	
	//$form['data'] gets displayed at the end of the form, put our output there.
	if(count($view_teams) > 0)
	{
		if($form_state['values']['grouping'] == 0)
		{
			foreach($view_teams as $team_id)
			{
				
				$volunteers = _transmute_webform_get_volunteers($team_id);
				
				$output = t('<hr/><h3>%team</h3>', array('%team' => $volunteers['name']));
				$headers = array(t('User'), t('Name'), t('Email'), t('Signup date'));//, t('Skills'));
				$output .= theme('table', $headers, $volunteers['data']);
				
				$form['data']['#value'] .= $output;
			}
		}
		else
		{
			$volunteers = _transmute_webform_get_volunteers($view_teams);
			
			$output = t('<hr/><h3>%team</h3>', array('%team' => $volunteers['name']));
			$headers = array(t('User'), t('Name'), t('Email'), t('Signup date'), t('Teams'));//, t('Skills'));
			$output .= theme('table', $headers, $volunteers['data']);
			
			$form['data']['#value'] .= $output;
		}
	}
	else
	{
		$form['data']['#value'] = '';
	}
	
	return $form;
} //function transmute_webform_data_list

function transmute_webform_data_list_submit($form, &$form_state)
{
	$clicked_button = $form_state['clicked_button'];
	$view_type = $clicked_button['#name']; 
	
	if($view_type == 'download')
	{
		//If we're downloading, output the csv file with headers
		
		_transmute_webform_download_csv($form_state, $form['teams']);
	}
	else
	{
		//If not, submit and rebuild the form, the logic is at the end of transmute_data_list()
		$form_state['rebuild'] = TRUE;
	}
} //function transmute_webform_data_list_submit

function _transmute_webform_get_volunteers($component_id)
{
	$output = array('data' => array());
	$is_multiple = is_array($component_id);
	if($is_multiple)
	{
		$mult_component_id = implode(',', array_fill(0, count($component_id), "%d"));
		$query_vars = $component_id;
	}
	else
	{
		$query_vars = array($component_id);
	}
	
	//Have to use a separate array for the query input, because we have to pass either all ints, or one array.
	array_push($query_vars, variable_get('transmute_webform_target_node_id', 0));
	
	$result = db_query('SELECT u.uid, '.
			'tw.timestamp , '.
			'GROUP_CONCAT(DISTINCT wc.name SEPARATOR \', \') AS teams ' . 
		'FROM {transmute_webform} tw ' . 
			'LEFT JOIN {users} u ON tw.user_id = u.uid ' .
			'LEFT JOIN {webform_component} wc ON wc.cid = tw.component_id ' .
		'WHERE tw.component_id ' . ($is_multiple ? "IN ($mult_component_id) " : '= %d ') .
			'AND wc.nid = %d '.
		'GROUP BY u.uid '.
		'ORDER BY tw.timestamp DESC ',
		$query_vars
	);
	
	//if($is_multiple)
	//{
		$output['name'] = t('Multiple Teams');
	/*}
	else
	{
		$team_result = db_query('SELECT name FROM {webform_component} WHERE cid = %d AND nid = %d', $component_id, variable_get('transmute_webform_target_node_id', 0));
		$output['name'] = check_plain(db_fetch_object($team_result)->name);
	}*/
	
	$count = 0;
	while($data = db_fetch_array($result))
	{
		//Get a user object for username, email, and real name
		$user = user_load($data['uid']);
		profile_load_profile($user);
		$date = date('M j, Y g:ia', strtotime($data['timestamp']));
		
		$a = array(check_plain($user->name), check_plain($user->{profile_name}), check_plain($user->mail), $date);
		
		if($is_multiple)
		{
			array_push($a, check_plain($data['teams']));
		}
		else
		{
			$output['name'] = check_plain($data['teams']);
		}
		
		//array_push($a, check_plain($user->{profile_skills}));
		
		$output['data'][] = $a;
		$count++;
	}
	
	if(empty($output['data']))
	{
		$team_result = db_query('SELECT name FROM {webform_component} WHERE cid = %d AND nid = %d', $component_id, variable_get('transmute_webform_target_node_id', 0));
		$output['name'] = check_plain(db_fetch_object($team_result)->name);
		$output['data'][] = array('No Data', '', '', '');
	}
	
	$output['name'] .= sprintf(' (%d)', $count);
	
	return $output;
}

function _transmute_webform_download_csv($form_state, $groups)
{
	$output = array();
	$output[] = array('Created ' . date('D, M j g:ia'));
	
	$view_teams = array();
	foreach($groups as $cid => $v)
	{
		if(is_numeric($cid))
			$view_teams = $view_teams + $form_state['values'][$cid];
	}
	
	//teams array returns K->V pairs of 0 or 1.  Remove the zeroes to iterate only the teams we want to see.
	function over_zero($x)
	{
		return($x > 0);
	}
	$view_teams = array_filter($view_teams, 'over_zero');
	
	if($form_state['values']['grouping'] == 0)
	{
		foreach($view_teams as $team_id)
		{
			$x = _transmute_webform_get_volunteers($team_id);
			$output[] = array($x['name']);
			$output[] = array('User', 'Name', 'Email', 'Signup date');
			$output = array_merge($output, $x['data']);
			$output[] = array('');
		}
	}
	else
	{
		$x = _transmute_webform_get_volunteers($view_teams);
		$output[] = array($x['name']);
		$output[] = array('User', 'Name', 'Email', 'Teams');
		$output = array_merge($output, $x['data']);
	}
	
	$filename = 'volunteers-' . date('Y-m-d') . '.csv';
	
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
} //function _transmute_webform_download_csv