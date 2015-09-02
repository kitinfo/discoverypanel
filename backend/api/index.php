<?php

/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

require_once("output.php");
require_once("pdo_sqlite.php");

main();

function main() {

	$dbpath = "../updater.db3";

	// init
	$output = Output::getInstance();
	$db = new DB($dbpath, $output);

	// db connection
	if (!$db->connect()) {
		header("HTTP/1.0 500 Cannot connect to db");

		die();
	}

	$http_raw = file_get_contents("php://input");
	$obj = json_decode($http_raw, true);

	if (isset($_GET["files"])) {
		$sql = "SELECT * FROM files JOIN trees ON (files.file_id = trees.id)";

		$output->add("files", $db->query($sql, [], DB::F_ARRAY));
	} else if (isset($_GET["trees"])) {
		$sql = "SELECT * FROM trees";

		$output->add("trees", $db->query($sql, [], DB::F_ARRAY));
	} else if (isset($_GET["add_tree"])) {

		if (!isset($obj["base"])) {
			$output->add("status", "We need a base.");
		} else {

			$sql = "INSERT INTO trees (base) VALUES (:base)";
			$params = [
				":base" => $obj["base"]
			];

			$output->add("add_tree", $db->insert($sql, [$params]));
		}
	}


	$output->write();
}

