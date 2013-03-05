<?php
// $Id$ 

/**
*  This file is used to tell the views module about the new node_example table.
*
* Database definition:
* @code
*   CREATE TABLE node_example (
*     vid int(10) unsigned NOT NULL default '0',
*     nid int(10) unsigned NOT NULL default '0',
*     color varchar(255) NOT NULL default '',
*     quantity int(10) unsigned NOT NULL default '0',
*     PRIMARY KEY (vid, nid),
*     KEY `node_example_nid` (nid)
*   )
* @endcode
*/

function transmute_views_data()
{
	// Basic table information.

	// ----------------------------------------------------------------
	// node_example table
	//  New group within Views called 'Example'

	$data = array();
	$data['transmute_teams']['table']['group']  = t('Transmute');
	$data['transmute_teams']['table']['base'] = array(
		'field'	=> 'team_id',
		'title'	=> t('Volunteer Team'),
		'help' => t("A volunteer team that users can signup for shifts in."),
		'weight' => -10,
	);
/*
	$data['transmute_shifts']['table']['join']['transmute_teams'] = array(
		'left_field'	=> 'team_id',
		'field'			=> 'team_id',
	);
	
	$data['transmute_participants']['table']['join']['transmute_teams'] = array(
		'left_table'	=> 'transmute_shifts',
		'left_field'	=> 'team_id',
		'field'			=> 'team_id',
	);
	*/
	
	$data['transmute_teams']['team_id'] = array(
		'title'	=> t('Team ID'),
		'help'	=> t('The Team ID of the Team'),
		'field' => array(
			'handler' => 'views_handler_field',
		),
		'filter' => array(
			'handler' => 'views_handler_filter_numeric',
		),
		'argument' => array(
			'handler' => 'views_handler_argument_numeric',
			'name field'	=> 'name',
			'numeric'	=> TRUE,
		),
		'sort' => array(
			'handler' => 'views_handler_sort',
		),
		
	);
	
	$data['transmute_teams']['name'] = array(
		'title' => t('Team Name'),
		'help'	=> t('The Name of the team.'),
		'field'	=> array(
			'handler' => 'views_handler_field',
		),
		'filter' => array(
			'handler' => 'views_handler_filter_string',
		),
		'argument' => array(
			'handler' => 'views_handler_argument_string',
			'name field' => 'name',
			'numeric' => FALSE,
		),
		'sort' => array(
			'handler' => 'views_handler_sort',
		),
	);
	
	$data['transmute_teams']['description'] = array(
		'title' => t('Team Description'),
		'help'	=> t('The Description of the team.'),
		'field'	=> array(
			'handler' => 'views_handler_field_markup',
			'format'  => 'filtered_html',
		),
		'filter' => array(
			'handler' => 'views_handler_filter_string',
		),
		'argument' => array(
			'handler' => 'views_handler_argument_string',
			'name field' => 'name',
			'numeric' => FALSE,
		),
		'sort' => array(
			'handler' => 'views_handler_sort',
		),
	);

	$data['transmute_shifts']['table']['join']['transmute_teams'] = array(
		'left_field' => 'team_id',
		'field'	     => 'team_id',
	);

	$data['transmute_shifts']['table']['group']  = t('Transmute');
	$data['transmute_shifts']['table']['base'] = array(
		'field'	=> 'shift_id',
		'title'	=> t('Volunteer Shift'),
		'help' => t("A volunteer shift that belongs to a team."),
	);

	$data['transmute_shifts']['team_id'] = array(
		'title'	=> t('Shift Team ID'),
		'help'	=> t('The Team ID the shift belongs to.'),
		'field' => array(
			'handler' => 'views_handler_field',
		),
		'filter' => array(
			'handler' => 'views_handler_filter_numeric',
		),
		'argument' => array(
			'handler' => 'views_handler_argument_numeric',
			'name field'	=> 'name',
			'numeric'	=> TRUE,
		),
		'sort' => array(
			'handler' => 'views_handler_sort',
		),
	);

	$data['transmute_shifts']['shift_id'] = array(
		'title'	=> t('Shift ID'),
		'help'	=> t('The Shift ID of the shift.'),
		'field' => array(
			'handler' => 'views_handler_field',
		),
		'filter' => array(
			'handler' => 'views_handler_filter_numeric',
		),
		'argument' => array(
			'handler' => 'views_handler_argument_numeric',
			'name field'	=> 'name',
			'numeric'	=> TRUE,
		),
		'sort' => array(
			'handler' => 'views_handler_sort',
		),
	);

	$data['transmute_shifts']['length'] = array(
		'title'	=> t('Shift Length'),
		'help'	=> t('The Length of the shift, in hours.'),
		'field' => array(
			'handler' => 'views_handler_field',
		),
		'filter' => array(
			'handler' => 'views_handler_filter_numeric',
		),
		'argument' => array(
			'handler' => 'views_handler_argument_numeric',
			'name field'	=> 'name',
			'numeric'	=> TRUE,
		),
		'sort' => array(
			'handler' => 'views_handler_sort',
		),
	);

	$data['transmute_shifts']['participant_num'] = array(
		'title' => t('Shift Maximum Participants'),
		'help'	=> t('The maximum number of participants in the shift'),
		'field' => array(
			'handler' => 'views_handler_field',
		),
		'filter' => array(
			'handler' => 'views_handler_filter_numeric',
		),
		'argument' => array(
			'handler' => 'views_handler_argument_numeric',
			'name field'	=> 'name',
			'numeric'	=> TRUE,
		),
		'sort' => array(
			'handler' => 'views_handler_sort',
		),
	);

	$data['transmute_shifts']['title'] = array(
		'title' => t('Shift Title'),
		'help'	=> t('The custom title for the shift (Optional)'),
		'field'	=> array(
			'handler' => 'views_handler_field',
		),
		'filter' => array(
			'handler' => 'views_handler_filter_string',
		),
		'argument' => array(
			'handler' => 'views_handler_argument_string',
			'name field' => 'name',
			'numeric' => FALSE,
		),
		'sort' => array(
			'handler' => 'views_handler_sort',
		),
	);

	$data['transmute_shifts']['start_date'] = array(
		'title' => t('Shift Start Date'),
		'help'	=> t('The datetime the shift starts.'),
		'field'	=> array(
			'handler' => 'views_handler_field_date',
		),
		'filter' => array(
			'handler' => 'views_handler_filter_date',
		),
//		'argument' => array(
//			'handler' => 'views_handler_argument_date',
//		),
		'sort' => array(
			'handler' => 'views_handler_sort_date',
		),
	);
	return $data;
}
