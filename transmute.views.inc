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
		'title'	=> t('Team'),
		'help' => t("Nodes are a Drupal site's primary content."),
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

	return $data;
}