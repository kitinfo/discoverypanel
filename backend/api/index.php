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

	$dbpath = "/var/db/apis/updater.db3";

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
		$sql = "SELECT trees.base, files.path, group_concat(DISTINCT tags.tag_text) AS tags, files.status FROM files JOIN trees ON (files.tree_id = trees.id) JOIN tagmap ON (files.file_id = tagmap.file) JOIN tags ON (tagmap.tag = tags.tag_id) GROUP BY files.file_id;";

		$output->add("files", $db->query($sql, [], DB::F_ARRAY));
	} else if (isset($_GET["trees"])) {
		$sql = "SELECT * FROM trees";

		$output->add("trees", $db->query($sql, [], DB::F_ARRAY));
	} else if (isset($_GET["add_tree"])) {

		if (!isset($obj["base"])) {
			$output->add("status", "We need a base.");
		} else {
			$comment = "";
			if (isset($obj["comment"]) && !empty($obj["comment"])) {
				$comment = $obj["comment"];
			}
			$sql = "INSERT INTO trees (base, comment) VALUES (:base, :comment)";
			$params = [
				":base" => $obj["base"],
				":comment" => $comment
			];

			$output->add("add_tree", $db->insert($sql, [$params]));
		}
	}

	$output->write();
}

